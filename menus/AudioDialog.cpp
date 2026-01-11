/*
 *  AudioDialog.cpp
 *  Nuvie
 *  Copyright (C) 2013 The Nuvie Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "SDL.h"
#include "nuvieDefs.h"

#include "GUI.h"
#include "GUI_types.h"
#include "GUI_button.h"
#include "GUI_text.h"
#include "GUI_TextToggleButton.h"
#include "GUI_CallBack.h"
#include "GUI_area.h"

#include "GUI_Dialog.h"
#include "AudioDialog.h"
#include "SoundManager.h"
#include "Configuration.h"
#include "Keys.h"
#include "Party.h"
#include "Converse.h"
#include "U6misc.h"
#include "FontManager.h"
#include "KoreanTranslation.h"
#include <math.h>

#define AD_WIDTH 292
#define AD_HEIGHT 166

static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled() && Game::get_game()->is_original_plus())
		return 3;
	return 1;
}

// Helper for translated text
static std::string get_ad_text(const char* english) {
	FontManager *fm = Game::get_game()->get_font_manager();
	KoreanTranslation *kt = Game::get_game()->get_korean_translation();
	if (fm && fm->is_korean_enabled() && kt) {
		std::string korean = kt->getUIText(english);
		if (!korean.empty()) return korean;
	}
	return std::string(english);
}

AudioDialog::AudioDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - AD_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - AD_HEIGHT * get_menu_scale())/2,
                       AD_WIDTH * get_menu_scale(), AD_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE) {
	callback_object = callback;
	init();
	grab_focus();
}

bool AudioDialog::init() {
	int scale = get_menu_scale();
	int height = 12 * scale;
	int colX[] = { 213*scale, 243*scale };
	int textX[] = { 9*scale, 9*scale, 9*scale };  // All labels at same X for Korean
	int buttonY = 9 * scale;
	int textY = 11 * scale;  // Changed to int to prevent overflow
	int row_h = 13 * scale;
	b_index_num = -1;
	last_index = 0;

	GUI_Widget *widget;
	GUI_Font * font = GUI::get_gui()->get_font();

	// Get translated texts
	std::string txt_audio = get_ad_text("Audio:");
	std::string txt_music = get_ad_text("Enable music:");
	std::string txt_musicvol = get_ad_text("Music volume:");
	std::string txt_combat = get_ad_text("Combat changes music:");
	std::string txt_vehicle = get_ad_text("Vehicle changes music:");
	std::string txt_converse = get_ad_text("Conversations stop music:");
	std::string txt_group = get_ad_text("Stop music on group change:");
	std::string txt_sfx = get_ad_text("Enable sfx:");
	std::string txt_sfxvol = get_ad_text("Sfx volume:");
	std::string txt_speech = get_ad_text("Enable speech:");
	std::string txt_cancel = get_ad_text("Cancel");
	std::string txt_save = get_ad_text("Save");

	widget = (GUI_Widget *) new GUI_Text(textX[0], textY, 0, 0, 0, txt_audio.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[1], textY += row_h, 0, 0, 0, txt_music.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[2], textY += row_h, 0, 0, 0, txt_musicvol.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[2], textY += row_h, 0, 0, 0, txt_combat.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[2], textY += row_h, 0, 0, 0, txt_vehicle.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[2], textY += row_h, 0, 0, 0, txt_converse.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[2], textY += row_h, 0, 0, 0, txt_group.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[1], textY += row_h, 0, 0, 0, txt_sfx.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(textX[2], textY += row_h, 0, 0, 0, txt_sfxvol.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
 	bool use_speech_b = (Game::get_game()->get_game_type() == NUVIE_GAME_U6 && has_fmtowns_support(Game::get_game()->get_config()));
	if(use_speech_b) {
		widget = (GUI_Widget *) new GUI_Text(textX[1], textY += row_h, 0, 0, 0, txt_speech.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
	}
	char musicBuff[5], sfxBuff[5];
	int sfxVol_selection, musicVol_selection, num_of_sfxVol, num_of_musicVol;
	SoundManager *sm = Game::get_game()->get_sound_manager();
	const char* const enabled_text[] = { "Disabled", "Enabled" };
	const char* const yes_no_text[] = { "no", "yes" };

	uint8 music_percent = round(sm->get_music_volume() / 2.55); // round needed for 10%, 30%, etc. 
	sprintf(musicBuff, "%u%%", music_percent);
	const char* const musicVol_text[] = { "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", musicBuff };

	if(music_percent % 10 == 0) {
		num_of_musicVol = 11;
		musicVol_selection = music_percent/10;
	} else {
		num_of_musicVol = 12;
		musicVol_selection = 11;
	}

	uint8 sfx_percent = round(sm->get_sfx_volume() / 2.55); // round needed for 10%, 30%, etc.
	sprintf(sfxBuff, "%u%%", sfx_percent);
	const char* const sfxVol_text[] = { "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%", sfxBuff };

	if(sfx_percent % 10 == 0) {
		num_of_sfxVol = 11;
		sfxVol_selection = sfx_percent/10;
	} else {
		num_of_sfxVol = 12;
		sfxVol_selection = 11;
	}

	audio_button = new GUI_TextToggleButton(this, colX[0], buttonY, 70*scale, height, enabled_text, 2, sm->is_audio_enabled(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { audio_button->SetTextScale(scale); audio_button->ChangeTextButton(-1,-1,-1,-1,audio_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(audio_button);
	button_index[last_index] = audio_button;

	music_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, sm->is_music_enabled(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { music_button->SetTextScale(scale); music_button->ChangeTextButton(-1,-1,-1,-1,music_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(music_button);
	button_index[last_index += 1] = music_button;

	musicVol_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, musicVol_text, num_of_musicVol, musicVol_selection, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { musicVol_button->SetTextScale(scale); musicVol_button->ChangeTextButton(-1,-1,-1,-1,musicVol_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(musicVol_button);
	button_index[last_index += 1] = musicVol_button;

	Party *party = Game::get_game()->get_party();
	combat_b = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, party->combat_changes_music, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { combat_b->SetTextScale(scale); combat_b->ChangeTextButton(-1,-1,-1,-1,combat_b->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(combat_b);
	button_index[last_index += 1] = combat_b;

	vehicle_b = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, party->vehicles_change_music, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { vehicle_b->SetTextScale(scale); vehicle_b->ChangeTextButton(-1,-1,-1,-1,vehicle_b->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(vehicle_b);
	button_index[last_index += 1] = vehicle_b;

	bool stop_converse = Game::get_game()->get_converse()->conversations_stop_music;
	converse_b = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, stop_converse, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { converse_b->SetTextScale(scale); converse_b->ChangeTextButton(-1,-1,-1,-1,converse_b->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(converse_b);
	button_index[last_index += 1] = converse_b;

	group_b = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, sm->stop_music_on_group_change, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { group_b->SetTextScale(scale); group_b->ChangeTextButton(-1,-1,-1,-1,group_b->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(group_b);
	button_index[last_index += 1] = group_b;

	sfx_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, sm->is_sfx_enabled(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { sfx_button->SetTextScale(scale); sfx_button->ChangeTextButton(-1,-1,-1,-1,sfx_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(sfx_button);
	button_index[last_index += 1] = sfx_button;

	sfxVol_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, sfxVol_text, num_of_sfxVol, sfxVol_selection, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { sfxVol_button->SetTextScale(scale); sfxVol_button->ChangeTextButton(-1,-1,-1,-1,sfxVol_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(sfxVol_button);
	button_index[last_index += 1] = sfxVol_button;

	if(use_speech_b) {
		speech_b = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, 40*scale, height, yes_no_text, 2, sm->is_speech_enabled(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { speech_b->SetTextScale(scale); speech_b->ChangeTextButton(-1,-1,-1,-1,speech_b->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(speech_b);
		button_index[last_index += 1] = speech_b;
	} else
		speech_b = NULL;
	cancel_button = new GUI_Button(this, 80*scale, AD_HEIGHT*scale - 20*scale, 54*scale, height, txt_cancel.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { cancel_button->SetTextScale(scale); cancel_button->ChangeTextButton(-1,-1,-1,-1,txt_cancel.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cancel_button);
	button_index[last_index += 1] = cancel_button;

	save_button = new GUI_Button(this, 151*scale, AD_HEIGHT*scale - 20*scale, 60*scale, height, txt_save.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { save_button->SetTextScale(scale); save_button->ChangeTextButton(-1,-1,-1,-1,txt_save.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(save_button);
	button_index[last_index += 1] = save_button;
 
 return true;
}

AudioDialog::~AudioDialog() {
}

GUI_status AudioDialog::close_dialog() {
	Delete(); // mark dialog as deleted. it will be freed by the GUI object
	callback_object->callback(0, this, this);
	return GUI_YUM;
}

GUI_status AudioDialog::KeyDown(SDL_Keysym key) {
	KeyBinder *keybinder = Game::get_game()->get_keybinder();
	ActionType a = keybinder->get_ActionType(key);

	switch(keybinder->GetActionKeyType(a))
	{
		case NORTH_KEY:
		case WEST_KEY:
			if(b_index_num != -1)
				button_index[b_index_num]->set_highlighted(false);

			if(b_index_num <= 0)
				b_index_num = last_index;
			else
				b_index_num = b_index_num - 1;
			button_index[b_index_num]->set_highlighted(true); break;
		case SOUTH_KEY:
		case EAST_KEY:
			if(b_index_num != -1)
				button_index[b_index_num]->set_highlighted(false);

			if(b_index_num == last_index)
				b_index_num = 0;
			else
				b_index_num += 1;
			button_index[b_index_num]->set_highlighted(true); break;
		case DO_ACTION_KEY: if(b_index_num != -1) return button_index[b_index_num]->Activate_button(); break;
		case CANCEL_ACTION_KEY: return close_dialog();
		default: keybinder->handle_always_available_keys(a); break;
	}
	return GUI_YUM;
}

GUI_status AudioDialog::callback(uint16 msg, GUI_CallBack *caller, void *data) {
	if(caller == (GUI_CallBack *)cancel_button) {
		return close_dialog();
	} else if(caller == (GUI_CallBack *)save_button) {
		Configuration *config = Game::get_game()->get_config();
		SoundManager *sm = Game::get_game()->get_sound_manager();

		int music_selection = musicVol_button->GetSelection();
		if(music_selection != 11) {
			uint8 musicVol = music_selection * 25.5;
			sm->set_music_volume(musicVol);
			if(sm->get_m_pCurrentSong() != NULL)
				sm->get_m_pCurrentSong()->SetVolume(musicVol);
			config->set("config/audio/music_volume", musicVol);
		}

		int sfx_selection = sfxVol_button->GetSelection();
		if(sfx_selection != 11) {
			uint8 sfxVol = sfx_selection * 25.5;
			sm->set_sfx_volume(sfxVol);
// probably need to update sfx volume if we have background sfx implemented
			config->set("config/audio/sfx_volume", sfxVol);
		}

		if(music_button->GetSelection() != sm->is_music_enabled())
			sm->set_music_enabled(music_button->GetSelection());
		config->set("config/audio/enable_music", music_button->GetSelection() ? "yes" : "no");
		if(sfx_button->GetSelection() != sm->is_sfx_enabled())
			sm->set_sfx_enabled(sfx_button->GetSelection());

		Party *party = Game::get_game()->get_party();
		party->combat_changes_music = combat_b->GetSelection();
		config->set("config/audio/combat_changes_music", combat_b->GetSelection() ? "yes" : "no");

		party->vehicles_change_music = vehicle_b->GetSelection();
		config->set("config/audio/vehicles_change_music", vehicle_b->GetSelection() ? "yes" : "no");

		Game::get_game()->get_converse()->conversations_stop_music = converse_b->GetSelection();
		config->set("config/audio/conversations_stop_music", converse_b->GetSelection() ? "yes" : "no");

		sm->stop_music_on_group_change = group_b->GetSelection();
		config->set("config/audio/stop_music_on_group_change", group_b->GetSelection() ? "yes" : "no");

		config->set("config/audio/enable_sfx", sfx_button->GetSelection() ? "yes" : "no");
		if(audio_button->GetSelection() != sm->is_audio_enabled())
			sm->set_audio_enabled(audio_button->GetSelection());
		config->set("config/audio/enabled", audio_button->GetSelection() ? "yes" : "no");

		if(speech_b) {
			bool speech_enabled = speech_b->GetSelection() ? true : false;
			config->set("config/ultima6/enable_speech", speech_b->GetSelection() ? "yes" : "no");
			if(speech_enabled != sm->is_speech_enabled())
				sm->set_speech_enabled(speech_enabled);
		}
		config->write();
		return close_dialog();
	}

	return GUI_PASS;
}
