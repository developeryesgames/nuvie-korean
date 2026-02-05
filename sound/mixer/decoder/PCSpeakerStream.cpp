/* Created by Eric Fry 
 * Copyright (C) 2011 The Nuvie Team
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
#include <stdlib.h>
#include <string.h>
#include "nuvieDefs.h"

#include "PCSpeakerStream.h"

// Approximate OSI_sDelay timing (see DELAY2.ASM)
// base tick uses SPKR_OUTPUT_RATE / 1255 (existing legacy constant in this file)
static const uint8 pcspkr_delay_shifts[] = {0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4};

static uint32 pcspkr_delay_samples(uint16 a0, uint16 a2)
{
	const uint32 base = (SPKR_OUTPUT_RATE / 1255);
	uint8 shift = 0;
	if(a2 < (sizeof(pcspkr_delay_shifts) / sizeof(pcspkr_delay_shifts[0])))
		shift = pcspkr_delay_shifts[a2];
	uint32 samples = (a0 * base) >> shift;
	if(samples < 1) samples = 1;
	return samples;
}

// Game tick rate (~18.2065 Hz) used by U6 for clock cadence
static uint32 pcspkr_tick_samples()
{
	return (uint32)(SPKR_OUTPUT_RATE / 18.2065f);
}

static uint32 pcspkr_clock_period_samples()
{
	return (uint32)((SPKR_OUTPUT_RATE * 16.0f) / 18.2065f);
}



PCSpeakerFreqStream::PCSpeakerFreqStream(uint32 freq, uint16 d)
{
	frequency = freq;

	duration = d * (SPKR_OUTPUT_RATE / 1255);
	if(freq != 0)
	{
		pcspkr->SetOn();
		pcspkr->SetFrequency(frequency);
	}

	total_samples_played = 0;
}


PCSpeakerFreqStream::~PCSpeakerFreqStream()
{

}

uint32 PCSpeakerFreqStream::getLengthInMsec()
{
	return (uint32)(duration / (getRate()/1000.0f));
}

int PCSpeakerFreqStream::readBuffer(sint16 *buffer, const int numSamples)
{
	uint32 samples = (uint32)numSamples;

	if(total_samples_played >= duration)
		return 0;

	if(total_samples_played + samples > duration)
		samples = duration - total_samples_played;

	if(frequency != 0)
		pcspkr->PCSPEAKER_CallBack(buffer, samples);
	else
		memset(buffer, 0, sizeof(sint16)*numSamples);

	total_samples_played += samples;

	if(total_samples_played >= duration)
	{
		finished = true;
		pcspkr->SetOff();
	}

	return samples;
}

bool PCSpeakerFreqStream::rewind()
{
	// Reset to beginning for looping ambient sounds (fire, clock, etc.)
	total_samples_played = 0;
	finished = false;

	// Re-enable speaker for new loop iteration
	pcspkr->SetOn();
	if(frequency != 0)
		pcspkr->SetFrequency(frequency);
	return true;
}

//******** PCSpeakerSweepFreqStream

PCSpeakerSweepFreqStream::PCSpeakerSweepFreqStream(uint32 start, uint32 end, uint16 d, uint16 s)
{
	start_freq = start;
	finish_freq = end;
	cur_freq = start_freq;

	num_steps = d / s;
	freq_step = ((finish_freq - start_freq) * s) / d;
	stepping = s;
	duration = d * (SPKR_OUTPUT_RATE / 1255);
	samples_per_step = (float)s * (SPKR_OUTPUT_RATE * 0.000879533f); //(2 * (uint32)(SPKR_OUTPUT_RATE / start_freq)); //duration / num_steps;
	sample_pos = 0.0f;
	pcspkr->SetOn();
	pcspkr->SetFrequency(start_freq);


	total_samples_played = 0;
	cur_step = 0;
	DEBUG(0, LEVEL_DEBUGGING, "num_steps = %d freq_step = %d samples_per_step = %f\n", num_steps, freq_step, samples_per_step);
}


PCSpeakerSweepFreqStream::~PCSpeakerSweepFreqStream()
{

}

uint32 PCSpeakerSweepFreqStream::getLengthInMsec()
{
	return (uint32)((num_steps * samples_per_step) / (getRate()/1000.0f));
}

int PCSpeakerSweepFreqStream::readBuffer(sint16 *buffer, const int numSamples)
{
	uint32 samples = (uint32)numSamples;
	uint32 i;
	//if(total_samples_played >= duration)
	//	return 0;

	//if(total_samples_played + samples > duration)
	//	samples = duration - total_samples_played;

	for(i = 0;i < samples && cur_step < num_steps;)
	{
		//DEBUG(0, LEVEL_DEBUGGING, "sample_pos = %f\n", sample_pos);
		float n = samples_per_step - sample_pos;
		if((float)i + n > (float)samples)
			n = (float)(samples - i);

		float remainder = n - floor(n);
		n = floor(n);
		pcspkr->PCSPEAKER_CallBack(&buffer[i], (uint32)n);
		sample_pos += n;

		i += (uint32)n;
		//DEBUG(0, LEVEL_DEBUGGING, "sample_pos = %f remainder = %f\n", sample_pos, remainder);
		if(sample_pos + remainder >= samples_per_step)
		{
			cur_freq += freq_step;

			pcspkr->SetFrequency(cur_freq, remainder);

			if(remainder != 0.0f)
			{
				sample_pos = 1.0f - remainder;
				pcspkr->PCSPEAKER_CallBack(&buffer[i], 1);
				i++;
			}
			else
			{
				sample_pos = 0;
			}

			cur_step++;
		}

	}

	total_samples_played += i;

	if(cur_step >= num_steps) //total_samples_played >= duration)
	{
		DEBUG(0, LEVEL_DEBUGGING, "total_samples_played = %d cur_freq = %d\n", total_samples_played, cur_freq);
		finished = true;
		pcspkr->SetOff();
	}

	return i;
}


//**************** PCSpeakerRandomStream

PCSpeakerRandomStream::PCSpeakerRandomStream(uint32 freq, uint16 d, uint16 s)
{
	rand_value = 0x7664;
	base_val = freq;
	duration = d;
	stepping = s;

	pcspkr->SetOn();
	// getNextFreqValue() returns Hz (original U6 SOUND_FREQ converts Hz to PIT divisor)
	pcspkr->SetFrequency(getNextFreqValue());

	cur_step = 0;
	sample_pos = 0;

	// Original ASM analysis:
	// The loop generates random freq, plays for DELAY_CXAX cycles, repeats
	// DELAY_CXAX is: CX = step value (from arg), AX = D_2FC6 >> 4 (CPU speed)
	// The delay is extremely short - just a few CPU cycles per iteration
	//
	// For noise to sound like "shhh" instead of "beep", frequency must change
	// VERY rapidly - every few samples (not every few hundred samples)
	//
	// For fountain: base_val=10 causes underflow -> wide freq range noise
	// The key is rapid frequency changes to create white noise effect

	if(s >= d)
	{
		// Noise mode (fountain, water wheel): step >= duration
		//
		// Original PC Speaker noise: frequency changes SO FAST that no
		// proper tone forms. The result is random clicks = noise.
		//
		// Original 4.77MHz 8088: ~10-50 CPU cycles per iteration
		// = ~2-10 microseconds between frequency changes
		// At 22050Hz sample rate: 2us = 0.044 samples
		//
		// To recreate this "noise" character:
		// - Change frequency every 1-2 samples
		// - This prevents any tone from forming
		// - Result: random amplitude noise (또로로록)
		//
		// If we change every 5ms = 110 samples, we get "삐비비비" tones
		// If we change every 1-2 samples, we get noise
		num_steps = 500;  // Many frequency changes
		samples_per_step = 2;  // Change every 2 samples (~0.09ms)
		// Total: 500 * 2 = 1000 samples = ~45ms of noise
	}
	else
	{
		// Multi-step mode: generate new freq each step
		num_steps = (s > 0) ? d / s : 1;
		if(num_steps == 0)
			num_steps = 1;
		// Each step lasts approximately 'step' delay units
		samples_per_step = (s > 0) ? (s * SPKR_OUTPUT_RATE) / 10000 : 20;
		if(samples_per_step < 1)
			samples_per_step = 1;
	}

	total_samples_played = 0;
	DEBUG(0, LEVEL_DEBUGGING, "RandomStream: base=%d d=%d s=%d num_steps=%d samples_per_step=%d\n",
	      base_val, d, s, num_steps, samples_per_step);

}


PCSpeakerRandomStream::~PCSpeakerRandomStream()
{

}

bool PCSpeakerRandomStream::rewind()
{
	// Reset to beginning for looping ambient sounds (fountain, water wheel)
	// DON'T reset rand_value - keep it running to avoid repetitive patterns
	cur_step = 0;
	sample_pos = 0;
	total_samples_played = 0;
	finished = false;

	// MUST call SetOn() to re-enable speaker after SetOff() was called
	// This resets the PIT state and delay queue for clean looping
	pcspkr->SetOn();
	// getNextFreqValue() returns Hz
	pcspkr->SetFrequency(getNextFreqValue());
	return true;
}

uint32 PCSpeakerRandomStream::getLengthInMsec()
{
	return (uint32)((num_steps * samples_per_step) / (getRate()/1000.0f));
}

uint16 PCSpeakerRandomStream::getNextFreqValue()
{
	// Update random value using original U6 PRNG algorithm
	rand_value += 0x9248;
	rand_value = rand_value & 0xffff;
	uint16 bits = rand_value & 0x7;
	rand_value = (rand_value >> 3) + (bits << 13); // rotate right by 3 bits
	rand_value = rand_value ^ 0x9248;
	rand_value += 0x11;
	rand_value = rand_value & 0xffff;

	// Original U6 algorithm: range = base_val - 100 + 1 (with 16-bit unsigned arithmetic)
	// When base_val < 100 (fountain=10, water wheel=20), underflow creates large range.
	//
	// However, original PC Speaker hardware can't reproduce high frequencies well.
	// The physical speaker acts as a lowpass filter (~5kHz effective cutoff).
	// High frequencies (10kHz+) become quiet clicks, not audible tones.
	//
	// For authentic "dripping water" sound (또로로록), use limited frequency range:
	// - Fountain/water: 200-2000 Hz (low gurgling tones)
	// - Original wide range (100-65535) sounds like white noise (쏴아아)
	//
	// When base_val < 100, use "water sound" limited range
	uint16 freq;
	if(base_val < 100)
	{
		// Water sounds: limited to audible low-frequency range
		// 200-2000 Hz gives "dripping/gurgling" character
		freq = (rand_value % 1800) + 200;
	}
	else
	{
		// Normal range calculation
		uint16 range = (uint16)(base_val - 0x64 + 1);
		if(range == 0) range = 1;
		freq = (rand_value % range) + 0x64;
	}

	return freq;
}

int PCSpeakerRandomStream::readBuffer(sint16 *buffer, const int numSamples)
{
	uint32 samples = (uint32)numSamples;
	uint32 s = 0;

	// Debug: log first call info
	static int call_count = 0;
	if(call_count < 3)
	{
		DEBUG(0, LEVEL_DEBUGGING, "readBuffer: numSamples=%d cur_step=%d num_steps=%d samples_per_step=%d\n",
		      numSamples, cur_step, num_steps, samples_per_step);
		call_count++;
	}

	for(uint32 i=0;i < samples && cur_step <= num_steps;)
	{
		uint32 n = samples_per_step - sample_pos;
		if(i + n > samples)
			n = samples - i;

		pcspkr->PCSPEAKER_CallBack(&buffer[i], n);
		sample_pos += n;
		i += n;

		if(sample_pos >= samples_per_step)
		{
			uint16 freq = getNextFreqValue();
			// Debug: log frequency changes (first few only)
			static int freq_log_count = 0;
			if(freq_log_count < 5)
			{
				DEBUG(0, LEVEL_DEBUGGING, "SetFrequency: %d Hz\n", freq);
				freq_log_count++;
			}
			// Original U6 SOUND_FREQ converts Hz to PIT divisor
			// So getNextFreqValue() returns Hz, use SetFrequency
			pcspkr->SetFrequency(freq);
			sample_pos = 0;
			cur_step++;
		}

		s += n;

	}

	total_samples_played += s;

	if(cur_step >= num_steps)
	{
		DEBUG(0, LEVEL_DEBUGGING, "Finished: total_samples_played = %d\n", total_samples_played);
		finished = true;
		pcspkr->SetOff();
	}

	return s;
}

//**************** PCSpeakerRapidClickStream
//
// Exactly the same pattern as PCSpeakerTickStream (clock), but:
// - Many clicks instead of just tick/tock
// - Random frequency per click
// - Short gap between clicks
// Pattern per click: SetOn → SetFrequency → CallBack(tick_len) → SetOff → CallBack(gap_len)
//
class PCSpeakerRapidClickStream : public PCSpeakerStream
{
public:
	// tick_ms: duration of each audible click
	// gap_ms: silence between clicks
	// num_clicks: how many clicks total
	// min_freq, max_freq: frequency range in Hz
	PCSpeakerRapidClickStream(uint32 tick_ms, uint32 gap_ms, uint32 num_clicks_val, uint16 min_freq, uint16 max_freq)
	{
		tick_len = (SPKR_OUTPUT_RATE * tick_ms) / 1000;
		gap_len = (SPKR_OUTPUT_RATE * gap_ms) / 1000;
		if(tick_len < 1) tick_len = 1;
		if(gap_len < 1) gap_len = 1;

		num_clicks = num_clicks_val;
		click_period = tick_len + gap_len;
		total_samples = click_period * num_clicks;

		freq_min = min_freq;
		freq_range = (max_freq > min_freq) ? (max_freq - min_freq) : 1;

		rand_state = 0x7664;

		total_samples_played = 0;
		finished = false;
		pcspkr->SetOff();
	}

	uint32 getLengthInMsec()
	{
		return (uint32)(total_samples / (getRate()/1000.0f));
	}

	int readBuffer(sint16 *buffer, const int numSamples)
	{
		uint32 samples = (uint32)numSamples;
		if(total_samples_played >= total_samples)
			return 0;

		if(total_samples_played + samples > total_samples)
			samples = total_samples - total_samples_played;

		uint32 written = 0;
		while(written < samples)
		{
			uint32 pos = total_samples_played + written;
			uint32 click_pos = pos % click_period;  // position within current click cycle
			uint32 click_end = pos - click_pos + click_period;  // end of current click cycle

			if(click_pos < tick_len)
			{
				// Audible part: SetOn + frequency
				uint32 click_idx = pos / click_period;
				pcspkr->SetOn();
				// Deterministic freq per click: seed from click index
				pcspkr->SetFrequency(getFreqForClick(click_idx));
				uint32 seg_end = pos - click_pos + tick_len;
				uint32 n = seg_end - pos;
				if(written + n > samples)
					n = samples - written;
				pcspkr->PCSPEAKER_CallBack(&buffer[written], n);
				written += n;
			}
			else
			{
				// Silent gap
				pcspkr->SetOff();
				uint32 n = click_end - pos;
				if(written + n > samples)
					n = samples - written;
				pcspkr->PCSPEAKER_CallBack(&buffer[written], n);
				written += n;
			}
		}

		total_samples_played += written;

		if(total_samples_played >= total_samples)
		{
			finished = true;
			pcspkr->SetOff();
		}

		return written;
	}

	bool rewind()
	{
		total_samples_played = 0;
		finished = false;
		pcspkr->SetOff();
		return true;
	}

private:
	uint16 getFreqForClick(uint32 click_idx)
	{
		// Generate deterministic random freq for each click
		// Using U6 PRNG seeded per click
		uint16 r = (uint16)((rand_state + click_idx * 0x9248) & 0xffff);
		uint16 bits = r & 0x7;
		r = (r >> 3) + (bits << 13);
		r ^= 0x9248;
		r += 0x11;
		return (r % freq_range) + freq_min;
	}

	uint32 total_samples;
	uint32 total_samples_played;
	uint32 tick_len;
	uint32 gap_len;
	uint32 click_period;
	uint32 num_clicks;
	uint16 freq_min;
	uint16 freq_range;
	uint16 rand_state;
};

//**************** PCSpeakerTickStream (clock)

class PCSpeakerTickStream : public PCSpeakerStream
{
public:
	PCSpeakerTickStream(uint32 pit_tick_div, uint32 pit_tock_div, uint32 tick_len_samples, uint32 period_len_samples)
	{
		pit_tick = pit_tick_div;
		pit_tock = pit_tock_div;
		tick_samples = tick_len_samples;
		period_samples = period_len_samples;
		if(period_samples == 0)
			period_samples = 1;
		if(tick_samples > period_samples)
			tick_samples = period_samples;

		tock_offset_samples = period_samples / 2;

		total_samples_played = 0;
		finished = false;
		pcspkr->SetOff();
	}

	uint32 getLengthInMsec()
	{
		return (uint32)(period_samples / (getRate()/1000.0f));
	}

	int readBuffer(sint16 *buffer, const int numSamples)
	{
		uint32 samples = (uint32)numSamples;

		if(total_samples_played >= period_samples)
			return 0;

		if(total_samples_played + samples > period_samples)
			samples = period_samples - total_samples_played;

		uint32 written = 0;
		while(written < samples)
		{
			uint32 global_pos = total_samples_played + written;
			uint32 segment_end = period_samples;

			if(global_pos < tick_samples)
			{
				pcspkr->SetOn();
				pcspkr->SetFrequency((uint16)pit_tick);  // Hz frequency
				segment_end = tick_samples;
			}
			else if(global_pos < tock_offset_samples)
			{
				pcspkr->SetOff();
				segment_end = tock_offset_samples;
			}
			else if(global_pos < tock_offset_samples + tick_samples)
			{
				pcspkr->SetOn();
				pcspkr->SetFrequency((uint16)pit_tock);  // Hz frequency
				segment_end = tock_offset_samples + tick_samples;
			}
			else
			{
				pcspkr->SetOff();
				segment_end = period_samples;
			}

			uint32 n = segment_end - global_pos;
			if(written + n > samples)
				n = samples - written;

			pcspkr->PCSPEAKER_CallBack(&buffer[written], n);
			written += n;
		}

		total_samples_played += written;

		if(total_samples_played >= period_samples)
		{
			finished = true;
			pcspkr->SetOff();
		}

		return written;
	}

	bool rewind()
	{
		total_samples_played = 0;
		finished = false;
		pcspkr->SetOff();
		return true;
	}

private:
	uint32 pit_tick;
	uint32 pit_tock;
	uint32 tick_samples;
	uint32 period_samples;
	uint32 tock_offset_samples;
	uint32 total_samples_played;
};
//**************** PCSpeakerFireClickStream (fire / fireplace)
//
// Fire crackle: each cycle randomly produces 0~3 clicks with random gaps.
// "탁", "타닥", "탁.." pattern. Volume scaled down.
// Same SetOn/SetFrequency/CallBack/SetOff pattern as clock.
//
class PCSpeakerFireClickStream : public PCSpeakerStream
{
public:
	PCSpeakerFireClickStream()
	{
		// Total cycle length = ~55ms (one game tick)
		total_samples = pcspkr_tick_samples();
		total_samples_played = 0;
		finished = false;
		rand_state = 0x7664;

		// Pre-generate this cycle's click pattern
		generatePattern();

		pcspkr->SetOff();
	}

	uint32 getLengthInMsec()
	{
		return (uint32)(total_samples / (getRate()/1000.0f));
	}

	int readBuffer(sint16 *buffer, const int numSamples)
	{
		uint32 samples = (uint32)numSamples;
		if(total_samples_played >= total_samples)
			return 0;

		if(total_samples_played + samples > total_samples)
			samples = total_samples - total_samples_played;

		uint32 written = 0;
		while(written < samples)
		{
			uint32 pos = total_samples_played + written;

			// Find which segment we're in
			bool found = false;
			for(int i = 0; i < click_count; i++)
			{
				uint32 click_start = click_offsets[i];
				uint32 click_end = click_start + click_len;

				if(pos >= click_start && pos < click_end)
				{
					// In audible click
					pcspkr->SetOn();
					pcspkr->SetFrequency(click_freqs[i]);
					uint32 n = click_end - pos;
					if(written + n > samples)
						n = samples - written;
					pcspkr->PCSPEAKER_CallBack(&buffer[written], n);
					written += n;
					found = true;
					break;
				}
			}

			if(!found)
			{
				// In silence - find next click or end
				pcspkr->SetOff();
				uint32 next_boundary = total_samples;
				for(int i = 0; i < click_count; i++)
				{
					if(click_offsets[i] > pos && click_offsets[i] < next_boundary)
						next_boundary = click_offsets[i];
				}
				uint32 n = next_boundary - pos;
				if(written + n > samples)
					n = samples - written;
				pcspkr->PCSPEAKER_CallBack(&buffer[written], n);
				written += n;
			}
		}

		total_samples_played += written;

		if(total_samples_played >= total_samples)
		{
			finished = true;
			pcspkr->SetOff();
		}

		// Scale down volume - fire is subtle
		for(uint32 i = 0; i < written; i++)
			buffer[i] = (sint16)(buffer[i] / 8);

		return written;
	}

	bool rewind()
	{
		total_samples_played = 0;
		finished = false;
		generatePattern();
		pcspkr->SetOff();
		return true;
	}

private:
	void generatePattern()
	{
		// 3ms per click (same as clock)
		click_len = (SPKR_OUTPUT_RATE * 3) / 1000;
		if(click_len < 1) click_len = 1;

		// Random 0-3 clicks per cycle
		advanceRand();
		click_count = rand_state % 4;  // 0, 1, 2, or 3

		for(int i = 0; i < click_count; i++)
		{
			// Random position within the cycle
			advanceRand();
			click_offsets[i] = rand_state % (total_samples - click_len);

			// Random freq 2000-15000 Hz
			advanceRand();
			click_freqs[i] = (rand_state % 13001) + 2000;
		}

		// Sort by offset to avoid overlap issues
		for(int i = 0; i < click_count - 1; i++)
		{
			for(int j = i + 1; j < click_count; j++)
			{
				if(click_offsets[j] < click_offsets[i])
				{
					uint32 tmp = click_offsets[i];
					click_offsets[i] = click_offsets[j];
					click_offsets[j] = tmp;
					uint16 tmp2 = click_freqs[i];
					click_freqs[i] = click_freqs[j];
					click_freqs[j] = tmp2;
				}
			}
		}

		// Remove overlapping clicks
		for(int i = 1; i < click_count; i++)
		{
			if(click_offsets[i] < click_offsets[i-1] + click_len)
			{
				click_offsets[i] = click_offsets[i-1] + click_len + (SPKR_OUTPUT_RATE / 100); // 10ms gap
			}
		}
	}

	void advanceRand()
	{
		rand_state += 0x9248;
		rand_state &= 0xffff;
		uint16 bits = rand_state & 0x7;
		rand_state = (rand_state >> 3) + (bits << 13);
		rand_state ^= 0x9248;
		rand_state += 0x11;
		rand_state &= 0xffff;
	}

	uint32 total_samples;
	uint32 total_samples_played;
	uint32 click_len;
	int click_count;            // 0-3 clicks per cycle
	uint32 click_offsets[4];    // sample offset for each click
	uint16 click_freqs[4];      // frequency for each click
	uint16 rand_state;
};
//**************** PCSpeakerStutterStream

PCSpeakerStutterStream::PCSpeakerStutterStream(sint16 a0, uint16 a2, uint16 a4, uint16 a6, uint16 a8)
{
	arg_0 = a0;
	arg_2 = a2;
	arg_4 = a4;
	arg_6 = a6;
	arg_8 = a8;

	cx = arg_4;
	dx = 0;

	pcspkr->SetOn();
	pcspkr->SetFrequency(22096);

	delay = (SPKR_OUTPUT_RATE / 22050) * arg_6; //FIXME this needs to be refined.
	if(delay < 1) delay = 1;  // Prevent zero delay issues
	delay_remaining = 0;

	//samples_per_step = s * (SPKR_OUTPUT_RATE / 20 / 800); //1255);
	//total_samples_played = 0;
	//DEBUG(0, LEVEL_DEBUGGING, "num_steps = %d samples_per_step = %d\n", num_steps, samples_per_step);

}


PCSpeakerStutterStream::~PCSpeakerStutterStream()
{

}

uint32 PCSpeakerStutterStream::getLengthInMsec()
{
	return (uint32)((arg_4 * delay) / (getRate()/1000.0f));
}


int PCSpeakerStutterStream::readBuffer(sint16 *buffer, const int numSamples)
{
	uint32 s = 0;

	for(; cx > 0 && s < (uint32)numSamples; cx--)
	{
		uint32 n = (uint32)floor(delay_remaining);
		if(n > 0)
		{
			pcspkr->PCSPEAKER_CallBack(&buffer[s], n);
			delay_remaining -= n;
			s += n;
		}

	   dx = (dx + arg_8) & 0xffff;

	   if(dx > arg_2)
	   {
		   pcspkr->SetOn();
	   }
	   else
	   {
		   pcspkr->SetOff();
	   }

	   arg_2 += arg_0;
/*
	   for(int i = arg_6; i > 0 ; i--)
	   {
	      for(int j = counter;j > 0;)
	      {
             j--;
	      }
	   }
*/
	   n = (uint32)floor(delay);
	   if(s + n > (uint32)numSamples)
		   n = numSamples - s;

	   pcspkr->PCSPEAKER_CallBack(&buffer[s], n);
	   delay_remaining = delay - n;
	   s += n;
	}

	if(cx <= 0)
	{
		//DEBUG(0, LEVEL_DEBUGGING, "total_samples_played = %d\n", total_samples_played);
		finished = true;
		pcspkr->SetOff();
	}

	return s;
}

