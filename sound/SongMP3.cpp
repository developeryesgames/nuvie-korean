/*
 *  SongMP3.cpp
 *  Nuvie
 *
 *  MP3/OGG/WAV music playback using dr_mp3 and stb_vorbis
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
#include "SongMP3.h"
#include "decoder/MP3AudioStream.h"
#include "mixer/audiostream.h"

SongMP3::SongMP3(Audio::Mixer *m)
    : mixer(m)
    , stream(nullptr)
{
}

SongMP3::~SongMP3() {
    if (stream) {
        mixer->stopHandle(handle);
        delete stream;
        stream = nullptr;
    }
}

bool SongMP3::Init(const char *filename) {
    if (filename == nullptr) {
        return false;
    }

    m_Filename = filename;

    stream = new MP3AudioStream();
    if (!stream->load(filename)) {
        DEBUG(0, LEVEL_WARNING, "SongMP3: Failed to load: %s\n", filename);
        delete stream;
        stream = nullptr;
        return false;
    }

    return true;
}

bool SongMP3::Play(bool looping) {
    if (stream) {
        stream->rewind();

        // Always use LoopingAudioStream - BGM should always loop
        Audio::LoopingAudioStream *looping_stream =
            new Audio::LoopingAudioStream((Audio::RewindableAudioStream *)stream, 0, DisposeAfterUse::NO);
        mixer->playStream(Audio::Mixer::kMusicSoundType, &handle, looping_stream,
                         -1, Audio::Mixer::kMaxChannelVolume, 0, DisposeAfterUse::YES);
        return true;
    }
    return false;
}

bool SongMP3::Stop() {
    if (stream) {
        mixer->stopHandle(handle);
        stream->rewind();
    }
    return true;
}

bool SongMP3::SetVolume(uint8 volume) {
    mixer->setChannelVolume(handle, volume);
    if (stream) {
        stream->setVolume(volume);
    }
    return true;
}
