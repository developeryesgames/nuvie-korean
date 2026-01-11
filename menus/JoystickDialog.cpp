/*
 *  JoystickDialog.cpp
 *  Nuvie
 *  Copyright (C) 2014 The Nuvie Team
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

//#include "SDL.h"
#include "nuvieDefs.h"

#ifdef HAVE_JOYSTICK_SUPPORT
#include "GUI.h"
#include "GUI_types.h"
#include "GUI_button.h"
#include "GUI_text.h"
#include "GUI_TextToggleButton.h"
#include "GUI_CallBack.h"
#include "GUI_area.h"

#include "GUI_Dialog.h"
#include "JoystickDialog.h"
#include "Configuration.h"
#include "Keys.h"
#include "FontManager.h"
//#include <math.h>

#define JD_WIDTH 244
#define JD_HEIGHT 127

static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled() && Game::get_game()->is_original_plus())
		return 3;
	return 1;
}

JoystickDialog::JoystickDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - JD_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - JD_HEIGHT * get_menu_scale())/2,
                       JD_WIDTH * get_menu_scale(), JD_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE) {
	callback_object = callback;
	init();
	grab_focus();
}

bool JoystickDialog::init() {
	int scale = get_menu_scale();
	int height = 12 * scale;
	int buttonX[] = { 128*scale, 142*scale, 183*scale};
	int textX[] = { 9*scale, 115*scale, 170*scale };
	int textY = 11 * scale;
	int buttonY = 9 * scale;
	uint8 sub_h = 4 * scale;
	uint8 row_h = 13 * scale;
	uint8 axis_w = 40 * scale;
	b_index_num = -1;
	last_index = 0;
//	uint8 yesno_width = 32;

	GUI_Widget *widget;
	GUI_Font * font = GUI::get_gui()->get_font();



//	Configuration *config = Game::get_game()->get_config();
	KeyBinder *kb = Game::get_game()->get_keybinder();

	uint8 enable_selection; /*index*/;
	char enable_buff[11], axis_buff[4];

	if(kb->get_enable_joystick() == -1)
		enable_selection = 4;
#if SDL_VERSION_ATLEAST(2,0,0) // haven't considered this in the saving loop
	else if(kb->get_enable_joystick() == 127)
	{
		enable_selection = 5;
		sprintf(enable_buff, "%s", "auto detect");
	}
#endif
	else if(kb->get_enable_joystick() < 4)
		enable_selection = kb->get_enable_joystick();
	else
	{
		enable_selection = 5;
		sprintf(enable_buff, "Joystick %i", kb->get_enable_joystick());
	}

