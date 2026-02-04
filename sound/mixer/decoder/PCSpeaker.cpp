/*
 *  PCSpeaker.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Sun Feb 13 2011.
 *  Copyright (c) 2011. All rights reserved.
 *
 *  DOSBox-style PC Speaker emulation - 2026
 *  Based on DOSBox's PIT timer + delay queue approach
 *  This accurately emulates the noise effects used in original U6
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <math.h>
#include <string.h>
#include "SDL.h"
#include "nuvieDefs.h"

#include "mixer.h"
#include "decoder/PCSpeaker.h"

#define SPKR_VOLUME 5000

PCSpeaker::PCSpeaker(uint32 mixer_rate)
{
	rate = mixer_rate;

	// PIT state initialization
	pit_divisor = 0;
	pit_index = 0.0;
	pit_max = 0.0;
	pit_half = 0.0;
	pit_output = true;  // DOSBox starts with output high
	pit_output_level = (float)SPKR_VOLUME;

	// Gate state
	enabled = false;
	pit_gate = false;

	// Delay queue initialization
	delay_write_index = 0;
	delay_read_index = 0;
	delay_base_index = 0.0;
	memset(delay_entries, 0, sizeof(delay_entries));

	// Volume - start at current output level, not zero
	cur_vol = 0.0f;
	target_vol = 0.0f;

	wav_length = 0;
}

void PCSpeaker::SetOn()
{
	wav_length = 0;
	enabled = true;
	pit_gate = true;

	// DOSBox Mode 3: On rising edge of gate, reset PIT and start high
	pit_index = 0;
	pit_output = true;
	pit_output_level = (float)SPKR_VOLUME;

	// Add delay entry for speaker on
	AddDelayEntry(delay_base_index, pit_output_level);
}

void PCSpeaker::SetOff()
{
	enabled = false;
	pit_gate = false;
	pit_output_level = 0.0f;

	// When speaker turns off, add a delay entry to go to zero
	AddDelayEntry(delay_base_index, 0.0f);
}

void PCSpeaker::SetFrequency(uint16 freq, float offset)
{
	if(freq == 0)
	{
		pit_divisor = 0;
		pit_max = 0;
		return;
	}

	// Convert Hz to PIT divisor: divisor = PIT_TICK_RATE / freq
	uint32 divisor = PIT_TICK_RATE / freq;
	if(divisor < 2) divisor = 2;  // Minimum divisor

	SetPITDivisor(divisor);
}

// DOSBox-style: Set PIT divisor directly (more accurate)
// This is called when the counter is written (like port 42h writes)
void PCSpeaker::SetPITDivisor(uint32 divisor)
{
	if(divisor == pit_divisor)
		return;

	pit_divisor = divisor;

	if(pit_divisor > 0)
	{
		// DOSBox Mode 3 timing calculation:
		// pit_max = milliseconds per full PIT cycle
		// pit_half = milliseconds per half cycle (when output toggles)
		// Using same formula as DOSBox: (1000.0/PIT_TICK_RATE)*divisor
		double new_max = ((double)pit_divisor * 1000.0) / (double)PIT_TICK_RATE;
		double new_half = new_max / 2.0;

		// DOSBox approach: update timing parameters
		// Don't reset pit_index on frequency change - this preserves phase continuity
		pit_max = new_max;
		pit_half = new_half;

		// If pit_index exceeds new cycle length, wrap it
		if(pit_index > pit_max)
			pit_index = fmod(pit_index, pit_max);
	}
	else
	{
		pit_max = 0;
		pit_half = 0;
	}
}

// Add entry to delay queue (DOSBox style)
void PCSpeaker::AddDelayEntry(double index, float vol)
{
	// Don't add duplicate consecutive entries
	if(delay_write_index > 0)
	{
		uint32 prev_idx = (delay_write_index - 1) % SPKR_DELAY_ENTRIES;
		if(delay_entries[prev_idx].vol == vol)
			return;
	}

	// If the queue is full, drop the oldest entry to avoid overwrite
	if(delay_write_index - delay_read_index >= SPKR_DELAY_ENTRIES)
	{
		delay_read_index = delay_write_index - (SPKR_DELAY_ENTRIES - 1);
	}

	delay_entries[delay_write_index % SPKR_DELAY_ENTRIES].index = index;
	delay_entries[delay_write_index % SPKR_DELAY_ENTRIES].vol = vol;
	delay_write_index++;
}

// Advance PIT by given amount of time (milliseconds) - DOSBox Mode 3 (square wave)
// In Mode 3, output toggles at HALF cycle, not full cycle
void PCSpeaker::ForwardPIT(double amount_ms)
{
	if(!enabled || pit_max <= 0 || pit_half <= 0)
		return;

	double done = 0;
	double start_index = delay_base_index;

	while(done < amount_ms)
	{
		double remaining = amount_ms - done;

		// Calculate time until next toggle (at pit_half boundary)
		double time_to_toggle;
		if(pit_output)
		{
			// Currently high - toggle when we reach pit_half
			time_to_toggle = pit_half - pit_index;
		}
		else
		{
			// Currently low - toggle when we complete the cycle (pit_max)
			time_to_toggle = pit_max - pit_index;
		}

		if(time_to_toggle <= 0)
		{
			// Toggle immediately
			pit_output = !pit_output;
			pit_output_level = pit_output ? (float)SPKR_VOLUME : (float)(-SPKR_VOLUME);
			AddDelayEntry(start_index + done, pit_output_level);

			// Reset pit_index when going from low to high (completing cycle)
			if(pit_output)
				pit_index = 0;
		}
		else if(time_to_toggle <= remaining)
		{
			// We'll reach a toggle point within this amount
			done += time_to_toggle;
			pit_index += time_to_toggle;

			// Toggle output
			pit_output = !pit_output;
			pit_output_level = pit_output ? (float)SPKR_VOLUME : (float)(-SPKR_VOLUME);
			AddDelayEntry(start_index + done, pit_output_level);

			// Reset pit_index when completing cycle (going back to high)
			if(pit_output)
				pit_index = 0;
		}
		else
		{
			// No toggle in this period
			pit_index += remaining;
			done = amount_ms;
		}
	}
}

// DOSBox-style sample generation using delay queue (time in milliseconds)
void PCSpeaker::PCSPEAKER_CallBack(sint16 *stream, const uint32 len)
{
	const double sample_ms = 1000.0 / (double)rate;
	for(uint32 i = 0; i < len; i++)
	{
		double sample_start = delay_base_index;
		double sample_end = delay_base_index + sample_ms;

		// Advance PIT for this sample (this adds entries to delay queue)
		if(enabled && pit_max > 0)
		{
			ForwardPIT(sample_ms);
		}

		// Process delay queue entries within this sample period
		// Calculate average volume over the sample period (like DOSBox)
		double total_vol = 0.0;
		double last_index = sample_start;

		// Use pit_output_level as the current volume state
		// This ensures we have the correct volume even when no queue entries exist
		float last_vol = pit_output_level;

		// If speaker is disabled, volume is 0
		if(!enabled)
			last_vol = 0.0f;

		while(delay_read_index < delay_write_index)
		{
			PCSpeakerDelayEntry& entry = delay_entries[delay_read_index % SPKR_DELAY_ENTRIES];

			if(entry.index >= sample_end)
				break;

			// Accumulate volume for the period before this entry
			if(entry.index > last_index)
			{
				total_vol += last_vol * (entry.index - last_index);
				last_index = entry.index;
			}

			last_vol = entry.vol;
			delay_read_index++;
		}

		// Accumulate remaining period with current volume
		if(sample_end > last_index)
		{
			total_vol += last_vol * (sample_end - last_index);
		}

		// Total volume is weighted average over the sample period
		const double sample_value = (sample_ms > 0.0) ? (total_vol / sample_ms) : 0.0;
		stream[i] = (sint16)sample_value;

		// Update cur_vol for fade-out when disabled
		cur_vol = (float)sample_value;

		delay_base_index = sample_end;
	}

	// Compact delay queue periodically
	if(delay_base_index > 100000)
	{
		double adjust = delay_base_index - 1000;
		delay_base_index -= adjust;

		for(uint32 j = delay_read_index; j < delay_write_index; j++)
		{
			delay_entries[j % SPKR_DELAY_ENTRIES].index -= adjust;
		}
	}
}



