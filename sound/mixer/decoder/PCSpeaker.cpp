/*
 *  PCSpeaker.cpp
 *  Nuvie
 *
 *  DOSBox-X style PC Speaker emulation
 *  Ported from DOSBox-X pcspeaker.cpp for accurate PC speaker sound
 *
 *  Original DOSBox code Copyright (C) 2002-2021 The DOSBox Team
 *  This port Copyright (C) 2026 The Nuvie Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 */

#include <math.h>
#include <string.h>
#include "SDL.h"
#include "nuvieDefs.h"

#include "mixer.h"
#include "decoder/PCSpeaker.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// DOSBox uses SPKR_VOLUME=10000 but mixer default is 100%
// PC Speaker is typically too loud - DOSBox community recommends SPKR mixer at 25%
// So we use 10000 * 0.25 = 2500 as effective volume
#define SPKR_VOLUME 2500

PCSpeaker::PCSpeaker(uint32 mixer_rate)
{
	rate = mixer_rate;

	pit_divisor = 0;
	pit_index = 0.0;
	pit_max = 0.0;
	pit_half = 0.0;
	pit_new_max = 0.0;
	pit_new_half = 0.0;
	pit_output_level = true;
	pit_mode3_counting = false;

	// DOSBox-X: minimum_counter = PIT_TICK_RATE/(rate*10)
	// This prevents aliasing from ultrasonic frequencies
	minimum_counter = (uint32)(PIT_TICK_RATE / (rate * 10));
	if(minimum_counter < 2)
		minimum_counter = 2;

	pit_output_enabled = false;
	pit_clock_gate_enabled = false;

	delay_used = 0;
	last_index = 0.0;
	last_output_level = false;

	volcur = 0.0;
	volwant = 0.0;
	// DOSBox-X: SPKR_SPEED = (SPKR_VOLUME*2*44100)/(0.050*rate)
	// This controls the volume slew rate for click removal
	spkr_speed = (double)((SPKR_VOLUME * 2 * 44100) / (0.050 * rate));

	// DOSBox-X style lowpass filter: SetLowpassFreq(14000, 3)
	// 14kHz cutoff, 3rd order filter
	// tau = 1.0 / (freq * 2 * PI)
	// alpha = timeInterval / (tau + timeInterval)
	// timeInterval = 1.0 / rate
	double lowpass_freq = 14000.0;
	double timeInterval = 1.0 / (double)rate;
	double tau = 1.0 / (lowpass_freq * 2.0 * M_PI);
	lowpass_alpha = timeInterval / (tau + timeInterval);

	// Initialize filter state for all orders
	for(int i = 0; i < LOWPASS_ORDER; i++)
		lowpass_state[i] = 0.0;

	wav_length = 0;
}

void PCSpeaker::AddDelayEntry(double index, float output_level)
{
	// DOSBox-X: deduplicate identical consecutive levels
	bool level = (output_level > 0.5f);
	if(level == last_output_level)
		return;
	last_output_level = level;

	if(delay_used >= SPKR_DELAY_ENTRIES)
		return;

	delay_entries[delay_used].index = index;
	delay_entries[delay_used].output_level = level ? 1.0f : 0.0f;
	delay_used++;
}

void PCSpeaker::ForwardPIT(double new_index)
{
	// DOSBox-X Mode 3 square wave generator
	double passed = (new_index - last_index);
	double delay_base = last_index;
	last_index = new_index;

	if(!pit_mode3_counting || pit_half <= 0.0)
		return;

	while(passed > 0.0)
	{
		if((pit_index + passed) >= pit_half)
		{
			double delay = pit_half - pit_index;
			delay_base += delay;
			passed -= delay;
			pit_output_level = !pit_output_level;
			if(pit_output_enabled)
				AddDelayEntry(delay_base, pit_output_level ? 1.0f : 0.0f);
			pit_index = 0.0;

			// Apply pending divisor on toggle (Mode 3 behavior)
			if(pit_new_half > 0.0) {
				pit_half = pit_new_half;
				pit_max = pit_new_max;
			}
		}
		else
		{
			pit_index += passed;
			return;
		}
	}
}

