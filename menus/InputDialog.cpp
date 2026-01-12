/*
 *  InputDialog.cpp
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
#include "InputDialog.h"
#include "Configuration.h"
#include "Event.h"
#include "MapWindow.h"
#include "PartyView.h"
#include "ViewManager.h"
#include "CommandBarNewUI.h"
#include "U6misc.h"
#include "Keys.h"
#include "FontManager.h"
#include "KoreanTranslation.h"

#define ID_WIDTH 280
#define ID_HEIGHT 166

static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled())
		return 3;  // 3x scale for Korean mode
	return 1;
}

// Helper for translated text
static std::string get_id_text(const char* english) {
	FontManager *fm = Game::get_game()->get_font_manager();
	KoreanTranslation *kt = Game::get_game()->get_korean_translation();
	if (fm && fm->is_korean_enabled() && kt) {
		std::string korean = kt->getUIText(english);
		if (!korean.empty()) return korean;
	}
	return std::string(english);
}

InputDialog::InputDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - ID_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - ID_HEIGHT * get_menu_scale())/2,
                       ID_WIDTH * get_menu_scale(), ID_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE) {
	callback_object = callback;
	init();
	grab_focus();
}

bool InputDialog::init() {
	int scale = get_menu_scale();
	int textY = 11 * scale;  // Changed to int to prevent overflow
	int buttonY = 9 * scale;
	int colX[] = { 9*scale, 239*scale };
	int height = 12 * scale;
	int row_h = 13 * scale;
	int yesno_width = 32 * scale;
	b_index_num = -1;
	last_index = 0;
	GUI_Widget *widget;
	GUI *gui = GUI::get_gui();
	GUI_Font *font = gui->get_font();
	Game *game = Game::get_game();
	MapWindow *map_window = game->get_map_window();

	// Get translated texts
	std::string txt_interface = get_id_text("Interface:");
	std::string txt_dragging = get_id_text("Dragging enabled:");
	std::string txt_direction = get_id_text("Direction selects target:");
	std::string txt_look = get_id_text("Look on left_click:");
	std::string txt_walk = get_id_text("Walk with left button:");
	std::string txt_doubleclick = get_id_text("Enable doubleclick:");
	std::string txt_balloon = get_id_text("Allow free balloon movement:");
	std::string txt_container = get_id_text("Doubleclick opens containers:");
	std::string txt_command = get_id_text("Use new command bar:");
	std::string txt_party = get_id_text("Party view targeting:");
	std::string txt_cancel = get_id_text("Cancel");
	std::string txt_save = get_id_text("Save");

	widget = (GUI_Widget *) new GUI_Text(colX[0], textY, 0, 0, 0, txt_interface.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_dragging.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_direction.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_look.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_walk.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_doubleclick.c_str(), font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	if(game->get_game_type() == NUVIE_GAME_U6) {
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_balloon.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
	}
	if(!game->is_new_style()) {
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_container.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
	}
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_command.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
	if(!game->is_new_style()) {
		widget = (GUI_Widget *) new GUI_Text(colX[0], textY += row_h, 0, 0, 0, txt_party.c_str(), font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
	}

	Configuration *config = game->get_config();
	int interface;
	std::string interface_str;
	config->value("config/input/interface", interface_str, "normal"); // get cfg variable because hackmove changes InterfaceType
	if(interface_str == "ignore_block")
		interface = 2;
	else if(interface_str == "fullscreen")
		interface = 1;
	else // normal
		interface = 0;

	const char* const yesno_text[] = { "no", "yes" };
	const char* const interface_text[] = { "Normal", "U7 like", "ignores obstacles" };

	interface_button = new GUI_TextToggleButton(this, 129*scale, buttonY, 142*scale, height, interface_text, 3, interface, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { interface_button->SetTextScale(scale); interface_button->ChangeTextButton(-1,-1,-1,-1,interface_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(interface_button);
	button_index[last_index] = interface_button;

	dragging_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, game->is_dragging_enabled(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { dragging_button->SetTextScale(scale); dragging_button->ChangeTextButton(-1,-1,-1,-1,dragging_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(dragging_button);
	button_index[last_index+=1] = dragging_button;

	direction_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, game->get_event()->is_direction_selecting_targets(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { direction_button->SetTextScale(scale); direction_button->ChangeTextButton(-1,-1,-1,-1,direction_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(direction_button);
	button_index[last_index+=1] = direction_button;

	look_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, map_window->will_look_on_left_click(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { look_button->SetTextScale(scale); look_button->ChangeTextButton(-1,-1,-1,-1,look_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(look_button);
	button_index[last_index+=1] = look_button;

	walk_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, map_window->will_walk_with_left_button(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { walk_button->SetTextScale(scale); walk_button->ChangeTextButton(-1,-1,-1,-1,walk_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(walk_button);
	button_index[last_index+=1] = walk_button;

	doubleclick_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, map_window->is_doubleclick_enabled(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { doubleclick_button->SetTextScale(scale); doubleclick_button->ChangeTextButton(-1,-1,-1,-1,doubleclick_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(doubleclick_button);
	button_index[last_index+=1] = doubleclick_button;

	if(game->get_game_type() == NUVIE_GAME_U6) {
		balloon_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, game->has_free_balloon_movement(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { balloon_button->SetTextScale(scale); balloon_button->ChangeTextButton(-1,-1,-1,-1,balloon_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(balloon_button);
		button_index[last_index+=1] = balloon_button;
	} else
		balloon_button = NULL;
	if(!Game::get_game()->is_new_style()) {
		open_container_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, game->doubleclick_opens_containers(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { open_container_button->SetTextScale(scale); open_container_button->ChangeTextButton(-1,-1,-1,-1,open_container_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(open_container_button);
		button_index[last_index+=1] = open_container_button;
		}
		command_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, game->get_new_command_bar() != NULL, font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { command_button->SetTextScale(scale); command_button->ChangeTextButton(-1,-1,-1,-1,command_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(command_button);
		button_index[last_index+=1] = command_button;

	if(!Game::get_game()->is_new_style()) {
		bool party_view_targeting;
		config->value("config/input/party_view_targeting", party_view_targeting, false);
		party_targeting_button = new GUI_TextToggleButton(this, colX[1], buttonY += row_h, yesno_width, height, yesno_text, 2, party_view_targeting,  font, BUTTON_TEXTALIGN_CENTER, this, 0);
		if (scale > 1) { party_targeting_button->SetTextScale(scale); party_targeting_button->ChangeTextButton(-1,-1,-1,-1,party_targeting_button->GetCurrentText(),BUTTON_TEXTALIGN_CENTER); }
		AddWidget(party_targeting_button);
		button_index[last_index+=1] = party_targeting_button;
	} else
		open_container_button = party_targeting_button = NULL;
	cancel_button = new GUI_Button(this, 83*scale, ID_HEIGHT*scale - 20*scale, 54*scale, height, txt_cancel.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { cancel_button->SetTextScale(scale); cancel_button->ChangeTextButton(-1,-1,-1,-1,txt_cancel.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cancel_button);
	button_index[last_index+=1] = cancel_button;

	save_button = new GUI_Button(this, 154*scale, ID_HEIGHT*scale - 20*scale, 40*scale, height, txt_save.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { save_button->SetTextScale(scale); save_button->ChangeTextButton(-1,-1,-1,-1,txt_save.c_str(),BUTTON_TEXTALIGN_CENTER); }
	AddWidget(save_button);
	button_index[last_index+=1] = save_button;
 
 return true;
}

InputDialog::~InputDialog() {
}

GUI_status InputDialog::close_dialog() {
	Delete(); // mark dialog as deleted. it will be freed by the GUI object
	callback_object->callback(0, this, this);
	return GUI_YUM;
}

GUI_status InputDialog::KeyDown(SDL_Keysym key) {
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

GUI_status InputDialog::callback(uint16 msg, GUI_CallBack *caller, void *data) {
	if(caller == (GUI_CallBack *)cancel_button) {
		return close_dialog();
	} else if(caller == (GUI_CallBack *)save_button) {
		Game *game = Game::get_game();
		MapWindow *map_window = game->get_map_window();
		Configuration *config = Game::get_game()->get_config();

		std::string interface_str;
		if(interface_button->GetSelection() == 2)
			interface_str = "ignore_block";
		else if(interface_button->GetSelection() == 1)
			interface_str = "fullscreen";
		else // 0
			interface_str = "normal";
		config->set("config/input/interface", interface_str);
		map_window->set_interface(); // must come after you set cfg

		game->set_dragging_enabled(dragging_button->GetSelection());
		config->set("config/input/enabled_dragging", dragging_button->GetSelection() ? "yes" : "no");

		game->get_event()->set_direction_selects_target(direction_button->GetSelection());
		config->set("config/input/direction_selects_target", direction_button->GetSelection() ? "yes" : "no");

		map_window->set_look_on_left_click(look_button->GetSelection());
		config->set("config/input/look_on_left_click", look_button->GetSelection() ? "yes" : "no");

		map_window->set_walk_with_left_button(walk_button->GetSelection());
		config->set("config/input/walk_with_left_button", walk_button->GetSelection() ? "yes" : "no");

		map_window->set_enable_doubleclick(doubleclick_button->GetSelection());
		config->set("config/input/enable_doubleclick", doubleclick_button->GetSelection() ? "yes" : "no");

		map_window->set_use_left_clicks(); // allow or disallow left clicks - Must come after look_on_left_click and enable_doubleclick

		if(game->get_game_type() == NUVIE_GAME_U6) {
			game->set_free_balloon_movement(balloon_button->GetSelection() == 1);
			config->set(config_get_game_key(config) + "/free_balloon_movement", balloon_button->GetSelection() ? "yes" : "no");
		}
		if(open_container_button) {
			game->set_doubleclick_opens_containers(open_container_button->GetSelection());
			config->set("config/input/doubleclick_opens_containers", open_container_button->GetSelection() ? "yes" : "no");
		}
			if(command_button->GetSelection())
				game->init_new_command_bar();
			else
				game->delete_new_command_bar();
			config->set("config/input/new_command_bar", command_button->GetSelection() ? "yes" : "no");
		if(party_targeting_button) {
			game->get_view_manager()->get_party_view()->set_party_view_targeting(party_targeting_button->GetSelection());
			config->set("config/input/party_view_targeting", party_targeting_button->GetSelection() ? "yes" : "no");
		}

		config->write();
		close_dialog();
		return GUI_YUM;
	}

 return GUI_PASS;
}
