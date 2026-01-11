/*
 *  SaveSlot.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Wed May 12 2004.
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
#include "U6misc.h"

#include "GUI.h"
#include "GUI_types.h"
#include "GUI_text.h"
#include "GUI_TextInput.h"

#include "GUI_CallBack.h"

#include "Game.h"
#include "NuvieIOFile.h"
#include "MapWindow.h"

#include "SaveGame.h"
#include "SaveSlot.h"
#include "SaveManager.h"
#include "SaveDialog.h"
#include "GUI_Scroller.h"
#include "FontManager.h"
#include "KoreanTranslation.h"

int get_saveslot_scale() {
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled() && Game::get_game()->is_original_plus())
		return 3;
	return 1;
}

int get_saveslot_height() {
	return NUVIE_SAVESLOT_HEIGHT_BASE * get_saveslot_scale();
}

SaveSlot::SaveSlot(GUI_CallBack *callback, GUI_Color bg_color) : GUI_Widget(NULL, 0, 0, 266 * get_saveslot_scale(), get_saveslot_height())
{
 callback_object = callback;
 background_color = bg_color;

 selected = false;
 new_save = false;
 thumbnail = NULL;
 textinput_widget = NULL;
}

SaveSlot::~SaveSlot()
{
 if(thumbnail)
  SDL_FreeSurface(thumbnail);
}

bool SaveSlot::init(const char *directory, std::string *file)
{
 GUI *gui = GUI::get_gui();

 if(file != NULL)
  {
   filename.assign(file->c_str());
   if(!load_info(directory))
     return false;
  }
 else
  {
   filename.assign(""); //empty save slot.
   if(directory != NULL)
   {
     KoreanTranslation *kt = Game::get_game()->get_korean_translation();
     if (kt && kt->isEnabled()) {
       std::string translated = kt->getUIText("Original Game Save");
       save_description.assign(translated.empty() ? "Original Game Save" : translated);
     } else {
       save_description.assign("Original Game Save");
     }
   }
   else
   {
     KoreanTranslation *kt = Game::get_game()->get_korean_translation();
     if (kt && kt->isEnabled()) {
       std::string translated = kt->getUIText("New Save.");
       save_description.assign(translated.empty() ? "New Save." : translated);
     } else {
       save_description.assign("New Save.");
     }
     new_save = true;
   }
  }

 int scale = get_saveslot_scale();
 int thumb_offset = MAPWINDOW_THUMBNAIL_SIZE * scale + 2 * scale;
 textinput_widget = new GUI_TextInput(thumb_offset, 2 * scale, 255, 255, 255, (char *)save_description.c_str(), gui->get_font(),26,2, this);
 if (scale > 1) {
   textinput_widget->SetTextScale(scale);
   textinput_widget->UpdateAreaSize();
 }
 AddWidget((GUI_Widget *)textinput_widget);

 return true;
}

bool SaveSlot::load_info(const char *directory)
{
 NuvieIOFileRead loadfile;
 SaveGame *savegame;
 SaveHeader *header;
 GUI_Widget *widget;
 GUI *gui = GUI::get_gui();
 std::string full_path;
 char buf[64];  // Increased buffer for Korean UTF-8 text
 uint8 i;
 int scale = get_saveslot_scale();
 int thumb_offset = MAPWINDOW_THUMBNAIL_SIZE * scale + 2 * scale;

 savegame = new SaveGame(Game::get_game()->get_config());

 build_path(directory, filename.c_str(), full_path);

 if(loadfile.open(full_path.c_str()) == false || savegame->check_version(&loadfile) == false)
   {
    DEBUG(0,LEVEL_ERROR,"Reading header from %s\n", filename.c_str());
    delete savegame;
    return false;
   }

 header = savegame->load_info(&loadfile);

 save_description = header->save_description;

 thumbnail = SDL_ConvertSurface(header->thumbnail, header->thumbnail->format, SDL_SWSURFACE);

 // Check for Korean mode
 KoreanTranslation *kt = Game::get_game()->get_korean_translation();
 bool use_korean = kt && kt->isEnabled();

 if(use_korean)
 {
   const char *gender_str = (header->player_gender == 0) ? "(남)" : "(여)";
   sprintf(buf, "%s %s", header->player_name.c_str(), gender_str);
 }
 else
 {
   char gender = (header->player_gender == 0) ? 'm' : 'f';
   sprintf(buf, "%s (%c)", header->player_name.c_str(), gender);
   if(strlen(buf) < 17)
   {
     for(i=strlen(buf);i < 17; i++)
       buf[i] = ' ';
     buf[17] = '\0';
   }
   strcat(buf, " Day:");
 }

 widget = (GUI_Widget *) new GUI_Text(thumb_offset, 20 * scale, 200, 200, 200, buf, gui->get_font()); //evil const cast lets remove this
 if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
 AddWidget(widget);

 if(use_korean)
   sprintf(buf, "레벨:%d 힘%d/민%d/지%d 경험:%d", header->level, header->str, header->dex, header->intelligence, header->exp);
 else
   sprintf(buf, "Lvl:%d %3d/%3d/%3d  XP:%d", header->level, header->str, header->dex, header->intelligence, header->exp);

 widget = (GUI_Widget *) new GUI_Text(thumb_offset, 29 * scale, 200, 200, 200, buf, gui->get_font()); //evil const cast lets remove this
 if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
 AddWidget(widget);


 if(use_korean)
   sprintf(buf, "%s (%d회 저장)", (char *)filename.c_str(), header->num_saves);
 else
 {
   sprintf(buf, "%s (%d save", (char *)filename.c_str(), header->num_saves);
   if(header->num_saves > 1)
     strcat(buf, "s");
   strcat(buf, ")");
 }

 widget = (GUI_Widget *) new GUI_Text(thumb_offset, 38 * scale, 200, 200, 200, buf, gui->get_font());
 if (scale > 1) ((GUI_Text*)widget)->SetTextScale(scale);
 AddWidget(widget);

 delete savegame;

 return true;
}

std::string *SaveSlot::get_filename()
{
 return &filename;
}

void SaveSlot::deselect()
{
	selected = false;
	textinput_widget->set_text(save_description.c_str()); //revert to original text
}

std::string SaveSlot::get_save_description()
{
	// In Korean mode, use UTF-8 text
	FontManager *fm = Game::get_game()->get_font_manager();
	if (fm && fm->is_korean_enabled() && Game::get_game()->is_original_plus())
	{
		return textinput_widget->get_utf8_text();
	}
	return std::string(textinput_widget->get_text());
}

/* Map the color to the display */
void SaveSlot::SetDisplay(Screen *s)
{
	GUI_Widget::SetDisplay(s);
	background_color.map_color(surface);
}