Audio::AudioStream *makePCSpeakerSlugDissolveSfxStream(uint32 rate)
{
	// Always use SPKR_OUTPUT_RATE for queuing stream to match PCSpeakerStream::getRate()
	Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);
	for(uint16 i=0;i<20;i++)
	{
		stream->queueAudioStream(new PCSpeakerRandomStream((NUVIE_RAND() % 0x1068) + 0x258, 0x15e, 1), DisposeAfterUse::YES);
	}

	return stream;
}

Audio::AudioStream *makePCSpeakerGlassSfxStream(uint32 rate)
{
	Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);
	for(uint16 i=0x7d0;i<0x4e20;i+=0x3e8)
	{
		stream->queueAudioStream(new PCSpeakerRandomStream(i,0x78, 0x28), DisposeAfterUse::YES);
	}

	return stream;
}


Audio::AudioStream *makePCSpeakerMagicCastingP1SfxStream(uint32 rate, uint8 magic_circle)
{
	//Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(rate, false);

	return new PCSpeakerRandomStream(0x2bc, 0x640 * magic_circle + 0x1f40, 0x320);

	//return stream;
}

Audio::AudioStream *makePCSpeakerMagicCastingP2SfxStream(uint32 rate, uint8 magic_circle)
{
	Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);

	const sint16 word_30188[] = {3, 2, 2, 2, 1, 1, 1, 1, 1};

	const uint16 word_30164[] = {0xA8C, 0xBB8, 0x3E8, 0x64, 0x1388, 0xFA0, 0x9C4, 0x3E8, 1};
	const uint16 word_30176[] = {0x7FBC, 0x7918, 0x9088, 0xAFC8, 0x7918, 0x84D0, 0x8E94, 0x9858, 0xA410};

	const uint16 word_30152[] = {0x226A, 0x1E96, 0x1B94, 0x1996, 0x173E, 0x15C2, 0x143C, 0x12D4, 0x1180};

	stream->queueAudioStream(new PCSpeakerStutterStream(word_30188[magic_circle], word_30164[magic_circle], (magic_circle * 0xfa0) + 0x2710, 1, word_30152[magic_circle]));
	stream->queueAudioStream(new PCSpeakerStutterStream(-word_30188[magic_circle], word_30176[magic_circle], (magic_circle * 0xfa0) + 0x2710, 1, word_30152[magic_circle]));

	return stream;
}

