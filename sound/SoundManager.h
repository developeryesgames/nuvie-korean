/*
 *  SoundManager.h
 *  Nuvie
 *
 *  Created by Adrian Boeing on Wed Jan 21 2004.
 *  Copyright (c) 2003. All rights reserved.
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

//notes: this file should be moved to the nuvie main directory instead of sound.

//priorities:
//todo:
//-sample loading partialy implemented, will do later (now is 21/01/04)
//-make songs fade in & out - add query/callback for end of song so that they can cycle
//-make samples sound from mapwindow
//-make samples fade in & out according to distance
//-try and use original .m files

#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H
#include "Sound.h"
#include "Song.h"
#include "nuvieDefs.h"
#include "Configuration.h"
#include "NuvieIOFile.h"
#include "sdl-mixer.h"
#include "sfx.h"

#define SFX_PLAY_ASYNC true
#define SFX_PLAY_SYNC false

// Music style options
enum MusicStyle {
    MUSIC_STYLE_NATIVE,   // Original AdLib/OPL
    MUSIC_STYLE_MT32,     // MT-32 emulation via Munt
    MUSIC_STYLE_MP3,      // MP3/OGG files via SDL_mixer
    MUSIC_STYLE_CUSTOM    // Custom audio files (legacy)
};

class SfxManager;
class CEmuopl;

typedef struct {
	SfxIdType sfx_id;
	Audio::SoundHandle handle;
} SoundManagerSfx;

class SoundManager {
public:
	SoundManager();
	~SoundManager();

	bool nuvieStartup(Configuration *config);
	bool initAudio();
	void update_map_sfx(); //updates the active sounds
    void update(); // at the moment this just changes songs if required

    void musicPlayFrom(string group);

    void musicPause();
    void musicPlay();
    void musicPlay(const char *filename, uint16 song_num=0);

    void musicStop(); // SB-X
    Audio::SoundHandle playTownsSound(std::string filename, uint16 sample_num);
    bool isSoundPLaying(Audio::SoundHandle handle);

    bool playSfx(uint16 sfx_id, bool async = false);

    // Musical instrument interface - plays note with instrument type
    // instrument_type: 0=Harp, 1=Harpsichord, 2=Lute, 3=Panpipes, 4=Xylophone
    // note: 0-9 (keyboard keys)
    bool playInstrumentNote(uint8 instrument_type, uint8 note);

    bool is_audio_enabled() { return audio_enabled; }
    void set_audio_enabled(bool val);
    bool is_music_enabled() { return music_enabled; }
    void set_music_enabled(bool val);
    bool is_speech_enabled() { return speech_enabled; }
    void set_speech_enabled(bool val);
    bool is_sfx_enabled() { return sfx_enabled; }
    void set_sfx_enabled(bool val) { sfx_enabled = val; }
    bool is_custom_sfx_enabled() { return custom_sfx_enabled; }
    void set_custom_sfx_enabled(bool val);
    uint8 get_sfx_volume() { return sfx_volume; }
    void set_sfx_volume(uint8 val) { sfx_volume = val; }
    uint8 get_music_volume() { return music_volume; }
    void set_music_volume(uint8 val) {music_volume = val; }
    Sound *get_m_pCurrentSong() { return m_pCurrentSong; }

    bool stop_music_on_group_change;

private:
	bool LoadCustomSongs(string scriptname);
    bool LoadNativeU6Songs();
#ifdef HAVE_MT32EMU
    bool LoadMT32U6Songs();
#endif
    bool LoadMP3U6Songs();
    bool loadSong(Song *song, const char *filename);
    bool loadSong(Song *song, const char *filename, const char *title);
    bool groupAddSong(const char *group, Song *song);

	//bool LoadObjectSamples(string sound_dir);
	//bool LoadTileSamples(string sound_dir);
	bool LoadSfxManager(string sfx_style);

	Sound* SongExists(string name); //have we loaded this sound before?
	Sound* SampleExists(string name); //have we loaded this sound before?


	Sound* RequestTileSound(int id);
	Sound* RequestObjectSound(int id);
	Sound* RequestSong(string group); //request a song from this group

	uint16 RequestObjectSfxId(uint16 obj_n);
	uint16 RequestTileSfxId(uint16 tile_num);

	map<int,SoundCollection *> m_TileSampleMap;
	map<int,SoundCollection *> m_ObjectSampleMap;
	map<string,SoundCollection *> m_MusicMap;
	list<Sound *> m_Songs;
	list<Sound *> m_Samples;
	Configuration *m_Config;

	//state info:
	string m_CurrentGroup;
	Sound *m_pCurrentSong;
	list<SoundManagerSfx> m_ActiveSounds;
    bool audio_enabled;
    bool music_enabled;
    bool speech_enabled;
    bool sfx_enabled;
    bool custom_sfx_enabled;

    uint8 music_volume;
    uint8 sfx_volume;

    SdlMixerManager *mixer;
    SfxManager *m_SfxManager;
    SfxManager *m_FallbackSfxManager;  // For hybrid custom+pcspeaker mode

    CEmuopl *opl;

    MusicStyle current_music_style;
    std::string mt32_rom_path;

    int game_type; //FIXME there's a nuvie_game_t, but almost everything uses int game_type (or gametype)

public:
	static bool g_MusicFinished;
};

#endif
