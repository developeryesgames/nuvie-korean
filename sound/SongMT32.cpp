/*
 *  SongMT32.cpp
 *  Nuvie
 *
 *  Created for MT-32 emulation support.
 *  Copyright (c) 2024 The Nuvie Team
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
#include "SongMT32.h"
#include "decoder/MT32DecoderStream.h"
#include <fstream>

SongMT32::SongMT32(Audio::Mixer *m, const std::string &rp)
    : mixer(m)
    , rom_path(rp)
    , stream(nullptr)
{
}

SongMT32::~SongMT32() {
    if (stream) {
        mixer->stopHandle(handle);
        delete stream;
    }
}

bool SongMT32::checkRomsExist(const std::string &rom_path) {
    // Check for CM-32L ROMs first
    std::string control_path = rom_path + "/CM32L_CONTROL.ROM";
    std::string pcm_path = rom_path + "/CM32L_PCM.ROM";

    std::ifstream control_test(control_path);
    std::ifstream pcm_test(pcm_path);

    if (control_test.is_open() && pcm_test.is_open()) {
        return true;
    }

    // Try MT-32 ROMs
    control_path = rom_path + "/MT32_CONTROL.ROM";
    pcm_path = rom_path + "/MT32_PCM.ROM";

    control_test.close();
    pcm_test.close();
    control_test.open(control_path);
    pcm_test.open(pcm_path);

    return control_test.is_open() && pcm_test.is_open();
}

bool SongMT32::Init(const char *filename, uint16 song_num) {
    if (filename == nullptr) {
        return false;
    }

    m_Filename = filename;

    stream = new MT32DecoderStream();
    if (!stream->init(rom_path)) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Failed to initialize emulator\n");
        delete stream;
        stream = nullptr;
        return false;
    }

    if (!stream->loadMidi(filename)) {
        DEBUG(0, LEVEL_ERROR, "MT-32: Failed to load MIDI file: %s\n", filename);
        delete stream;
        stream = nullptr;
        return false;
    }

    return true;
}

bool SongMT32::Play(bool looping) {
    if (stream) {
        mixer->playStream(Audio::Mixer::kMusicSoundType, &handle, stream,
                         -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::NO);
        return true;
    }
    return false;
}

bool SongMT32::Stop() {
    if (stream) {
        mixer->stopHandle(handle);
        stream->rewind();
    }
    return true;
}

bool SongMT32::SetVolume(uint8 volume) {
    mixer->setChannelVolume(handle, volume);
    if (stream) {
        stream->setVolume(volume);
    }
    return true;
}