Audio::AudioStream *makePCSpeakerAvatarDeathSfxStream(uint32 rate)
{
	const uint16 avatar_death_tune[] = {0x12C, 0x119, 0x12C, 0xFA, 0x119, 0xDE, 0xFA, 0xFA};

	Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);
	for(uint8 i=0;i<8;i++)
	{
		stream->queueAudioStream(new PCSpeakerStutterStream(3, 1, 0x4e20, 1, avatar_death_tune[i]));
	}

	return stream;
}

Audio::AudioStream *makePCSpeakerKalLorSfxStream(uint32 rate)
{
	Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);
	for(uint8 i=0;i<0x32;i++)
	{
		stream->queueAudioStream(new PCSpeakerStutterStream((0x32 - i) << 2, 0x2710 - (i << 6), 0x3e8, 1, (i << 4) + 0x320));
	}

	stream->queueAudioStream(new PCSpeakerStutterStream(8, 0, 0x1f40, 1, 0x640));

	return stream;
}

Audio::AudioStream *makePCSpeakerHailStoneSfxStream(uint32 rate)
{
	//FIXME This doesn't sound right. It should probably use a single pcspkr object. The original also plays the hailstones individually not all at once like we do. :(
	Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);

	for(uint16 i=0;i<0x28;i++)
	{
		stream->queueAudioStream(new PCSpeakerFreqStream((NUVIE_RAND()%0x28)+0x20, 8), DisposeAfterUse::YES);
	}

	/* The original logic looks something like this. But this doesn't sound right.
	uint16 base_freq = (NUVIE_RAND()%0x64)+0x190;
	for(uint16 i=0;i<0x28;i++)
	{
		if(NUVIE_RAND()%7==0)
			stream->queueAudioStream(new PCSpeakerFreqStream(base_freq + (NUVIE_RAND()%0x28), 8), DisposeAfterUse::YES);
		else
			stream->queueAudioStream(new PCSpeakerFreqStream(0, 8), DisposeAfterUse::YES);
	}
	 */

	return stream;
}

