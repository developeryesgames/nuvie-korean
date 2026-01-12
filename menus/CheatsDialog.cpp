/*
 *  CheatsDialog.cpp
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
#include "CheatsDialog.h"
#include "EggManager.h"
#include "U6misc.h"
#include "Converse.h"
#include "ObjManager.h"
#include "MapWindow.h"
#include "Configuration.h"
#include "Keys.h"
#include "FontManager.h"
#include "KoreanTranslation.h"

#define CD_WIDTH 212
#define CD_HEIGHT 101

static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled())
		return 3;  // 3x scale for Korean mode
	return 1;
}

CheatsDialog::CheatsDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - CD_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - CD_HEIGHT * get_menu_scale())/2,
                       CD_WIDTH * get_menu_scale(), CD_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE) {
	callback_object = callback;
	init();
	grab_focus();
}

// Helper to get translated text
static std::string get_translated_text(const char *english_text) {
	KoreanTranslation *kt = Game::get_game()->get_korean_translation();
	if (kt && kt->isEnabled()) {
		std::string t = kt->getUIText(english_text);
		if (!t.empty()) return t;
	}
	return english_text;
}

bool CheatsDialog::init() {
	int scale = get_menu_scale();
	int textY[] = { 11*scale, 24*scale, 37*scale, 50*scale, 63*scale };
	int buttonY[] = { 9*scale, 22*scale, 35*scale, 48*scale, 61*scale, 80*scale };
	int colX[] = { 9*scale, 163*scale };
	int height = 12*scale;
	b_index_num = -1;
	last_index = 0;
	GUI_Widget *widget;
	GUI *gui = GUI::get_gui();

	std::string txt;
	txt = get_translated_text("Cheats:");
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY[0], 0, 0, 0, txt.c_str(), gui->get_font());
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	txt = get_translated_text("Show eggs:");
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY[1], 0, 0, 0, txt.c_str(), gui->get_font());
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	txt = get_translated_text("Enable hackmove:");
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY[2], 0, 0, 0, txt.c_str(), gui->get_font());
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	txt = get_translated_text("Anyone will join:");
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY[3], 0, 0, 0, txt.c_str(), gui->get_font());
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	txt = get_translated_text("Minimum brightness:");
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY[4], 0, 0, 0, txt.c_str(), gui->get_font());
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);

	bool party_all_the_time;
	Game *game = Game::get_game();
	Configuration *config = game->get_config();
	config->value("config/cheats/party_all_the_time", party_all_the_time);
	std::string disabled_txt = get_translated_text("Disabled");
	std::string enabled_txt = get_translated_text("Enabled");
	const char* enabled_text[] = { disabled_txt.c_str(), enabled_txt.c_str() };
	std::string no_txt = get_translated_text("no");
	std::string yes_txt = get_translated_text("yes");
	const char* yesno_text[] = { no_txt.c_str(), yes_txt.c_str() };
	char buff[4];
	int brightness_selection;
	int num_of_brightness = 8;
	uint8 min_brightness = game->get_map_window()->get_min_brightness();

	if(min_brightness == 255) {
		brightness_selection = 7;
	} else if(min_brightness%20 == 0 && min_brightness <= 120) {
		brightness_selection = min_brightness/20;
	} else {
		num_of_brightness = 9;
		brightness_selection = 8; // manually edited setting or old 128
		sprintf(buff, "%d", min_brightness);
	}
	const char* const brightness_text[] = { "0", "20", "40", "60", "80", "100", "120", "255", buff };

	cheat_button = new GUI_TextToggleButton(this, colX[1] - 30*scale, buttonY[0], 70*scale, height, enabled_text, 2, game->are_cheats_enabled(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { cheat_button->SetTextScale(scale); cheat_button->ChangeTextButton(-1,-1,-1,-1,cheat_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cheat_button);
	button_index[last_index] = cheat_button;

	egg_button = new GUI_TextToggleButton(this, colX[1], buttonY[1], 40*scale, height, yesno_text, 2, game->get_obj_manager()->is_showing_eggs(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { egg_button->SetTextScale(scale); egg_button->ChangeTextButton(-1,-1,-1,-1,egg_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(egg_button);
	button_index[last_index+=1] = egg_button;

	hackmove_button = new GUI_TextToggleButton(this, colX[1], buttonY[2], 40*scale, height, yesno_text, 2, game->using_hackmove(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { hackmove_button->SetTextScale(scale); hackmove_button->ChangeTextButton(-1,-1,-1,-1,hackmove_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(hackmove_button);
	button_index[last_index+=1] = hackmove_button;

	party_button = new GUI_TextToggleButton(this, colX[1], buttonY[3], 40*scale, height, yesno_text, 2, party_all_the_time, gui->get_font(), BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { party_button->SetTextScale(scale); party_button->ChangeTextButton(-1,-1,-1,-1,party_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(party_button);
	button_index[last_index+=1] = party_button;

	brightness_button = new GUI_TextToggleButton(this, colX[1], buttonY[4], 40*scale, height, brightness_text, num_of_brightness, brightness_selection,  gui->get_font(), BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { brightness_button->SetTextScale(scale); brightness_button->ChangeTextButton(-1,-1,-1,-1,brightness_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(brightness_button);
	button_index[last_index+=1] = brightness_button;

	std::string cancel_txt = get_translated_text("Cancel");
	cancel_button = new GUI_Button(this, 50*scale, buttonY[5], 54*scale, height, cancel_txt.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { cancel_button->SetTextScale(scale); cancel_button->ChangeTextButton(-1,-1,-1,-1,cancel_txt.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cancel_button);
	button_index[last_index+=1] = cancel_button;

	std::string save_txt = get_translated_text("Save");
	save_button = new GUI_Button(this, 121*scale, buttonY[5], 40*scale, height, save_txt.c_str(), gui->get_font(), BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { save_button->SetTextScale(scale); save_button->ChangeTextButton(-1,-1,-1,-1,save_txt.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(save_button);
	button_index[last_index+=1] = save_button;

	return true;
}

CheatsDialog::~CheatsDialog() {
}

GUI_status CheatsDialog::close_dialog() {
	Delete(); // mark dialog as deleted. it will be freed by the GUI object
	callback_object->callback(0, this, this); // I don't think this does anything atm
	return GUI_YUM;
}

GUI_status CheatsDialog::KeyDown(SDL_Keysym key) {
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

GUI_status CheatsDialog::callback(uint16 msg, GUI_CallBack *caller, void *data) {
	if(caller == (GUI_CallBack *)cancel_button) {
		return close_dialog();
	} else if(caller == (GUI_CallBack *)save_button) {
		Game *game = Game::get_game();
		Configuration *config = game->get_config();

		std::string key = config_get_game_key(config);
		key.append("/show_eggs");
		config->set(key, egg_button->GetSelection() ? "yes" : "no");
		game->get_obj_manager()->set_show_eggs(egg_button->GetSelection());
		game->get_egg_manager()->set_egg_visibility(cheat_button->GetSelection() ? egg_button->GetSelection() : false);

		game->set_cheats_enabled(cheat_button->GetSelection());
		config->set("config/cheats/enabled", cheat_button->GetSelection() ? "yes" : "no");
		game->set_hackmove(hackmove_button->GetSelection());
		config->set("config/cheats/enable_hackmove", hackmove_button->GetSelection() ? "yes" : "no");
		game->get_converse()->set_party_all_the_time(party_button->GetSelection());
		config->set("config/cheats/party_all_the_time", party_button->GetSelection() ? "yes" : "no");

		int brightness = brightness_button->GetSelection();
		if(brightness < 8) {
			int min_brightness;
			if(brightness == 7)
				min_brightness = 255;
			else
				min_brightness = brightness*20;
			config->set("config/cheats/min_brightness", min_brightness);
			game->get_map_window()->set_min_brightness(min_brightness);
			game->get_map_window()->updateAmbience();
		}

		config->write();
		return close_dialog();
	}

	return GUI_PASS;
}
