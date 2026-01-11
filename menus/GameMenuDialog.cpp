/*
 *  GameMenuDialog.cpp
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

#include "nuvieDefs.h"

#include "GUI.h"
#include "GUI_types.h"
#include "GUI_button.h"
#include "GUI_CallBack.h"
#include "GUI_area.h"

#include "GUI_Dialog.h"
#include "GameMenuDialog.h"
#include "VideoDialog.h"
#include "AudioDialog.h"
#include "GameplayDialog.h"
#include "InputDialog.h"
#include "CheatsDialog.h"
#include "Event.h"
#include "Keys.h"
#include "FontManager.h"
#include "KoreanTranslation.h"

#define GMD_WIDTH 150

#ifdef HAVE_JOYSTICK_SUPPORT
#include "JoystickDialog.h"

#define GMD_HEIGHT 135 // 13 bigger
#else
#define GMD_HEIGHT 122
#endif

// Get scale factor for Korean mode
static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled() && Game::get_game()->is_original_plus())
		return 3;  // 3x scale for Korean mode
	return 1;
}

// Helper for translated menu text - returns std::string to avoid pointer issues
static std::string get_menu_text(const char* english) {
	FontManager *fm = Game::get_game()->get_font_manager();
	KoreanTranslation *kt = Game::get_game()->get_korean_translation();
	if (fm && fm->is_korean_enabled() && kt) {
		std::string korean = kt->getUIText(english);
		if (!korean.empty()) return korean;
	}
	return std::string(english);
}

GameMenuDialog::GameMenuDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - GMD_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - GMD_HEIGHT * get_menu_scale())/2,
                       GMD_WIDTH * get_menu_scale(), GMD_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE) {
	callback_object = callback;
	init();
	grab_focus();
}

bool GameMenuDialog::init() {
	int scale = get_menu_scale();
	int width = 132 * scale;
	int height = 12 * scale;
	int buttonX = 9 * scale;
	int buttonY = 9 * scale;
	int row_h = 13 * scale;
	b_index_num = -1;
	last_index = 0;
	GUI *gui = GUI::get_gui();

	// Store translated texts locally to avoid pointer issues
	std::string txt_saveload = get_menu_text("Load/Save Game");
	std::string txt_video = get_menu_text("Video Options");
	std::string txt_audio = get_menu_text("Audio Options");
	std::string txt_input = get_menu_text("Input Options");
	std::string txt_joystick = get_menu_text("Joystick Options");
	std::string txt_gameplay = get_menu_text("Gameplay Options");
	std::string txt_cheats = get_menu_text("Cheats");
	std::string txt_back = get_menu_text("Back to Game");
	std::string txt_quit = get_menu_text("Quit");

	saveLoad_button = new GUI_Button(this, buttonX, buttonY, width, height, txt_saveload.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { saveLoad_button->SetTextScale(scale); saveLoad_button->ChangeTextButton(-1,-1,-1,-1,txt_saveload.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(saveLoad_button);
	button_index[last_index] = saveLoad_button;
	video_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_video.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { video_button->SetTextScale(scale); video_button->ChangeTextButton(-1,-1,-1,-1,txt_video.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(video_button);
	button_index[last_index+=1] = video_button;
	audio_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_audio.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { audio_button->SetTextScale(scale); audio_button->ChangeTextButton(-1,-1,-1,-1,txt_audio.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(audio_button);
	button_index[last_index+=1] = audio_button;
	input_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_input.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { input_button->SetTextScale(scale); input_button->ChangeTextButton(-1,-1,-1,-1,txt_input.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(input_button);
	button_index[last_index+=1] = input_button;
#ifdef HAVE_JOYSTICK_SUPPORT
	joystick_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_joystick.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { joystick_button->SetTextScale(scale); joystick_button->ChangeTextButton(-1,-1,-1,-1,txt_joystick.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(joystick_button);
	button_index[last_index+=1] = joystick_button;
#endif
	gameplay_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_gameplay.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { gameplay_button->SetTextScale(scale); gameplay_button->ChangeTextButton(-1,-1,-1,-1,txt_gameplay.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(gameplay_button);
	button_index[last_index+=1] = gameplay_button;
	cheats_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_cheats.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { cheats_button->SetTextScale(scale); cheats_button->ChangeTextButton(-1,-1,-1,-1,txt_cheats.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cheats_button);
	button_index[last_index+=1] = cheats_button;
	continue_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_back.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { continue_button->SetTextScale(scale); continue_button->ChangeTextButton(-1,-1,-1,-1,txt_back.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(continue_button);
	button_index[last_index += 1] = cheats_button;
	quit_button = new GUI_Button(this, buttonX, buttonY += row_h, width, height, txt_quit.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { quit_button->SetTextScale(scale); quit_button->ChangeTextButton(-1,-1,-1,-1,txt_quit.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(quit_button);
	button_index[last_index+=1] = quit_button;

	return true;
}

GameMenuDialog::~GameMenuDialog() {
}

GUI_status GameMenuDialog::close_dialog() {
	Delete(); // mark dialog as deleted. it will be freed by the GUI object
	callback_object->callback(GAMEMENUDIALOG_CB_DELETE, this, this);
	GUI::get_gui()->unlock_input();
	return GUI_YUM;
}

GUI_status GameMenuDialog::KeyDown(SDL_Keysym key) {
	KeyBinder *keybinder = Game::get_game()->get_keybinder();
	ActionType a = keybinder->get_ActionType(key);

	switch(keybinder->GetActionKeyType(a))
	{
		case NORTH_KEY:
			if(b_index_num != -1)
				button_index[b_index_num]->set_highlighted(false);

			if(b_index_num <= 0)
				b_index_num = last_index;
			else
				b_index_num = b_index_num - 1;
			button_index[b_index_num]->set_highlighted(true); break;
		case SOUTH_KEY:
			if(b_index_num != -1)
				button_index[b_index_num]->set_highlighted(false);

			if(b_index_num == last_index)
				b_index_num = 0;
			else
				b_index_num += 1;
			button_index[b_index_num]->set_highlighted(true); break;
		case DO_ACTION_KEY:
			if(b_index_num != -1) return button_index[b_index_num]->Activate_button(); break;
		case CANCEL_ACTION_KEY: return close_dialog();
		default: keybinder->handle_always_available_keys(a); break;
	}
	return GUI_YUM;
}

GUI_status GameMenuDialog::callback(uint16 msg, GUI_CallBack *caller, void *data) {
	GUI *gui = GUI::get_gui();

	if(caller == this) {
		 close_dialog();
	} else if(caller == (GUI_CallBack *)saveLoad_button) {
		Game::get_game()->get_event()->saveDialog();
	} else if(caller == (GUI_CallBack *)video_button) {
		GUI_Widget *video_dialog;
		video_dialog = (GUI_Widget *) new VideoDialog((GUI_CallBack *)this);
		GUI::get_gui()->AddWidget(video_dialog);
		gui->lock_input(video_dialog);
	} else if(caller == (GUI_CallBack *)audio_button) {
		GUI_Widget *audio_dialog;
		audio_dialog = (GUI_Widget *) new AudioDialog((GUI_CallBack *)this);
		GUI::get_gui()->AddWidget(audio_dialog);
		gui->lock_input(audio_dialog);
	} else if(caller == (GUI_CallBack *)input_button) {
		GUI_Widget *input_dialog;
		input_dialog = (GUI_Widget *) new InputDialog((GUI_CallBack *)this);
		GUI::get_gui()->AddWidget(input_dialog);
		gui->lock_input(input_dialog);
#ifdef HAVE_JOYSTICK_SUPPORT
	} else if(caller == (GUI_CallBack *)joystick_button) {
		GUI_Widget *joystick_dialog;
		joystick_dialog = (GUI_Widget *) new JoystickDialog((GUI_CallBack *)this);
		GUI::get_gui()->AddWidget(joystick_dialog);
		gui->lock_input(joystick_dialog);
#endif
	} else if(caller == (GUI_CallBack *)gameplay_button) {
		GUI_Widget *gameplay_dialog;
		gameplay_dialog = (GUI_Widget *) new GameplayDialog((GUI_CallBack *)this);
		GUI::get_gui()->AddWidget(gameplay_dialog);
		gui->lock_input(gameplay_dialog);
	}
	else if (caller == (GUI_CallBack *)cheats_button) {
		GUI_Widget *cheats_dialog;
		cheats_dialog = (GUI_Widget *) new CheatsDialog((GUI_CallBack *)this);
		GUI::get_gui()->AddWidget(cheats_dialog);
		gui->lock_input(cheats_dialog);
	}
	else if (caller == (GUI_CallBack *)continue_button) {
		return close_dialog();
	} else if(caller == (GUI_CallBack *)quit_button) {
		Game::get_game()->get_event()->quitDialog();
	} else {
		gui->lock_input(this);
		return GUI_PASS;
	}
	return GUI_YUM;
}
