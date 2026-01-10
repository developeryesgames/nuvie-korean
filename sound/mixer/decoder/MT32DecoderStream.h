/*
 * MT32DecoderStream.h
 * Nuvie
 *
 * Created for MT-32 emulation support.
 * Copyright (c) 2024 The Nuvie Team
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
 */

#ifndef __MT32DecoderStream_h__
#define __MT32DecoderStream_h__

#include <cstdio>
#include <string>
#include <vector>
#include "SDL.h"
#include "audiostream.h"

// MT32Emu includes
#include "mt32emu.h"

// MIDI event structure
struct MidiEvent {
    uint32_t delta_time;  // Delta time in ticks
    uint32_t abs_time;    // Absolute time in ticks
    std::vector<uint8_t> data;
};

// MIDI track structure
struct MidiTrack {
    std::vector<MidiEvent> events;
    size_t current_event;
    uint32_t current_tick;
};

class MT32DecoderStream : public Audio::RewindableAudioStream {
public:
    MT32DecoderStream();
    MT32DecoderStream(const std::string &rom_path, const std::string &midi_filename);
    ~MT32DecoderStream();

    bool init(const std::string &rom_path);
    bool loadMidi(const std::string &filename);

    int readBuffer(sint16 *buffer, const int numSamples);

    bool isStereo() const { return true; }
    int getRate() const { return _sampleRate; }

    bool rewind();
    bool endOfData() const { return _endOfData; }

    // Volume control (0-255)
    void setVolume(uint8_t volume);

private:
    bool loadRoms(const std::string &rom_path);
    bool parseMidiFile(const std::string &filename);
    void processNextEvents();
    uint32_t readVarLen(const uint8_t *data, size_t &pos);

    MT32Emu::Service _service;
    bool _initialized;
    bool _endOfData;
    int _sampleRate;

    // MIDI playback state
    std::vector<MidiTrack> _tracks;
    uint32_t _currentTick;
    uint32_t _ticksPerQuarter;
    uint32_t _microsecondsPerQuarter;
    double _samplesPerTick;
    double _sampleCounter;

    // ROM data
    uint8_t *_controlRom;
    uint8_t *_pcmRom;
    size_t _controlRomSize;
    size_t _pcmRomSize;

    float _gain;
};

#endif /* __MT32DecoderStream_h__ */
