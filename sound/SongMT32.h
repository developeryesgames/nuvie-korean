/*
 *  SongMT32.h
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

#ifndef __SongMT32_h__
#define __SongMT32_h__

#include "mixer.h"
#include "Song.h"

class MT32DecoderStream;

class SongMT32 : public Song {
public:
    SongMT32(Audio::Mixer *m, const std::string &rom_path);
    ~SongMT32();

    bool Init(const char *filename) { return Init(filename, 0); }
    bool Init(const char *filename, uint16 song_num);
    bool Play(bool looping = true);
    bool Stop();
    bool SetVolume(uint8 volume);
    bool FadeOut(float seconds) { return false; }

    static bool checkRomsExist(const std::string &rom_path);

private:
    Audio::Mixer *mixer;
    std::string rom_path;
    MT32DecoderStream *stream;
    Audio::SoundHandle handle;
};

#endif /* __SongMT32_h__ */
