/*
 *  SongMP3.h
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

#ifndef __SongMP3_h__
#define __SongMP3_h__

#include "Song.h"
#include "mixer.h"

class MP3AudioStream;

class SongMP3 : public Song {
public:
    SongMP3(Audio::Mixer *m);
    ~SongMP3();

    bool Init(const char *filename);
    bool Init(const char *filename, uint16 song_num) { return Init(filename); }
    bool Play(bool looping = true);
    bool Stop();
    bool SetVolume(uint8 volume);
    bool FadeOut(float seconds) { return Stop(); }

private:
    Audio::Mixer *mixer;
    MP3AudioStream *stream;
    Audio::SoundHandle handle;
};

#endif /* __SongMP3_h__ */
