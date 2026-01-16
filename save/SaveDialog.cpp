/*
 *  SaveDialog.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Mon May 10 2004.
 *  Copyright (c) 2004. All rights reserved.
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

#include "SDL.h"
#include "nuvieDefs.h"

#include "GUI.h"
#include "GUI_types.h"
#include "GUI_button.h"
#include "GUI_text.h"
#include "GUI_Scroller.h"
#include "GUI_CallBack.h"
#include "GUI_area.h"

#include "GUI_Dialog.h"
#include "SaveSlot.h"
#include "SaveDialog.h"
#include "NuvieFileList.h"
#include "Keys.h"
#include "Event.h"
#include "Console.h"
#include "FontManager.h"
#include "KoreanTranslation.h"

#define CURSOR_HIDDEN 0
#define CURSOR_AT_TOP 1
#define CURSOR_AT_SLOTS 2

#define NUVIE_SAVE_SCROLLER_ROWS   3
#define SD_WIDTH 320
#define SD_HEIGHT 200

static int get_menu_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled())
		return 3;  // 3x scale for Korean mode
	return 1;
}

SaveDialog::SaveDialog(GUI_CallBack *callback)
          : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - SD_WIDTH * get_menu_scale())/2,
                       Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - SD_HEIGHT * get_menu_scale())/2,
                       SD_WIDTH * get_menu_scale(), SD_HEIGHT * get_menu_scale(), 244, 216, 131, GUI_DIALOG_UNMOVABLE)
{
 callback_object = callback;
 selected_slot = NULL;
 scroller = NULL;
 save_button = NULL;
 load_button = NULL;
 cancel_button = NULL;
 cursor_tile = NULL;
 show_cursor = false;
 index = 0;
 save_index = 3;
 set_cursor_pos(0);
 cursor_loc = CURSOR_HIDDEN;
}

bool SaveDialog::init(const char *save_directory, const char *search_prefix)
{
 int scale = get_menu_scale();
 uint32 num_saves, i;
 NuvieFileList filelist;
 std::string *filename;
 GUI_Widget *widget;
 GUI *gui = GUI::get_gui();
 GUI_Font *font = gui->get_font();
 GUI_Color bg_color = GUI_Color(162,144,87);
 GUI_Color bg_color1 = GUI_Color(147,131,74);
 GUI_Color *color_ptr;

 if(filelist.open(save_directory, search_prefix, NUVIE_SORT_TIME_DESC) == false)
 {
   ConsoleAddError(std::string("Failed to open ") + save_directory);
   return false;
 }

 int slot_height = get_saveslot_height();
 int scroller_height = NUVIE_SAVE_SCROLLER_ROWS * slot_height;
 scroller = new GUI_Scroller(20*scale,26*scale, 280*scale, scroller_height, 135,119,76, slot_height);

 // Get translated text
 KoreanTranslation *kt = Game::get_game()->get_korean_translation();
 std::string load_save_text = "Load/Save";
 if (kt && kt->isEnabled()) {
   std::string translated = kt->getUIText("Load/Save");
   if (!translated.empty()) load_save_text = translated;
 }
 widget = (GUI_Widget *) new GUI_Text(20*scale, 12*scale, 0, 0, 0, load_save_text.c_str(), font);
 if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
 AddWidget(widget);

 num_saves = filelist.get_num_files();



// Add an empty slot at the top.
 widget = new SaveSlot(this, bg_color1);
 ((SaveSlot *)widget)->init(NULL, NULL);

 scroller->AddWidget(widget);

 color_ptr = &bg_color;

 for(i=0; i < num_saves + 1; i++)
   {
    if(i < num_saves)
      filename = filelist.next();
    else
      filename = NULL;
    widget = new SaveSlot(this, *color_ptr);
    if(((SaveSlot *)widget)->init(save_directory, filename) == true)
     {
      scroller->AddWidget(widget);

      if(color_ptr == &bg_color)
        color_ptr = &bg_color1;
      else
        color_ptr = &bg_color;
     }
    else
      delete (SaveSlot *)widget;
   }

 // pad out empty slots if required
/*
 if(num_saves < NUVIE_SAVE_SCROLLER_ROWS-1)
   {
    for(i=0; i < NUVIE_SAVE_SCROLLER_ROWS - num_saves - 1; i++)
      {
       widget = new SaveSlot(this, *color_ptr);
       ((SaveSlot *)widget)->init(NULL);

       scroller->AddWidget(widget);

       if(color_ptr == &bg_color)
         color_ptr = &bg_color1;
       else
         color_ptr = &bg_color;
      }
   }
*/
 AddWidget(scroller);

 // Get translated button texts
 std::string load_text = "Load", save_text = "Save", cancel_text = "Cancel";
 if (kt && kt->isEnabled()) {
   std::string t = kt->getUIText("Load");
   if (!t.empty()) load_text = t;
   t = kt->getUIText("Save");
   if (!t.empty()) save_text = t;
   t = kt->getUIText("Cancel");
   if (!t.empty()) cancel_text = t;
 }

 load_button = new GUI_Button(this, 105*scale, 8*scale, 40*scale, 16*scale, load_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
 if (scale > 1) { load_button->SetTextScale(scale); load_button->ChangeTextButton(-1,-1,-1,-1,load_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
 AddWidget(load_button);

 save_button = new GUI_Button(this, 175*scale, 8*scale, 40*scale, 16*scale, save_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
 if (scale > 1) { save_button->SetTextScale(scale); save_button->ChangeTextButton(-1,-1,-1,-1,save_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
 AddWidget(save_button);

 cancel_button = new GUI_Button(this, 245*scale, 8*scale, 55*scale, 16*scale, cancel_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0); //154
 if (scale > 1) { cancel_button->SetTextScale(scale); cancel_button->ChangeTextButton(-1,-1,-1,-1,cancel_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
 AddWidget(cancel_button);

 filelist.close();

 if(Game::get_game()->is_armageddon() || Game::get_game()->get_event()->using_control_cheat())
     save_button->Hide();

 cursor_tile = Game::get_game()->get_tile_manager()->get_gump_cursor_tile();

 return true;
}


SaveDialog::~SaveDialog()
{
}

void SaveDialog::Display(bool full_redraw)
{
	GUI_Dialog::Display(full_redraw);
	if(show_cursor) {
		screen->blit(cursor_x, cursor_y,(unsigned char *)cursor_tile->data,8,16,16,16,true);
		screen->update(cursor_x, cursor_y, 16, 16);
	}
}

void SaveDialog::set_cursor_pos(uint8 index_num)
{
	int scale = get_menu_scale();
	switch(index_num) {
		case 0: cursor_x = 117*scale; cursor_y = 8*scale; break;
		case 1: cursor_x = 188*scale; cursor_y = 8*scale; break;
		case 2: cursor_x = 264*scale; cursor_y = 8*scale; break;
		case 3: cursor_x = 146*scale; cursor_y = 26*scale; break;
		case 4: cursor_x = 146*scale; cursor_y = 78*scale; break;
		default:
		case 5: cursor_x = 146*scale; cursor_y = 130*scale; break;
	}
		cursor_x += area.x; cursor_y += area.y;
}

GUI_status SaveDialog::close_dialog()
{
 Delete(); // mark dialog as deleted. it will be freed by the GUI object
 return callback_object->callback(SAVEDIALOG_CB_DELETE, this, this);
}

GUI_status SaveDialog::MouseWheel(sint32 x,sint32 y)
{
    if(y > 0)
    {
        scroller->move_up();
    }
    else if(y < 0)
    {
        scroller->move_down();
    }
    return GUI_YUM;
}

GUI_status SaveDialog::MouseDown(int x, int y, int button)
{
 return GUI_YUM;
}

GUI_status SaveDialog::KeyDown(SDL_Keysym key)
{
 KeyBinder *keybinder = Game::get_game()->get_keybinder();
 ActionType a = keybinder->get_ActionType(key);

 switch(keybinder->GetActionKeyType(a))
 {
	case EAST_KEY:
		if(cursor_loc == CURSOR_AT_TOP) {
			if(index == 2)
				set_cursor_pos(index = 0);
			else
				set_cursor_pos(++index);
			break;
		} // else fall through
	case MSGSCROLL_DOWN_KEY: scroller->page_down(); break;

	case WEST_KEY:
		if(cursor_loc == CURSOR_AT_TOP) {
			if(index == 0)
				set_cursor_pos(index = 2);
			else
				set_cursor_pos(--index);
			break;
		} // else fall through
	case MSGSCROLL_UP_KEY: scroller->page_up(); break;

	case NORTH_KEY:
		if(cursor_loc == CURSOR_AT_SLOTS && save_index != 3) { // not at top of save slots
			set_cursor_pos(--save_index); break;
		}
		scroller->move_up(); break;
	case SOUTH_KEY:
		if(cursor_loc == CURSOR_AT_SLOTS && save_index != 5) { // not at bottom of save slots
			set_cursor_pos(++save_index); break;
		}
		scroller->move_down(); break;
	case HOME_KEY: scroller->page_up(true); break;
	case END_KEY:scroller->page_down(true); break;
	case TOGGLE_CURSOR_KEY:
		if(cursor_loc == CURSOR_AT_TOP) {
			cursor_loc = CURSOR_HIDDEN;
			show_cursor = false;
		} else {
			if(cursor_loc == CURSOR_HIDDEN) {
				set_cursor_pos(save_index);
				cursor_loc = CURSOR_AT_SLOTS;
			} else { // if CURSOR_AT_SLOTS
				set_cursor_pos(index);
				cursor_loc = CURSOR_AT_TOP;
			}
			show_cursor = true;
		}
		break;
	case DO_ACTION_KEY: {
		if(show_cursor == false)
			break;
		if(cursor_loc == CURSOR_AT_SLOTS) // editing save description and selecting the slot
			show_cursor = false;
		uint16 x = cursor_x*screen->get_scale_factor();
		uint16 y = (cursor_y + 8)*screen->get_scale_factor();

		SDL_Event fake_event;
		fake_event.button.x = x;
		fake_event.button.y = y;
		fake_event.type = fake_event.button.type = SDL_MOUSEBUTTONDOWN;
		fake_event.button.state = SDL_RELEASED;
		fake_event.button.button = SDL_BUTTON_LEFT;
		GUI::get_gui()->HandleEvent(&fake_event);

		fake_event.button.x = x; //GUI::HandleEvent divides by scale so we need to restore it
		fake_event.button.y = y;
		fake_event.type = fake_event.button.type = SDL_MOUSEBUTTONUP;
		GUI::get_gui()->HandleEvent(&fake_event);
		break;
	}
	case CANCEL_ACTION_KEY: return close_dialog();
	default: keybinder->handle_always_available_keys(a); break;
 }
 return GUI_YUM;
}

GUI_status SaveDialog::callback(uint16 msg, GUI_CallBack *caller, void *data)
{
 if(caller == (GUI_CallBack *)cancel_button)
    return close_dialog();

 if(caller == (GUI_CallBack *)load_button)
    {
     if(selected_slot != NULL && callback_object->callback(SAVEDIALOG_CB_LOAD, this, selected_slot) == GUI_YUM)
        close_dialog();
     return GUI_YUM;
    }

 if(caller == (GUI_CallBack *)save_button)
    {
     // Don't allow saving to autosave slot
     if(selected_slot != NULL && !selected_slot->is_autosave_slot() &&
        callback_object->callback(SAVEDIALOG_CB_SAVE, this, selected_slot) == GUI_YUM)
        close_dialog();
     return GUI_YUM;
    }

 if(dynamic_cast<SaveSlot*>(caller))
   {
    if(msg == SAVESLOT_CB_SELECTED)
      {
       if(selected_slot != NULL)
         selected_slot->deselect();

       selected_slot = (SaveSlot *)caller;
      }

    if(msg == SAVESLOT_CB_SAVE)
      {
       // Don't allow saving to autosave slot
       SaveSlot *slot = (SaveSlot *)caller;
       if(!slot->is_autosave_slot() &&
          callback_object->callback(SAVEDIALOG_CB_SAVE, this, caller) == GUI_YUM) //caller = slot to save in
         close_dialog();
      }
   }
/*
 if(caller == (GUI_CallBack *)no_button)
   return no_callback_object->callback(YESNODIALOG_CB_NO, this, this);
*/
 return GUI_PASS;
}