Audio::AudioStream *makePCSpeakerEarthQuakeSfxStream(uint32 rate)
{
  Audio::QueuingAudioStream *stream = Audio::makeQueuingAudioStream(SPKR_OUTPUT_RATE, false);

  for(uint16 i=0;i<0x28;i++)
  {
    stream->queueAudioStream(new PCSpeakerFreqStream((NUVIE_RAND()%0xb5)+0x13, 8), DisposeAfterUse::YES);
  }

  return stream;
}

// Fountain sound based on original U6:
// OSI_playNoise(10, 30, 25000) - rapid frequency changes create "noise"
// Using same PC Speaker click method as clock, just very fast with random freq.
// Each click needs enough samples to form audible tone (~1-2ms per click)
Audio::AudioStream *makePCSpeakerFountainSfxStream(uint32 rate)
{
  // Fountain: rapid random-frequency clicks, like fast clock ticks
  // tick=3ms (same as clock), gap=7ms (clean separation between clicks)
  // ~6 clicks per cycle = ~60ms total
  // Low freq range (200-800 Hz) for water dripping "또로로록" character
  return new PCSpeakerRapidClickStream(3, 20, 3, 200, 800);
}

// Water wheel sound based on original U6:
// OSI_playNoise(20, 60, 10000) - similar to fountain, slightly different.
Audio::AudioStream *makePCSpeakerWaterWheelSfxStream(uint32 rate)
{
  // Water wheel: similar to fountain but slightly more clicks
  // tick=3ms, gap=7ms, ~7 clicks = ~70ms total
  return new PCSpeakerRapidClickStream(3, 20, 4, 200, 600);
}

