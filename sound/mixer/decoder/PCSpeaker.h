#ifndef __PCSpeaker_h__
#define __PCSpeaker_h__
/* Created by Eric Fry
 * Copyright (C) 2011 The Nuvie Team
 *
 * DOSBox-style PC Speaker emulation added 2026
 * Based on DOSBox PIT timer approach for accurate noise generation
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

#include "NuvieIOFile.h"

#define SPKR_OUTPUT_RATE 22050
#define PIT_TICK_RATE 1193182  // Original PC PIT clock frequency

// DOSBox-style delay entry for tracking speaker state changes
struct PCSpeakerDelayEntry {
	double index;      // Time index (in samples)
	float vol;         // Volume level at this point
};

#define SPKR_DELAY_ENTRIES 1024

class PCSpeaker {
private:
	uint32 rate;                    // Output sample rate (22050)

	// PIT Timer state (DOSBox style)
	uint32 pit_divisor;             // PIT counter divisor (frequency = PIT_TICK_RATE / divisor)
	double pit_index;               // Current position in PIT cycle
	double pit_max;                 // PIT cycle length in samples (full cycle)
	double pit_half;                // Half cycle length in samples (for Mode 3)
	bool pit_output;                // Current PIT output state (high/low)
	float pit_output_level;         // Current output level (+/- SPKR_VOLUME)

	// Speaker gate state
	bool enabled;                   // Speaker enabled (port 61h bit 1)
	bool pit_gate;                  // PIT gate (port 61h bit 0)

	// DOSBox-style delay queue for smooth transitions
	PCSpeakerDelayEntry delay_entries[SPKR_DELAY_ENTRIES];
	uint32 delay_write_index;
	uint32 delay_read_index;
	double delay_base_index;        // Base time index for delay queue

	// Volume state
	float cur_vol;                  // Current output volume
	float target_vol;               // Target volume (for smooth transitions)

	// For WAV output (debug)
	NuvieIOFileWrite dataFile;
	uint32 wav_length;

	// Internal methods
	void AddDelayEntry(double index, float vol);
	void ForwardPIT(double amount);
	float GetVolume();

public:
	PCSpeaker(uint32 mixer_rate);
	~PCSpeaker() { }

	void SetOn();
	void SetOff();
	void SetFrequency(uint16 freq, float offset = 0.0f);
	void SetPITDivisor(uint32 divisor);  // DOSBox style: set PIT divisor directly

	void PCSPEAKER_CallBack(sint16 *stream, const uint32 len);
};

#endif /* __PCSpeaker_h__ */