void SaveSlot::Display(bool full_redraw)
{
 SDL_Rect framerect = area;
 SDL_Rect destrect = area;
 if(selected)
   {
    int scale = get_saveslot_scale();
    if (scale > 1) {
      // Lighter blue for Korean mode
      Uint32 light_blue = SDL_MapRGB(surface->format, 100, 140, 200);
      SDL_FillRect(surface, &framerect, light_blue);
    } else {
      GUI *gui = GUI::get_gui();
      SDL_FillRect(surface, &framerect, gui->get_selected_color()->sdl_color);
    }
   }
 else
   SDL_FillRect(surface, &framerect, background_color.sdl_color);

 if(thumbnail)
   {
    int scale = get_saveslot_scale();
    if (scale > 1) {
      // Scale thumbnail to match Korean mode scaling
      SDL_Rect src_rect = {0, 0, (Uint16)thumbnail->w, (Uint16)thumbnail->h};
      SDL_Rect dst_rect = {area.x, area.y, (Uint16)(thumbnail->w * scale), (Uint16)(thumbnail->h * scale)};
      SDL_BlitScaled(thumbnail, &src_rect, surface, &dst_rect);
    } else {
      SDL_BlitSurface(thumbnail, NULL, surface, &destrect);
    }
   }

 DisplayChildren();

}

GUI_status SaveSlot::KeyDown(SDL_Keysym key)
{

 //if(key.sym == SDLK_ESCAPE)
 //  return GUI_YUM;

 /*
 return no_callback_object->callback(YESNODIALOG_CB_NO, this, this);
 */
 return GUI_PASS;
}

GUI_status SaveSlot::MouseWheel(sint32 x,sint32 y)
{
 SaveManager *save_manager = Game::get_game()->get_save_manager();
 if(y > 0)
 {
  save_manager->get_dialog()->get_scroller()->move_up();
 }
 else if(y < 0)
 {
  save_manager->get_dialog()->get_scroller()->move_down();
 }
 return GUI_YUM;
}

GUI_status SaveSlot::MouseDown(int x, int y, int button)
{
 if(selected != true)
   {
    selected = true;
    callback_object->callback(SAVESLOT_CB_SELECTED, this, NULL);
   }

 return GUI_YUM;
}

GUI_status SaveSlot::MouseUp(int x, int y, int button)
{
 return GUI_PASS;
}

GUI_status SaveSlot::callback(uint16 msg, GUI_CallBack *caller, void *data)
{
 if(msg == TEXTINPUT_CB_TEXT_READY) //we should probably make sure the caller is a GUI_TextInput object
   {
    return callback_object->callback(SAVESLOT_CB_SAVE, this, this);
   }

 return GUI_PASS;
}
