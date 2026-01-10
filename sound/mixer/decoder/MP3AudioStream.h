/*
 * MP3AudioStream.h
 * Nuvie
 *
 * MP3/OGG audio decoder stream using SDL_mixer for decoding only
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

#ifndef __MP3AudioStream_h__
#define __MP3AudioStream_h__

#include <string>
#include "SDL.h"
#include "audiostream.h"

// Forward declaration - we'll use SDL_RWops for file reading
// and decode MP3/OGG ourselves using stb_vorbis or dr_mp3

class MP3AudioStream : public Audio::RewindableAudioStream {
public:
    MP3AudioStream();
    ~MP3AudioStream();

    bool load(const std::string &filename);

    int readBuffer(sint16 *buffer, const int numSamples);
    bool isStereo() const { return _channels == 2; }
    int getRate() const { return _sampleRate; }
    bool rewind();
    bool endOfData() const { return _endOfData; }

    void setVolume(uint8_t volume) { _volume = volume; }

private:
    bool loadWav(const std::string &filename);
    bool loadOgg(const std::string &filename);
    bool loadMp3(const std::string &filename);

    sint16 *_audioData;
    uint32_t _audioLength;  // in samples (not bytes)
    uint32_t _position;
    int _sampleRate;
    int _channels;
    bool _endOfData;
    uint8_t _volume;
};

#endif /* __MP3AudioStream_h__ */
