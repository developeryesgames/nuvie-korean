/*
 *  GameplayDialog.cpp
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
#include "GameplayDialog.h"
#include "Party.h"
#include "Script.h"
#include "U6misc.h"
#include "Converse.h"
#include "ConverseGump.h"
#include "Configuration.h"
#include "Background.h"
#include "Keys.h"
#include "FontManager.h"
#include "KoreanTranslation.h"
#include "SaveManager.h"

#define GD_WIDTH 274
#define GD_HEIGHT 192

static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled())
		return 3;  // 3x scale for Korean mode
	return 1;
}

// Helper for translated text
static std::string get_gd_text(const char* english) {
	FontManager *fm = Game::get_game()->get_font_manager();
	KoreanTranslation *kt = Game::get_game()->get_korean_translation();
	if (fm && fm->is_korean_enabled() && kt) {
		std::string korean = kt->getUIText(english);
		if (!korean.empty()) return korean;
	}
	return std::string(english);
}

GameplayDialog::GameplayDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - GD_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - GD_HEIGHT * get_menu_scale())/2,
                       GD_WIDTH * get_menu_scale(), GD_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE) {
	callback_object = callback;
	init();
	grab_focus();
}

int get_selected_game_index(const std::string configvalue)
{
  if(string_i_compare(configvalue, "menuselect"))
  {
    return 0;
  }
  else if(string_i_compare(configvalue, "ultima6"))
  {
    return 1;
  }
  else if(string_i_compare(configvalue, "savage"))
  {
    return 2;
  }
  else if(string_i_compare(configvalue, "martian"))
  {
    return 3;
  }

  return 1; //default to U6
}

bool GameplayDialog::init() {
	int scale = get_menu_scale();
	int height = 12 * scale;
	int yesno_width = 32 * scale;
	const int selected_game_width = 120 * scale;
	int colX[] = { 9*scale, 9*scale, 233*scale };  // All labels at same X for Korean
	int buttonY = 9 * scale;
	int textY = 11 * scale;  // Changed to int to prevent overflow
	int row_h = 13 * scale;
	b_index_num = -1;
	last_index = 0;

	GUI_Widget *widget;
	GUI *gui = GUI::get_gui();
	GUI_Font *font = gui->get_font();
	Game *game = Game::get_game();
	Configuration *config = Game::get_game()->get_config();
	const char* const yesno_text[] = { "no", "yes" };
	const char* const formation_text[] = { "standard", "column", "row", "delta" };
	const char* const selected_game_text[] = {"Menu Select", "Ultima VI", "Savage Empire", "Martian Dreams"};
	const char* const converse_style_text[] = {"Default", "U7 Style", "WOU Style"};

	// Get translated texts
	std::string txt_formation = get_gd_text("Party formation:");
	std::string txt_stealing = get_gd_text("Look shows private property:");
	std::string txt_textgump = get_gd_text("Use text gump:");
	std::string txt_converse = get_gd_text("Converse gump:");
	std::string txt_solidbg = get_gd_text("Converse gump has solid bg:");
	std::string txt_restart = get_gd_text("The following require a restart:");
	std::string txt_startup = get_gd_text("Startup game:");
	std::string txt_skip = get_gd_text("Skip intro:");
	std::string txt_console = get_gd_text("Show console:");
	std::string txt_cursor = get_gd_text("Use original cursors:");
	std::string txt_autosave = get_gd_text("Autosave:");
	std::string txt_cancel = get_gd_text("Cancel");
	std::string txt_save = get_gd_text("Save");

  std::string selected_game;
  config->value("config/loadgame", selected_game, "");

	bool is_u6 = (game->get_game_type() == NUVIE_GAME_U6);
	bool show_stealing, skip_intro, show_console, use_original_cursor, solid_bg;
	std::string key = config_get_game_key(config);
	config->value(key + "/skip_intro", skip_intro, false);
	config->value("config/general/show_console", show_console, false);
	config->value("config/general/enable_cursors", use_original_cursor, false);
// party formation
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY, 0, 0, 0, txt_formation.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	formation_button = new GUI_TextToggleButton(this, 197*scale, buttonY, 68*scale, height, formation_text, 4, game->get_party()->get_formation(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { formation_button->SetTextScale(scale); formation_button->ChangeTextButton(-1,-1,-1,-1,formation_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(formation_button);
	button_index[last_index] = formation_button;
	if(is_u6) {
// show stealing
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_stealing.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
		config->value("config/ultima6/show_stealing", show_stealing, false);
		stealing_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, show_stealing, font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { stealing_button->SetTextScale(scale); stealing_button->ChangeTextButton(-1,-1,-1,-1,stealing_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(stealing_button);
		button_index[last_index+=1] = stealing_button;
	} else {
		stealing_button = NULL;
	}
	if(!Game::get_game()->is_new_style()) {
// Use text gump
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_textgump.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
		text_gump_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, game->is_using_text_gumps(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { text_gump_button->SetTextScale(scale); text_gump_button->ChangeTextButton(-1,-1,-1,-1,text_gump_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(text_gump_button);
		button_index[last_index+=1] = text_gump_button;
// use converse gump
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_converse.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
		converse_gump_button = new GUI_TextToggleButton(this, 187*scale, buttonY += row_h, 78*scale, height, converse_style_text, 3, get_converse_gump_type_from_config(config), font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { converse_gump_button->SetTextScale(scale); converse_gump_button->ChangeTextButton(-1,-1,-1,-1,converse_gump_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(converse_gump_button);
		old_converse_gump_type = game->get_converse_gump_type();
		button_index[last_index+=1] = converse_gump_button;
	} else {
		text_gump_button = NULL;
		converse_gump_button = NULL;
	}
	if(!game->is_forcing_solid_converse_bg()) {
// converse solid bg
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_solidbg.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
		config->value(key + "/converse_solid_bg", solid_bg, false); // need to check cfg since converse_gump may be NULL
		converse_solid_bg_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, solid_bg, font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { converse_solid_bg_button->SetTextScale(scale); converse_solid_bg_button->ChangeTextButton(-1,-1,-1,-1,converse_solid_bg_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(converse_solid_bg_button);
		button_index[last_index+=1] = converse_solid_bg_button;
	} else
		converse_solid_bg_button = NULL;

// autosave
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_autosave.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	bool autosave_enabled = game->get_save_manager()->is_autosave_enabled();
	autosave_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, autosave_enabled, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { autosave_button->SetTextScale(scale); autosave_button->ChangeTextButton(-1,-1,-1,-1,autosave_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(autosave_button);
	button_index[last_index+=1] = autosave_button;

// following require restart
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h*2, 0, 0, 0, txt_restart.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
// game select
  widget = (GUI_Widget *) new GUI_Text(colX[1], textY += row_h, 0, 0, 0, txt_startup.c_str(), font);
  if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
  AddWidget(widget);
  startup_game_button = new GUI_TextToggleButton(this, 145*scale, buttonY += row_h*3, selected_game_width, height, selected_game_text, 4, get_selected_game_index(selected_game),  font, BUTTON_TEXTALIGN_CENTER, this, 0);
  if (scale > 1) { startup_game_button->SetTextScale(scale); startup_game_button->ChangeTextButton(-1,-1,-1,-1,startup_game_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
  AddWidget(startup_game_button);
  button_index[last_index+=1] = startup_game_button;
// skip intro
	widget = (GUI_Widget *) new GUI_Text(colX[1], textY += row_h, 0, 0, 0, txt_skip.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	skip_intro_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, skip_intro,  font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { skip_intro_button->SetTextScale(scale); skip_intro_button->ChangeTextButton(-1,-1,-1,-1,skip_intro_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(skip_intro_button);
	button_index[last_index+=1] = skip_intro_button;
// show console
	widget = (GUI_Widget *) new GUI_Text(colX[1], textY += row_h, 0, 0, 0, txt_console.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	show_console_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, show_console, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { show_console_button->SetTextScale(scale); show_console_button->ChangeTextButton(-1,-1,-1,-1,show_console_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(show_console_button);
	button_index[last_index+=1] = show_console_button;
// original cursor
	widget = (GUI_Widget *) new GUI_Text(colX[1], textY += row_h, 0, 0, 0, txt_cursor.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	cursor_button = new GUI_TextToggleButton(this, colX[2], buttonY += row_h, yesno_width, height, yesno_text, 2, use_original_cursor, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { cursor_button->SetTextScale(scale); cursor_button->ChangeTextButton(-1,-1,-1,-1,cursor_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cursor_button);
	button_index[last_index+=1] = cursor_button;

	cancel_button = new GUI_Button(this, 77*scale, GD_HEIGHT*scale - 20*scale, 54*scale, height, txt_cancel.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { cancel_button->SetTextScale(scale); cancel_button->ChangeTextButton(-1,-1,-1,-1,txt_cancel.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cancel_button);
	button_index[last_index+=1] = cancel_button;
	save_button = new GUI_Button(this, 158*scale, GD_HEIGHT*scale - 20*scale, 40*scale, height, txt_save.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { save_button->SetTextScale(scale); save_button->ChangeTextButton(-1,-1,-1,-1,txt_save.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(save_button);
	button_index[last_index+=1] = save_button;

	return true;
}

GameplayDialog::~GameplayDialog() {
}

GUI_status GameplayDialog::close_dialog() {
	Delete(); // mark dialog as deleted. it will be freed by the GUI object
	callback_object->callback(0, this, this);
	return GUI_YUM;
}

GUI_status GameplayDialog::KeyDown(SDL_Keysym key) {
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

const char *get_selected_game_config_string(int selected_index)
{
  const char *config_strings[] = {"menuselect", "ultima6", "savage", "martian" };

  if(selected_index < 0 || selected_index > 3)
  {
    return config_strings[1];
  }

  return config_strings[selected_index];
}


const char *get_converse_gump_config_string(int selected_index)
{
  const char *config_strings[] = {"default", "u7style", "wou" };

  if(selected_index < 0 || selected_index >= 3)
  {
    return config_strings[0];
  }

  return config_strings[selected_index];
}

GUI_status GameplayDialog::callback(uint16 msg, GUI_CallBack *caller, void *data) {
	if(caller == (GUI_CallBack *)cancel_button) {
		return close_dialog();
	} else if(caller == (GUI_CallBack *)save_button) {
		Game *game  = Game::get_game();
		Configuration *config = game->get_config();
		std::string key = config_get_game_key(config);

		game->get_party()->set_formation(formation_button->GetSelection());
		config->set("config/general/party_formation", formation_button->GetSelection() ? "yes" : "no");
		if(game->get_game_type() == NUVIE_GAME_U6) {
			game->get_script()->call_set_g_show_stealing(stealing_button->GetSelection());
			config->set("config/ultima6/show_stealing", stealing_button->GetSelection() ? "yes" : "no");
		}
		if(!Game::get_game()->is_new_style()) {
			game->set_using_text_gumps(text_gump_button->GetSelection());
			config->set("config/general/use_text_gumps", text_gump_button->GetSelection() ? "yes" : "no");
			uint8 converse_gump_type = converse_gump_button->GetSelection();
			if(converse_gump_type != old_converse_gump_type) {
				config->set("config/general/converse_gump", get_converse_gump_config_string(converse_gump_type));
				game->set_converse_gump_type(converse_gump_type);
			}
		}
		if(converse_solid_bg_button) {
			if(game->get_converse_gump())
				game->get_converse_gump()->set_solid_bg(converse_solid_bg_button->GetSelection());
			config->set(key + "/converse_solid_bg", converse_solid_bg_button->GetSelection() ? "yes" : "no");
		}
		// autosave setting (immediate effect, no restart needed)
		bool new_autosave = autosave_button->GetSelection();
		game->get_save_manager()->set_autosave_enabled(new_autosave);
		config->set(key + "/autosave", new_autosave ? "yes" : "no");

		config->set("config/loadgame", get_selected_game_config_string(startup_game_button->GetSelection()));
		config->set(key + "/skip_intro", skip_intro_button->GetSelection() ? "yes" : "no"); // need restart
		config->set("config/general/show_console", show_console_button->GetSelection() ? "yes" : "no"); // need restart
		config->set("config/general/enable_cursors", cursor_button->GetSelection() ? "yes" : "no"); // need restart

		config->write();
		close_dialog();
		return GUI_YUM;
	}

 return GUI_PASS;
}
