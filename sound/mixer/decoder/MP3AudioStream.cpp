/*
 * MP3AudioStream.cpp
 * Nuvie
 *
 * MP3/OGG audio decoder stream using dr_mp3 and stb_vorbis
 * Copyright (c) 2024 The Nuvie Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "nuvieDefs.h"
#include "MP3AudioStream.h"
#include <cstring>
#include <algorithm>

// Implementation defines must come before includes
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

// stb_vorbis - include implementation
#include "stb_vorbis.c"

MP3AudioStream::MP3AudioStream()
    : _audioData(nullptr)
    , _audioLength(0)
    , _position(0)
    , _sampleRate(22050)
    , _channels(2)
    , _endOfData(false)
    , _volume(255)
{
}

MP3AudioStream::~MP3AudioStream() {
    if (_audioData) {
        free(_audioData);
        _audioData = nullptr;
    }
}

bool MP3AudioStream::load(const std::string &filename) {
    DEBUG(0, LEVEL_INFORMATIONAL, "MP3AudioStream: Attempting to load: %s\n", filename.c_str());

    // Check file extension
    std::string ext;
    size_t dot = filename.rfind('.');
    if (dot != std::string::npos) {
        ext = filename.substr(dot);
        // Convert to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    bool result = false;
    if (ext == ".mp3") {
        result = loadMp3(filename);
    } else if (ext == ".ogg") {
        result = loadOgg(filename);
    } else if (ext == ".wav") {
        result = loadWav(filename);
    } else {
        // Try MP3 as default
        result = loadMp3(filename);
    }

    if (!result) {
        DEBUG(0, LEVEL_WARNING, "MP3AudioStream: Failed to load: %s\n", filename.c_str());
    }
    return result;
}

bool MP3AudioStream::loadMp3(const std::string &filename) {
    drmp3_config config;
    drmp3_uint64 totalPCMFrameCount;

    // Decode entire MP3 to memory
    drmp3_int16 *pSampleData = drmp3_open_file_and_read_pcm_frames_s16(
        filename.c_str(),
        &config,
        &totalPCMFrameCount,
        NULL
    );

    if (!pSampleData) {
        DEBUG(0, LEVEL_WARNING, "MP3AudioStream: Failed to load MP3: %s\n", filename.c_str());
        return false;
    }

    _sampleRate = config.sampleRate;
    _channels = config.channels;
    _audioLength = (uint32_t)(totalPCMFrameCount * _channels);
    _audioData = (sint16 *)pSampleData;
    _position = 0;
    _endOfData = false;

    DEBUG(0, LEVEL_INFORMATIONAL, "MP3AudioStream: Loaded %s (%d Hz, %d ch, %u samples)\n",
          filename.c_str(), _sampleRate, _channels, _audioLength);

    return true;
}

bool MP3AudioStream::loadOgg(const std::string &filename) {
    int channels, sampleRate;
    short *output;

    int samples = stb_vorbis_decode_filename(filename.c_str(), &channels, &sampleRate, &output);

    if (samples <= 0) {
        DEBUG(0, LEVEL_WARNING, "MP3AudioStream: Failed to load OGG: %s\n", filename.c_str());
        return false;
    }

    _sampleRate = sampleRate;
    _channels = channels;
    _audioLength = samples * channels;
    _audioData = (sint16 *)output;
    _position = 0;
    _endOfData = false;

    DEBUG(0, LEVEL_INFORMATIONAL, "MP3AudioStream: Loaded %s (%d Hz, %d ch, %u samples)\n",
          filename.c_str(), _sampleRate, _channels, _audioLength);

    return true;
}

bool MP3AudioStream::loadWav(const std::string &filename) {
    // Use SDL_LoadWAV for WAV files
    SDL_AudioSpec spec;
    Uint8 *wavBuffer;
    Uint32 wavLength;

    if (!SDL_LoadWAV(filename.c_str(), &spec, &wavBuffer, &wavLength)) {
        DEBUG(0, LEVEL_WARNING, "MP3AudioStream: Failed to load WAV: %s (%s)\n",
              filename.c_str(), SDL_GetError());
        return false;
    }

    // Convert to 16-bit stereo if needed
    _sampleRate = spec.freq;
    _channels = spec.channels;

    if (spec.format == AUDIO_S16LSB || spec.format == AUDIO_S16MSB) {
        _audioLength = wavLength / sizeof(sint16);
        _audioData = (sint16 *)malloc(wavLength);
        memcpy(_audioData, wavBuffer, wavLength);
    } else {
        // Need to convert - for simplicity, just fail
        DEBUG(0, LEVEL_WARNING, "MP3AudioStream: Unsupported WAV format\n");
        SDL_FreeWAV(wavBuffer);
        return false;
    }

    SDL_FreeWAV(wavBuffer);
    _position = 0;
    _endOfData = false;

    DEBUG(0, LEVEL_INFORMATIONAL, "MP3AudioStream: Loaded %s (%d Hz, %d ch, %u samples)\n",
          filename.c_str(), _sampleRate, _channels, _audioLength);

    return true;
}

int MP3AudioStream::readBuffer(sint16 *buffer, const int numSamples) {
    if (!_audioData || _endOfData) {
        return 0;  // Return 0 to signal EOF for LoopingAudioStream
    }

    int samplesRemaining = _audioLength - _position;
    int samplesToCopy = std::min(numSamples, samplesRemaining);

    if (samplesToCopy > 0) {
        // Apply volume
        if (_volume == 255) {
            memcpy(buffer, _audioData + _position, samplesToCopy * sizeof(sint16));
        } else {
            for (int i = 0; i < samplesToCopy; i++) {
                int sample = _audioData[_position + i];
                sample = (sample * _volume) / 255;
                buffer[i] = (sint16)sample;
            }
        }
        _position += samplesToCopy;
    }

    // Mark end of data when we've read everything
    if (_position >= _audioLength) {
        _endOfData = true;
    }

    // Return actual samples read (LoopingAudioStream uses this to detect EOF)
    return samplesToCopy;
}

bool MP3AudioStream::rewind() {
    _position = 0;
    _endOfData = false;
    return true;
}
