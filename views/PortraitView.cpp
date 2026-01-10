/*
 *  PortraitView.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Tue May 20 2003.
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

#include <string>

#include "nuvieDefs.h"

#include "Configuration.h"
#include "U6misc.h"
#include "U6Lib_n.h"
#include "U6Shape.h"

#include "Game.h"
#include "Actor.h"
#include "Portrait.h"
#include "Font.h"
#include "FontManager.h"
#include "KoreanFont.h"
#include "ViewManager.h"
#include "MsgScroll.h"
#include "GUI.h"
#include "DollWidget.h"
#include "PortraitView.h"
#include "SunMoonStripWidget.h"
#include "KoreanTranslation.h"


PortraitView::PortraitView(Configuration *cfg) : View(cfg)
{
 portrait_data = NULL; portrait = NULL;
 bg_data = NULL;
 name_string = new string; show_cursor = false;
 doll_widget = NULL; waiting = false; display_doll = false;
 cur_actor_num = 0;
 gametype = get_game_type(cfg);

 //FIXME: Portraits in SE/MD are different size than in U6! 79x85 76x83
 switch(gametype)
 {
   case NUVIE_GAME_U6: portrait_width=56; portrait_height=64; break;
   case NUVIE_GAME_SE: portrait_width=79; portrait_height=85; break;
   case NUVIE_GAME_MD: portrait_width=76; portrait_height=83; break;
 }
}

PortraitView::~PortraitView()
{
 if(portrait_data != NULL)
   free(portrait_data);

 if(bg_data != NULL)
   delete bg_data;

 delete name_string;
}

bool PortraitView::init(uint16 x, uint16 y, Font *f, Party *p, Player *player, TileManager *tm, ObjManager *om, Portrait *port)
{
 View::init(x,y,f,p,tm,om);

 portrait = port;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();

 doll_widget = new DollWidget(config, this);
 // Original: (0, 16), 4x mode: (0, 16*4=64)
 int doll_y = use_4x ? 16 * 4 : 16;
 doll_widget->init(NULL, 0, doll_y, tile_manager, obj_manager, true);

 AddWidget(doll_widget);
 doll_widget->Hide();

 if(gametype == NUVIE_GAME_U6)
 {
   SunMoonStripWidget *sun_moon_widget = new SunMoonStripWidget(player, tile_manager);
   // In 4x mode, scale the SunMoon position
   if(use_4x)
     sun_moon_widget->init(-8 * 4, -2 * 4);
   else
     sun_moon_widget->init(-8, -2);
   AddWidget(sun_moon_widget);
 }
 else if(gametype == NUVIE_GAME_MD)
 {
   load_background("mdscreen.lzc", 1);
 }
 else if(gametype == NUVIE_GAME_SE)
 {
   load_background("bkgrnd.lzc", 0);
 }

 return true;
}


void PortraitView::load_background(const char *f, uint8 lib_offset)
{
  U6Lib_n file;
  bg_data = new U6Shape();
  std::string path;
  config_get_path(config,f,path);
  file.open(path,4,gametype);
  unsigned char *temp_buf = file.get_item(lib_offset);
  bg_data->load(temp_buf + 8);
  free(temp_buf);
}

void PortraitView::Display(bool full_redraw)
{
 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font = font_manager ? font_manager->get_korean_font() : NULL;
 bool use_4x = korean_font && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 int scale = use_4x ? 4 : 1;

 // In Korean 4x mode or new_style/full_map, always fill background
 if(Game::get_game()->is_new_style() || Game::get_game()->is_original_plus_full_map() || use_4x)
   screen->fill(bg_color, area.x, area.y, area.w, area.h);
 if(portrait_data != NULL/* && (full_redraw || update_display)*/)
  {
   update_display = false;
   if(gametype == NUVIE_GAME_U6)
   {
     if(display_doll)
     {
       if(use_4x) {
         // 4x mode: portrait at 72*4, 16*4 from area origin
         screen->blit4x(area.x+72*scale, area.y+16*scale, portrait_data, 8, portrait_width, portrait_height, portrait_width, false);
       } else {
         screen->blit(area.x+72,area.y+16,portrait_data,8,portrait_width,portrait_height,portrait_width,false);
       }
     }
     else
     {
       if(use_4x) {
         // 4x mode: centered portrait
         screen->blit4x(area.x+(area.w-portrait_width*scale)/2, area.y+(area.h-portrait_height*scale)/2,
                        portrait_data, 8, portrait_width, portrait_height, portrait_width, true);
       } else {
         screen->blit(area.x+(area.w-portrait_width)/2,area.y+(area.h-portrait_height)/2,portrait_data,8,portrait_width,portrait_height,portrait_width,true);
       }
     }
     display_name(80);
   }
   else if(gametype == NUVIE_GAME_MD)
   {
     uint16 w,h;
     bg_data->get_size(&w, &h);
     screen->blit(area.x,area.y-2,bg_data->get_data(),8,w,h,w,true);

     screen->blit(area.x+(area.w-portrait_width)/2,area.y+6,portrait_data,8,portrait_width,portrait_height,portrait_width,true);
     display_name(100);
   }
   else if(gametype == NUVIE_GAME_SE)
   {
     uint16 w,h;
     bg_data->get_size(&w, &h);
     screen->blit(area.x,area.y,bg_data->get_data(),8,w,h,w,true);

     screen->blit(area.x+(area.w-portrait_width)/2+1,area.y+1,portrait_data,8,portrait_width,portrait_height,portrait_width,true);
     display_name(98);
   }
  }
  if(show_cursor && gametype == NUVIE_GAME_U6) // FIXME: should we be using scroll's drawCursor?
   {
    screen->fill(bg_color, area.x, area.y + area.h - 8 * scale, 8 * scale, 8 * scale);
    Game::get_game()->get_scroll()->drawCursor(area.x, area.y + area.h - 8 * scale);
   }
   DisplayChildren(full_redraw);
   screen->update(area.x, area.y, area.w, area.h);
}

