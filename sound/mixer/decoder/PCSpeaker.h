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
	double index;       // Time index within a 1ms PIC tick (0.0 .. 1.0)
	float output_level; // -1.0, 0.0, +1.0 (speaker output level)
};

#define SPKR_DELAY_ENTRIES 8192

class PCSpeaker {
private:
	uint32 rate;                    // Output sample rate (22050)

	// PIT Timer state (DOSBox style)
	uint32 pit_divisor;             // PIT counter divisor (frequency = PIT_TICK_RATE / divisor)
	double pit_index;               // Current position in PIT cycle (milliseconds)
	double pit_max;                 // PIT cycle length in milliseconds (full cycle)
	double pit_half;                // Half cycle length in milliseconds (for Mode 3)
	double pit_new_max;             // Pending cycle length (applied on next toggle)
	double pit_new_half;            // Pending half-cycle length (applied on next toggle)
	bool pit_output_level;          // Current PIT output state (high/low)
	bool pit_mode3_counting;        // Mode 3 running state (DOSBox behavior)
	uint32 minimum_counter;         // Clamp high frequencies to avoid aliasing

	// Speaker gate state (port 61h bits)
	bool pit_output_enabled;        // Speaker output enabled (bit 1)
	bool pit_clock_gate_enabled;    // PIT clock gate enabled (bit 0)

	// DOSBox-style delay queue for smooth transitions
	PCSpeakerDelayEntry delay_entries[SPKR_DELAY_ENTRIES];
	uint32 delay_used;
	double last_index;              // Last PIC tick index (0.0 .. 1.0)
	bool last_output_level;         // For de-duplication of delay entries

	// Volume state (DOSBox-style)
	double volcur;
	double volwant;
	double spkr_speed;

	// Lowpass filter state (DOSBox-style multi-order RC filter)
	static const int LOWPASS_ORDER = 3;  // 3rd order like DOSBox-X
	double lowpass_state[LOWPASS_ORDER]; // Filter state per order
	double lowpass_alpha;                // Filter coefficient (16.16 fixed point style)

	// For WAV output (debug)
	NuvieIOFileWrite dataFile;
	uint32 wav_length;

	// Internal methods
	void AddDelayEntry(double index, float output_level);
	void ForwardPIT(double new_index);

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

