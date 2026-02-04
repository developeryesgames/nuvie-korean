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
		// Original PC Speaker noise changes freq every few CPU cycles
		// Balance between noise quality and performance
		//
		// 22050 Hz * 0.5 sec = 11025 samples total
		// With samples_per_step=5, need 2200 steps
		num_steps = 2200;  // Rapid frequency changes
		samples_per_step = 5;  // Change freq every 5 samples (~0.2ms)
		// Total length: 2200 * 5 = 11000 samples = ~0.5 sec
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

	// Original U6 algorithm exactly as in SOUND4.ASM:
	// range = base_val - 100 + 1 (with 16-bit unsigned arithmetic)
	// pit_counter = rand_value % range + 100
	// When base_val < 100, underflow creates large range -> wide freq spread
	uint16 range = (uint16)(base_val - 0x64 + 1);
	if(range == 0) range = 1;

	uint16 tmp = rand_value;
	uint16 pit_counter = tmp - (tmp / range) * range;  // tmp % range
	pit_counter += 0x64;  // Add 100

	// Clamp pit_counter to valid range
	if(pit_counter < 19)  // Below 19 would be > 62kHz (inaudible and problematic)
		pit_counter = 19;

	// DOSBox style: Set PIT divisor directly instead of converting to Hz
	// This is more accurate as it preserves the exact PIT timing behavior
	pcspkr->SetPITDivisor(pit_counter);

	// Return Hz for compatibility with legacy code paths
	uint32 hz = 1193182 / pit_counter;
	return (uint16)hz;
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
// OSI_playNoise(10, 30 - (di << 2), 25000 - (di << 11))
// OSI_playNoise(max_freq, duration, step) - based on ASM analysis:
//   [BP+06] = max_freq (1st arg) = 10
//   [BP+08] = duration (2nd arg) = 30 - (di << 2)
//   [BP+0Ah] = step (3rd arg) = 25000 - (di << 11)
// With di=0: max_freq=10, duration=30, step=25000
// base_val=10 < 100 causes underflow -> wide frequency noise (characteristic sound)
Audio::AudioStream *makePCSpeakerFountainSfxStream(uint32 rate)
{
  // Original: OSI_playNoise(10, 30, 25000)
  // base_val=10 triggers underflow handling in getNextFreqValue() for water sound
  return new PCSpeakerRandomStream(10, 30, 25000);
}

// Water wheel sound based on original U6:
// OSI_playNoise(20, 60 - (di << 2), 10000 - (di << 10))
// With di=0: max_freq=20, duration=60, step=10000
// base_val=20 < 100 causes underflow -> wide frequency noise
Audio::AudioStream *makePCSpeakerWaterWheelSfxStream(uint32 rate)
{
  // Original: OSI_playNoise(20, 60, 10000)
  // base_val=20 triggers underflow handling in getNextFreqValue() for water sound
  return new PCSpeakerRandomStream(20, 60, 10000);
}

// Fire/fireplace sound based on original U6:
// case OBJ_0A4: case OBJ_0C9: case OBJ_130: case OBJ_13D: /*Fire*/
//   if(OSI_rand(0, 3) == 0) {
//     for(bp_02 = 0; bp_02 < 5; bp_02++) {
//       if(OSI_rand(0, di + 7) == 0)
//         OSI_sound(OSI_rand(2000, 15000));  // PIT counter 2000-15000
//       else
//         OSI_nosound();
//       OSI_sDelay(8, 1);
//     }
//   }
Audio::AudioStream *makePCSpeakerFireSfxStream(uint32 rate)
{
  // Fire crackle: use RandomStream with low frequency range for crackling
  // Similar to fountain but with different parameters
  // base_val > 100 so no underflow, creates lower frequency range
  return new PCSpeakerRandomStream(5000, 100, 50);  // Lower freq, sparse crackles
}

// Clock sound based on original U6:
// Short tick sound - clock ticks are handled by game timing, not looping
Audio::AudioStream *makePCSpeakerClockSfxStream(uint32 rate)
{
  // Clock tick: PIT counter 3000 -> Hz = 1193180 / 3000 = 398 Hz
  // Just a short tick - game will call this periodically
  uint16 hz = 1193180 / 3000;  // 398 Hz
  return new PCSpeakerFreqStream(hz, 50);  // Short tick sound
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
  // Simplified protection field: high-pitched buzz
  // PIT counter range 200-1500 -> Hz range 795-5966
  uint16 pit_counter = (NUVIE_RAND() % 1301) + 200;
  uint16 hz = 1193180 / pit_counter;
  return new PCSpeakerFreqStream(hz, 8);
}