bool PortraitView::set_portrait(Actor *actor, const char *name)
{
 if(Game::get_game()->is_new_style())
   this->Show();
 cur_actor_num = actor->get_actor_num();
 int doll_x_offset = 0;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int scale = use_4x ? 4 : 1;

 if(portrait_data != NULL)
   free(portrait_data);

 portrait_data = portrait->get_portrait_data(actor);

 if(gametype == NUVIE_GAME_U6 && actor->has_readied_objects())
   {
    if(portrait_data == NULL)
      doll_x_offset = 34 * scale;

    // Original: (0, 16), 4x mode: (0, 16*4=64)
    int doll_y = 16 * scale;
    doll_widget->MoveRelativeToParent(doll_x_offset, doll_y);

    display_doll = true;
    doll_widget->Show();
    doll_widget->set_actor(actor);
   }
 else
   {
    display_doll = false;
    doll_widget->Hide();
    doll_widget->set_actor(NULL);
    
    if(portrait_data == NULL)
      return false;
   }

 if(name == NULL)
   name = actor->get_name();
 if(name == NULL)
   name_string->assign("");  // just in case
 else
   name_string->assign(name);
 
 if(screen)
   screen->fill(bg_color, area.x, area.y, area.w, area.h);
   
 Redraw();
 return true;
}

void PortraitView::display_name(uint16 y_offset)
{
 const char *name;

 name = name_string->c_str();

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font = font_manager ? font_manager->get_korean_font() : NULL;
 bool use_4x = korean_font && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();

 if(use_4x) {
   // Translate name to Korean if available
   KoreanTranslation *korean = Game::get_game()->get_korean_translation();
   std::string display_name = (korean && korean->isEnabled()) ? korean->translate(name) : name;

   // 4x mode: use Korean font, scale y_offset
   uint16 name_width = korean_font->getStringWidthUTF8(display_name.c_str(), 1);
   korean_font->drawStringUTF8(screen, display_name.c_str(), area.x + (area.w - name_width) / 2, area.y + y_offset * 4, 0x48, 0, 1);
 } else {
   font->drawString(screen, name, area.x + (area.w - strlen(name) * 8) / 2, area.y+y_offset);
 }

 return;
}


/* On any input return to previous status view if waiting.
 * Returns true if event was used.
 */
GUI_status PortraitView::HandleEvent(const SDL_Event *event)
{
    if(waiting
       && (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_KEYDOWN))
    {
        // Check for Korean 4x mode
        FontManager *font_manager = Game::get_game()->get_font_manager();
        bool use_4x = font_manager && font_manager->is_korean_enabled() &&
                      font_manager->get_korean_font() && Game::get_game()->is_original_plus();

        if(Game::get_game()->is_new_style())
            this->Hide();
        else if(!use_4x) // In Korean 4x mode, don't switch away from portrait view
            Game::get_game()->get_view_manager()->set_inventory_mode();
        // Game::get_game()->get_scroll()->set_input_mode(false);
        Game::get_game()->get_scroll()->message("\n");
        set_waiting(false);
        return(GUI_YUM);
    }
    return(GUI_PASS);
}


/* Start/stop waiting for input to continue, and (for now) steal cursor from
 * MsgScroll.
 */
void PortraitView::set_waiting(bool state)
{
    if(state == true && display_doll == false && portrait_data == NULL) // don't wait for nothing
    {
        if(Game::get_game()->is_new_style())
            this->Hide();
        return;
    }
    waiting = state;
    set_show_cursor(waiting);
    Game::get_game()->get_scroll()->set_show_cursor(!waiting);
    Game::get_game()->get_gui()->lock_input(waiting ? this : NULL);
}