// enable_button
	widget = (GUI_Widget *) new GUI_Text(textX[0], textY, 0, 0, 0, "Enable joystick:", font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	const char* const enabled_text[] = { "Joystick 0", "Joystick 1", "Joystick 2", "joystick 3", "Disabled", enable_buff };
	enable_button = new GUI_TextToggleButton(this, buttonX[1], buttonY, 93*scale, height, enabled_text, enable_selection == 5 ? 6 : 5, enable_selection, font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { enable_button->SetTextScale(scale); enable_button->ChangeTextButton(-1,-1,-1,-1,"Joystick 0",BUTTON_TEXTALIGN_CENTER); }
	AddWidget(enable_button);
	button_index[last_index] = enable_button;
// hat_repeating_b
	widget = (GUI_Widget *) new GUI_Text(textX[0], textY += row_h, 0, 0, 0, "Repeat when held:", font);
	if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
	AddWidget(widget);
	const char* const hat_repeating_text[] = { "axes pair 1", "hat" };
	hat_repeating_b = new GUI_TextToggleButton(this, buttonX[1], buttonY += row_h, 93*scale, height, hat_repeating_text, 2, kb->is_hat_repeating(), font, BUTTON_TEXTALIGN_CENTER, this, 0);
	if (scale > 1) { hat_repeating_b->SetTextScale(scale); hat_repeating_b->ChangeTextButton(-1,-1,-1,-1,"axes pair 1",BUTTON_TEXTALIGN_CENTER); }
	AddWidget(hat_repeating_b);
	button_index[last_index+= 1] = hat_repeating_b;
// Axes Pairs
	int str_i = 0; // used in loop
	const char* axis_text[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "none", "" };
	const char* const axes_str[] = { "Axes pair 1:", "Axes pair 2:", "Axes pair 3:", "Axes pair 4:" };
	for(int i=0; i < 8; i++) {
		if(i%2 == 0) { // pairs text
			widget = (GUI_Widget *) new GUI_Text(textX[0], textY += row_h, 0, 0, 0, axes_str[str_i++], font);
			if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
			AddWidget(widget);
		} // x and y text
		widget = (GUI_Widget *) new GUI_Text(i%2 ? textX[2] : textX[1], textY += i%2 ? 0 : sub_h, 0, 0, 0, i%2 ? "Y:" : "X:", font);
		if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
		AddWidget(widget);
	// x and y  button
		uint8 index = get_axis_index(kb->get_axis(i));
		if(index == 11) {
			sprintf(axis_buff, "%i", kb->get_axis(i));
			axis_text[11] = axis_buff;
		}
		axes_index[i] = new GUI_TextToggleButton(this, i%2 ? buttonX[2] : buttonX[0], buttonY += i%2 ? 0 : row_h + sub_h, axis_w, height, axis_text, index == 11 ? 12 : 11, index, font, BUTTON_TEXTALIGN_CENTER, this, 0);
		AddWidget(axes_index[i]); 
		button_index[last_index+= 1] = axes_index[i];
	}
// cancel_button
	cancel_button = new GUI_Button(this, 59*scale, JD_HEIGHT*scale - 20*scale, 54*scale, height, "Cancel", font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { cancel_button->SetTextScale(scale); cancel_button->ChangeTextButton(-1,-1,-1,-1,"Cancel",BUTTON_TEXTALIGN_CENTER); }
	AddWidget(cancel_button);
	button_index[last_index += 1] = cancel_button;
// save_button
	save_button = new GUI_Button(this, 124*scale, JD_HEIGHT*scale - 20*scale, 60*scale, height, "Save", font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
	if (scale > 1) { save_button->SetTextScale(scale); save_button->ChangeTextButton(-1,-1,-1,-1,"Save",BUTTON_TEXTALIGN_CENTER); }
	AddWidget(save_button);
	button_index[last_index += 1] = save_button;

 return true;
}

JoystickDialog::~JoystickDialog() {
}

uint8 JoystickDialog::get_axis_index(uint8 axis)
{
	if(axis == 255)
		return 10;
	else if(axis <= 9)
		return axis;
	else
		return 11;
}

GUI_status JoystickDialog::close_dialog() {
	Delete(); // mark dialog as deleted. it will be freed by the GUI object
	callback_object->callback(0, this, this);
	return GUI_YUM;
}

GUI_status JoystickDialog::KeyDown(SDL_Keysym key) {
	KeyBinder *keybinder = Game::get_game()->get_keybinder();
	ActionType a = keybinder->get_ActionType(key);

	switch(keybinder->GetActionKeyType(a))
	{
		case NORTH_KEY:
			if(b_index_num > 2) {
				button_index[b_index_num]->set_highlighted(false);
				if(b_index_num == last_index)
					b_index_num = b_index_num - 3;
				else
					b_index_num = b_index_num - 2;
				button_index[b_index_num]->set_highlighted(true);
				break;
			} // else fall through
		case WEST_KEY:
			if(b_index_num != -1)
				button_index[b_index_num]->set_highlighted(false);

			if(b_index_num <= 0)
				b_index_num = last_index;
			else
				b_index_num = b_index_num - 1;
			button_index[b_index_num]->set_highlighted(true); break;
		case SOUTH_KEY:
			if(b_index_num >1 && b_index_num != last_index) {
				button_index[b_index_num]->set_highlighted(false);
				b_index_num += 2;
				if(b_index_num > last_index)
					b_index_num = 0;
				button_index[b_index_num]->set_highlighted(true);
				break;
			} // else fall through
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
#include <cassert>
GUI_status JoystickDialog::callback(uint16 msg, GUI_CallBack *caller, void *data) {
	if(caller == (GUI_CallBack *)cancel_button) {
		return close_dialog();
	} else if(caller == (GUI_CallBack *)save_button) {
		Configuration *config = Game::get_game()->get_config();
		KeyBinder *kb = Game::get_game()->get_keybinder();
// enable joystick
		uint8 enabled_setting;
		if(enable_button->GetSelection() == 5)
			enabled_setting = kb->get_enable_joystick();
		else if(enable_button->GetSelection() == 4) {
			enabled_setting = 255;
			config->set("config/joystick/enable_joystick", "no");
		} else {
			enabled_setting = enable_button->GetSelection();
			config->set("config/joystick/enable_joystick", enabled_setting);
		}
		kb->init_joystick(enabled_setting == 255 ? -1 : enabled_setting); // will close and NULL joystick and open again if enabled
// hat repeating
		kb->set_hat_repeating(hat_repeating_b->GetSelection() == 1);
		config->set("config/joystick/repeat_hat", hat_repeating_b->GetSelection() == 1 ? "yes" : "no");
// Axes Pairs
		std::string axes_str[] = { "axes_pair1/x_axis", "axes_pair1/y_axis", "axes_pair2/x_axis", "axes_pair2/y_axis", 
		                           "axes_pair3/x_axis", "axes_pair3/y_axis", "axes_pair4/x_axis", "axes_pair4/y_axis" };
		for(int i=0; i < 8; i++) {
			uint8 val = axes_index[i]->GetSelection();
			if(val != kb->get_axis(i) && val != 11 && (val != 10 || kb->get_axis(i) != 255)) {
				val = val == 10 ? 255 : val;
				config->set("config/joystick/" + axes_str[i], val);
				kb->set_axis(i, val);
			}
		}

		config->write();
		return close_dialog();
	}

	return GUI_PASS;
}
#endif /* HAVE_JOYSTICK_SUPPORT */
