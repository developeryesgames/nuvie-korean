/*
 * MT32DecoderStream.cpp
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

#include "nuvieDefs.h"
#include "MT32DecoderStream.h"
#include <cstring>
#include <fstream>

// Simple report handler for MT32Emu
class NuvieReportHandler : public MT32Emu::IReportHandler {
public:
    void printDebug(const char *fmt, va_list list) override {
        // Optional: print debug messages
    }
    void onErrorControlROM() override {
        DEBUG(0, LEVEL_ERROR, "MT-32: Control ROM error\n");
    }
    void onErrorPCMROM() override {
        DEBUG(0, LEVEL_ERROR, "MT-32: PCM ROM error\n");
    }
    void showLCDMessage(const char *message) override {
        DEBUG(0, LEVEL_INFORMATIONAL, "MT-32 LCD: %s\n", message);
    }
    void onMIDIMessagePlayed() override {}
    bool onMIDIQueueOverflow() override { return false; }
    void onMIDISystemRealtime(MT32Emu::Bit8u) override {}
    void onDeviceReset() override {}
    void onDeviceReconfig() override {}
    void onNewReverbMode(MT32Emu::Bit8u) override {}
    void onNewReverbTime(MT32Emu::Bit8u) override {}
    void onNewReverbLevel(MT32Emu::Bit8u) override {}
    void onPolyStateChanged(MT32Emu::Bit8u) override {}
    void onProgramChanged(MT32Emu::Bit8u, const char*, const char*) override {}
};

static NuvieReportHandler g_reportHandler;

MT32DecoderStream::MT32DecoderStream()
    : _initialized(false)
    , _endOfData(false)
    , _sampleRate(32000)
    , _currentTick(0)
    , _ticksPerQuarter(120)
    , _microsecondsPerQuarter(500000)  // 120 BPM default
    , _samplesPerTick(0)
    , _sampleCounter(0)
    , _controlRom(nullptr)
    , _pcmRom(nullptr)
    , _controlRomSize(0)
    , _pcmRomSize(0)
    , _gain(1.0f)
{
}

MT32DecoderStream::MT32DecoderStream(const std::string &rom_path, const std::string &midi_filename)
    : MT32DecoderStream()
{
    if (init(rom_path)) {
        loadMidi(midi_filename);
    }
}

MT32DecoderStream::~MT32DecoderStream() {
    if (_initialized) {
        _service.closeSynth();
    }
    delete[] _controlRom;
    delete[] _pcmRom;
}

bool MT32DecoderStream::init(const std::string &rom_path) {
    if (_initialized) {
        return true;
    }

    if (!loadRoms(rom_path)) {
        return false;
    }

    _service.createContext(g_reportHandler);

    // Add ROM data
    if (_service.addROMData(_controlRom, _controlRomSize) != MT32EMU_RC_ADDED_CONTROL_ROM) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Failed to add control ROM\n");
        return false;
    }

    if (_service.addROMData(_pcmRom, _pcmRomSize) != MT32EMU_RC_ADDED_PCM_ROM) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Failed to add PCM ROM\n");
        return false;
    }

    // Open synth
    if (_service.openSynth() != MT32EMU_RC_OK) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Failed to open synth\n");
        return false;
    }

    _sampleRate = _service.getActualStereoOutputSamplerate();
    _service.setOutputGain(_gain);
    _service.setReverbOutputGain(_gain);
    _service.setMIDIDelayMode(MT32Emu::MIDIDelayMode_IMMEDIATE);

    _initialized = true;
    DEBUG(0, LEVEL_INFORMATIONAL, "MT-32 emulator initialized at %d Hz\n", _sampleRate);

    return true;
}

bool MT32DecoderStream::loadRoms(const std::string &rom_path) {
    // Try CM-32L ROMs first, then MT-32
    std::string control_path, pcm_path;

    // Try CM-32L
    control_path = rom_path + "/CM32L_CONTROL.ROM";
    pcm_path = rom_path + "/CM32L_PCM.ROM";

    std::ifstream control_file(control_path, std::ios::binary | std::ios::ate);
    if (!control_file.is_open()) {
        // Try MT-32
        control_path = rom_path + "/MT32_CONTROL.ROM";
        pcm_path = rom_path + "/MT32_PCM.ROM";
        control_file.open(control_path, std::ios::binary | std::ios::ate);
    }

    if (!control_file.is_open()) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Cannot find ROM files in %s\n", rom_path.c_str());
        DEBUG(0, LEVEL_ERROR, "Please place MT32_CONTROL.ROM and MT32_PCM.ROM (or CM32L versions) in the ROM directory.\n");
        return false;
    }

    _controlRomSize = control_file.tellg();
    control_file.seekg(0, std::ios::beg);
    _controlRom = new uint8_t[_controlRomSize];
    control_file.read(reinterpret_cast<char*>(_controlRom), _controlRomSize);
    control_file.close();

    std::ifstream pcm_file(pcm_path, std::ios::binary | std::ios::ate);
    if (!pcm_file.is_open()) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Cannot open PCM ROM: %s\n", pcm_path.c_str());
        return false;
    }

    _pcmRomSize = pcm_file.tellg();
    pcm_file.seekg(0, std::ios::beg);
    _pcmRom = new uint8_t[_pcmRomSize];
    pcm_file.read(reinterpret_cast<char*>(_pcmRom), _pcmRomSize);
    pcm_file.close();

    DEBUG(0, LEVEL_INFORMATIONAL, "MT-32: Loaded ROMs from %s\n", rom_path.c_str());
    return true;
}

bool MT32DecoderStream::loadMidi(const std::string &filename) {
    return parseMidiFile(filename);
}

uint32_t MT32DecoderStream::readVarLen(const uint8_t *data, size_t &pos) {
    uint32_t value = 0;
    uint8_t byte;
    do {
        byte = data[pos++];
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);
    return value;
}

bool MT32DecoderStream::parseMidiFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Cannot open MIDI file: %s\n", filename.c_str());
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    // Parse MIDI header
    if (fileSize < 14 || memcmp(data.data(), "MThd", 4) != 0) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Invalid MIDI file header\n");
        return false;
    }

    size_t pos = 8;
    uint16_t format = (data[pos] << 8) | data[pos + 1]; pos += 2;
    uint16_t numTracks = (data[pos] << 8) | data[pos + 1]; pos += 2;
    uint16_t division = (data[pos] << 8) | data[pos + 1]; pos += 2;

    if (division & 0x8000) {
        // SMPTE timing - not commonly used
        DEBUG(0, LEVEL_WARNING, "MT-32: SMPTE timing not fully supported\n");
        _ticksPerQuarter = 120;
    } else {
        _ticksPerQuarter = division;
    }

    _tracks.clear();
    _tracks.resize(numTracks);

    // Parse each track
    for (uint16_t t = 0; t < numTracks; t++) {
        if (pos + 8 > fileSize || memcmp(&data[pos], "MTrk", 4) != 0) {
            DEBUG(0, LEVEL_ERROR, "MT-32: Invalid track header\n");
            return false;
        }
        pos += 4;

        uint32_t trackLen = (data[pos] << 24) | (data[pos + 1] << 16) |
                           (data[pos + 2] << 8) | data[pos + 3];
        pos += 4;

        size_t trackEnd = pos + trackLen;
        uint32_t absTime = 0;
        uint8_t runningStatus = 0;

        while (pos < trackEnd) {
            uint32_t deltaTime = readVarLen(data.data(), pos);
            absTime += deltaTime;

            uint8_t status = data[pos];
            if (status < 0x80) {
                // Running status
                status = runningStatus;
            } else {
                pos++;
                if (status < 0xF0) {
                    runningStatus = status;
                }
            }

            MidiEvent event;
            event.delta_time = deltaTime;
            event.abs_time = absTime;

            if (status >= 0x80 && status < 0xF0) {
                // Channel message
                event.data.push_back(status);
                uint8_t msgType = status & 0xF0;
                if (msgType == 0xC0 || msgType == 0xD0) {
                    // Program change, channel pressure - 1 data byte
                    event.data.push_back(data[pos++]);
                } else {
                    // Other channel messages - 2 data bytes
                    event.data.push_back(data[pos++]);
                    event.data.push_back(data[pos++]);
                }
                _tracks[t].events.push_back(event);
            } else if (status == 0xF0 || status == 0xF7) {
                // SysEx
                uint32_t len = readVarLen(data.data(), pos);
                event.data.push_back(0xF0);
                for (uint32_t i = 0; i < len; i++) {
                    event.data.push_back(data[pos++]);
                }
                _tracks[t].events.push_back(event);
            } else if (status == 0xFF) {
                // Meta event
                uint8_t metaType = data[pos++];
                uint32_t len = readVarLen(data.data(), pos);

                if (metaType == 0x51 && len == 3) {
                    // Tempo change
                    _microsecondsPerQuarter = (data[pos] << 16) | (data[pos + 1] << 8) | data[pos + 2];
                }
                pos += len;
            }
        }

        _tracks[t].current_event = 0;
        _tracks[t].current_tick = 0;
    }

    // Calculate samples per tick
    _samplesPerTick = (_sampleRate * _microsecondsPerQuarter) / (_ticksPerQuarter * 1000000.0);

    _currentTick = 0;
    _sampleCounter = 0;
    _endOfData = false;

    DEBUG(0, LEVEL_INFORMATIONAL, "MT-32: Loaded MIDI with %d tracks, %d ticks/quarter\n",
          numTracks, _ticksPerQuarter);

    return true;
}

void MT32DecoderStream::processNextEvents() {
    for (auto &track : _tracks) {
        while (track.current_event < track.events.size()) {
            const MidiEvent &event = track.events[track.current_event];
            if (event.abs_time > _currentTick) {
                break;
            }

            // Send to MT-32
            if (!event.data.empty()) {
                if (event.data[0] == 0xF0) {
                    // SysEx
                    _service.playSysex(event.data.data(), event.data.size());
                } else if (event.data.size() >= 2) {
                    // Channel message
                    uint32_t msg = event.data[0];
                    msg |= (event.data[1] << 8);
                    if (event.data.size() >= 3) {
                        msg |= (event.data[2] << 16);
                    }
                    _service.playMsg(msg);
                }
            }

            track.current_event++;
        }
    }
}

int MT32DecoderStream::readBuffer(sint16 *buffer, const int numSamples) {
    if (!_initialized || _endOfData) {
        memset(buffer, 0, numSamples * sizeof(sint16));
        return numSamples;
    }

    int samplesGenerated = 0;
    int stereoSamples = numSamples / 2;  // MT-32 renders stereo pairs

    while (samplesGenerated < stereoSamples) {
        // Process MIDI events for current tick
        processNextEvents();

        // Calculate how many samples until next tick
        double nextTickSample = (_currentTick + 1) * _samplesPerTick;
        int samplesToRender = (int)(nextTickSample - _sampleCounter);

        if (samplesToRender <= 0) {
            samplesToRender = 1;
        }

        int remaining = stereoSamples - samplesGenerated;
        if (samplesToRender > remaining) {
            samplesToRender = remaining;
        }

        // Render audio
        _service.renderBit16s(buffer + samplesGenerated * 2, samplesToRender);

        samplesGenerated += samplesToRender;
        _sampleCounter += samplesToRender;

        // Advance tick counter
        while (_sampleCounter >= (_currentTick + 1) * _samplesPerTick) {
            _currentTick++;
        }

        // Check if all tracks finished
        bool allDone = true;
        for (const auto &track : _tracks) {
            if (track.current_event < track.events.size()) {
                allDone = false;
                break;
            }
        }

        if (allDone && !_service.isActive()) {
            _endOfData = true;
            break;
        }
    }

    return numSamples;
}

bool MT32DecoderStream::rewind() {
    _currentTick = 0;
    _sampleCounter = 0;
    _endOfData = false;

    for (auto &track : _tracks) {
        track.current_event = 0;
        track.current_tick = 0;
    }

    if (_initialized) {
        // Reset MT-32 synth (send all notes off, etc.)
        for (int ch = 0; ch < 16; ch++) {
            _service.playMsg(0xB0 | ch | (123 << 8));  // All Notes Off
            _service.playMsg(0xB0 | ch | (121 << 8));  // Reset All Controllers
        }
    }

    return true;
}

void MT32DecoderStream::setVolume(uint8_t volume) {
    _gain = volume / 255.0f;
    if (_initialized) {
        _service.setOutputGain(_gain);
        _service.setReverbOutputGain(_gain);
    }
}