// Fire/fireplace sound based on original U6:
// OSI_sound(OSI_rand(2000, 15000)) with ~25% play / 75% silence
// Using same PC Speaker click method as clock.
Audio::AudioStream *makePCSpeakerFireSfxStream(uint32 rate)
{
  // Fire: random 0-3 clicks per cycle, scattered throughout ~55ms
  // "탁", "타닥", "탁.." random pattern, volume /8
  return new PCSpeakerFireClickStream();
}

// Clock sound based on original U6:
// OSI_playNote(3000, 3) -> OSI_sound(3000) which is Hz frequency
// SOUND_FREQ macro: port[0x42] = 1193182 / freq (converts Hz to PIT divisor)
// So 3000 Hz tick, 2000 Hz tock
Audio::AudioStream *makePCSpeakerClockSfxStream(uint32 rate)
{
  // Clock tick/tock using Hz frequencies (OSI_sound takes Hz)
  // Tick/tock at ~18.2Hz game ticks: tick at 0, tock at 8 (period = 16 ticks)
  // Original delay depends on CPU speed calibration (D_2FC6).
  // Use longer duration for audible tick/tock (~20-30ms each)
  const uint32 tick_len = (SPKR_OUTPUT_RATE * 3) / 1000;  // 3ms tick duration
  const uint32 period_len = pcspkr_clock_period_samples();
  // Hz frequencies: 3000 Hz (high tick), 2000 Hz (lower tock)
  return new PCSpeakerTickStream(3000, 2000, tick_len, period_len);
}

// Protection field sound based on original U6:
// if(OSI_rand(0, 1) == 0) {
//   for(bp_02 = 0; bp_02 < 80; bp_02++) {
//     if(OSI_rand(0, 15) == 0)
//       OSI_sound(OSI_rand(200, 1500 - (di << 8)));  // PIT 200-1500
//     else
//       OSI_nosound();
//     OSI_sDelay(8, 1);
//   }
// }
Audio::AudioStream *makePCSpeakerProtectionFieldSfxStream(uint32 rate)
{
  // Simplified protection field: high-pitched buzz (Hz range 200-1500)
  uint16 freq = (NUVIE_RAND() % 1301) + 200;
  return new PCSpeakerFreqStream(freq, 8);
}



