/*
 *  PCSpeakerSfxManager.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Wed Feb 9 2011.
 *  Copyright (c) 2011. All rights reserved.
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

#include <string>
#include <map>

#include "nuvieDefs.h"
#include "mixer.h"
#include "decoder/PCSpeakerStream.h"
#include "PCSpeakerSfxManager.h"

// Original U6 instrument frequency table (from decompiled seg_27a1.c:1533)
// Scale frequencies for notes 0-9
static const uint16 instrument_freq_table[10] = {
	0x1EAB,  // 7851 - note 0
	0x0C2C,  // 3116 - note 1
	0x0DA9,  // 3497 - note 2
	0x0F56,  // 3926 - note 3
	0x103F,  // 4159 - note 4
	0x123C,  // 4668 - note 5
	0x1478,  // 5240 - note 6
	0x16FA,  // 5882 - note 7
	0x1857,  // 6231 - note 8
	0x1B53   // 6995 - note 9
};


PCSpeakerSfxManager::PCSpeakerSfxManager(Configuration *cfg, Audio::Mixer *m) : SfxManager(cfg, m)
{

}

PCSpeakerSfxManager::~PCSpeakerSfxManager()
{

}

bool PCSpeakerSfxManager::playSfx(SfxIdType sfx_id, uint8 volume)
{
	return playSfxLooping(sfx_id, NULL, volume);
}


bool PCSpeakerSfxManager::playSfxLooping(SfxIdType sfx_id, Audio::SoundHandle *handle, uint8 volume)
{
	Audio::AudioStream *stream = NULL;

	if(sfx_id == NUVIE_SFX_BLOCKED)
	{
		stream = new PCSpeakerFreqStream(311, 0xa);
	}
	else if(sfx_id == NUVIE_SFX_SUCCESS)
	{
		stream = new PCSpeakerFreqStream(2000, 0xa);
	}
	else if(sfx_id == NUVIE_SFX_FAILURE)
	{
		stream = new PCSpeakerSweepFreqStream(800, 2000, 50, 1);
	}
	else if(sfx_id == NUVIE_SFX_ATTACK_SWING)
	{
		stream = new PCSpeakerSweepFreqStream(400, 750, 150, 5);
	}
	else if(sfx_id == NUVIE_SFX_RUBBER_DUCK)
	{
		stream = new PCSpeakerSweepFreqStream(5000, 8000, 50, 1);
	}
	else if(sfx_id == NUVIE_SFX_HIT)
	{
		stream = new PCSpeakerRandomStream(0x2710, 0x320, 1);
	}
	else if(sfx_id == NUVIE_SFX_BROKEN_GLASS)
	{
		stream = makePCSpeakerGlassSfxStream(mixer->getOutputRate());
	}
	else if(sfx_id == NUVIE_SFX_CORPSER_DRAGGED_UNDER)
	{
		stream = new PCSpeakerSweepFreqStream(1200, 2000, 40, 1);
	}
	else if(sfx_id == NUVIE_SFX_CORPSER_REGURGITATE)
	{
		stream = new PCSpeakerRandomStream(0x258, 0x1b58, 1);
	}
	else if(sfx_id >= NUVIE_SFX_CASTING_MAGIC_P1 && sfx_id <= NUVIE_SFX_CASTING_MAGIC_P1_8)
	{
		uint8 magic_circle = sfx_id - NUVIE_SFX_CASTING_MAGIC_P1 + 1;
		stream = makePCSpeakerMagicCastingP1SfxStream(mixer->getOutputRate(), magic_circle);
	}
	else if(sfx_id >= NUVIE_SFX_CASTING_MAGIC_P2 && sfx_id <= NUVIE_SFX_CASTING_MAGIC_P2_8)
	{
		uint8 magic_circle = sfx_id - NUVIE_SFX_CASTING_MAGIC_P2 + 1;
		stream = makePCSpeakerMagicCastingP2SfxStream(mixer->getOutputRate(), magic_circle);
	}
	else if(sfx_id == NUVIE_SFX_BELL)
	{
		stream = new PCSpeakerStutterStream(-1, 0x4e20, 0x3e80, 1, 0x7d0);
	}
	else if(sfx_id == NUVIE_SFX_AVATAR_DEATH)
	{
		stream = makePCSpeakerAvatarDeathSfxStream(mixer->getOutputRate());
	}
	else if(sfx_id == NUVIE_SFX_KAL_LOR)
	{
		stream = makePCSpeakerKalLorSfxStream(mixer->getOutputRate());
	}
	else if(sfx_id == NUVIE_SFX_SLUG_DISSOLVE)
	{
		stream = makePCSpeakerSlugDissolveSfxStream(mixer->getOutputRate());
	}
	else if(sfx_id == NUVIE_SFX_HAIL_STONE)
	{
		stream = makePCSpeakerHailStoneSfxStream(mixer->getOutputRate());
	}
  else if(sfx_id == NUVIE_SFX_EARTH_QUAKE)
  {
    stream = makePCSpeakerEarthQuakeSfxStream(mixer->getOutputRate());
  }
  // Musical Instruments (based on original U6 decompiled code seg_2F1A.c)
  // Using simple frequency streams since PCSpeakerStutterStream parameters
  // don't map directly to OSI_playWavedNote behavior
  else if(NUVIE_SFX_IS_INSTRUMENT(sfx_id))
  {
    uint8 instrument_type = NUVIE_SFX_INSTRUMENT_TYPE(sfx_id);
    uint8 note = NUVIE_SFX_INSTRUMENT_NOTE(sfx_id);
    uint16 counter = instrument_freq_table[note];

    // Convert PIT counter value to Hz: freq = 1193180 / counter
    // The counter values are inverted: larger counter = lower frequency
    uint32 hz = 1193180 / counter;

    DEBUG(0, LEVEL_INFORMATIONAL, "Instrument SFX: id=%d, type=%d, note=%d, counter=%d, hz=%d\n",
          sfx_id, instrument_type, note, counter, hz);

    switch(instrument_type)
    {
      case NUVIE_SFX_INSTRUMENT_HARP:
        // Original: case 2: OSI_playWavedNote(di >> 1, 1, 6000, 30000, -5)
        // Harp: lower frequency (counter >> 1 means double the counter = half freq), longer sustain
        stream = new PCSpeakerFreqStream(hz / 2, 300);
        break;

      case NUVIE_SFX_INSTRUMENT_HARPSICHORD:
        // Original: case 3: OSI_playWavedNote(di, 1, 4000, 15000, -3)
        // Harpsichord: bright attack, quick decay
        stream = new PCSpeakerFreqStream(hz, 200);
        break;

      case NUVIE_SFX_INSTRUMENT_LUTE:
        // Original: case 18: OSI_playWavedNote(di >> 1, 1, 2000, 20000, -20)
        // Lute: warm tone, medium decay
        stream = new PCSpeakerFreqStream(hz / 2, 250);
        break;

      case NUVIE_SFX_INSTRUMENT_PANPIPES:
        // Original: case 19: OSI_playNote(di >> 2, 200)
        // Panpipes: simple sustained tone (counter >> 2 = quarter freq)
        stream = new PCSpeakerFreqStream(hz / 4, 200);
        break;

      case NUVIE_SFX_INSTRUMENT_XYLOPHONE:
        // Original: case 20: OSI_playNote(di, 6) + OSI_playWavedNote(...)
        // Xylophone: sharp attack, very short
        stream = new PCSpeakerFreqStream(hz, 80);
        break;

      default:
        break;
    }
  }

	if(stream)
	{
		sfx_duration = stream->getLengthInMsec();
		playSoundSample(stream, handle, volume);
		return true;
	}

	return false;
}

void PCSpeakerSfxManager::playSoundSample(Audio::AudioStream *stream, Audio::SoundHandle *looping_handle, uint8 volume)
{
	Audio::SoundHandle handle;

	if(looping_handle)
	{
		Audio::LoopingAudioStream *looping_stream = new Audio::LoopingAudioStream((Audio::RewindableAudioStream *)stream, 0);
		mixer->playStream(Audio::Mixer::kPlainSoundType, looping_handle, looping_stream, -1, volume);
	}
	else
	{
		mixer->playStream(Audio::Mixer::kPlainSoundType, &handle, stream, -1, volume);
	}

}