void PCSpeaker::SetOn()
{
	wav_length = 0;
	pit_output_enabled = true;
	pit_clock_gate_enabled = true;
	pit_output_level = true;
	pit_index = 0.0;

	if(pit_half > 0.0)
		pit_mode3_counting = true;
}

void PCSpeaker::SetOff()
{
	pit_output_enabled = false;
	pit_clock_gate_enabled = false;
	pit_mode3_counting = false;
}

void PCSpeaker::SetPITDivisor(uint32 divisor)
{
	if(divisor < minimum_counter)
		divisor = minimum_counter;

	pit_divisor = divisor;

	if(divisor == 0) {
		pit_new_max = pit_new_half = pit_max = pit_half = 0.0;
		pit_mode3_counting = false;
		return;
	}

	// DOSBox-X: pit_new_max = (1000.0f/PIT_TICK_RATE)*cntr
	// Convert PIT divisor to milliseconds
	pit_new_max = (1000.0 * (double)divisor) / (double)PIT_TICK_RATE;
	pit_new_half = pit_new_max / 2.0;

	if(!pit_mode3_counting)
	{
		pit_index = 0.0;
		pit_max = pit_new_max;
		pit_half = pit_new_half;
		if(pit_output_enabled && pit_clock_gate_enabled)
			pit_mode3_counting = true;
	}
}

void PCSpeaker::SetFrequency(uint16 freq, float offset)
{
	if(freq == 0) {
		SetPITDivisor(0);
		return;
	}

	// Convert Hz to PIT divisor: divisor = PIT_TICK_RATE / freq
	uint32 divisor = (uint32)((double)PIT_TICK_RATE / (double)freq);
	if(divisor == 0)
		divisor = 1;
	SetPITDivisor(divisor);

	(void)offset;
}

void PCSpeaker::PCSPEAKER_CallBack(sint16 *stream, const uint32 len)
{
	if(len == 0)
		return;

	// Simplified direct square wave generation
	// Instead of complex delay queue, generate samples directly
	//
	// pit_half is in milliseconds
	// sample_period_ms = 1000/rate milliseconds per sample

	double sample_period_ms = 1000.0 / (double)rate;

	for(uint32 i = 0; i < len; i++)
	{
		double target_vol;

		if(!pit_output_enabled || !pit_mode3_counting || pit_half <= 0.0)
		{
			// Speaker off - target silence
			target_vol = 0.0;
		}
		else
		{
			// Advance PIT counter by one sample period
			pit_index += sample_period_ms;

			// Mode 3: toggle output at pit_half intervals
			while(pit_index >= pit_half)
			{
				pit_index -= pit_half;
				pit_output_level = !pit_output_level;

				// Apply pending divisor on toggle
				if(pit_new_half > 0.0)
				{
					pit_half = pit_new_half;
					pit_max = pit_new_max;
				}
			}

			target_vol = pit_output_level ? (double)SPKR_VOLUME : 0.0;
		}

		// Volume slewing for click removal
		// spkr_speed is volume change per 1ms (index unit)
		// Scale to per-sample: max_change = spkr_speed * sample_period_ms
		double vol_diff = target_vol - volcur;
		if(vol_diff != 0.0)
		{
			double max_change = spkr_speed * sample_period_ms;
			if(fabs(vol_diff) <= max_change)
			{
				volcur = target_vol;
			}
			else if(vol_diff > 0)
			{
				volcur += max_change;
			}
			else
			{
				volcur -= max_change;
			}
		}

		// Apply 3rd order lowpass filter (DOSBox-X style)
		double filtered = volcur;
		for(int order = 0; order < LOWPASS_ORDER; order++)
		{
			lowpass_state[order] += lowpass_alpha * (filtered - lowpass_state[order]);
			filtered = lowpass_state[order];
		}

		stream[i] = (sint16)filtered;
	}
}
