/*
 *  PartyView.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Fri Aug 22 2003.
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
#include <cmath>
#include "nuvieDefs.h"

#include "Actor.h"
#include "Party.h"
#include "Player.h"
#include "GameClock.h"
#include "PartyView.h"
#include "Font.h"
#include "U6Font.h"
#include "KoreanFont.h"
#include "FontManager.h"
#include "Weather.h"
#include "Script.h"
#include "MsgScroll.h"
#include "Event.h"
#include "Configuration.h"
#include "CommandBar.h"
#include "UseCode.h"
#include "MapWindow.h"
#include "SunMoonStripWidget.h"
#include "KoreanTranslation.h"

extern GUI_status inventoryViewButtonCallback(void *data);
extern GUI_status actorViewButtonCallback(void *data);

#define U6 Game::get_game()->get_game_type()==NUVIE_GAME_U6
#define SE Game::get_game()->get_game_type()==NUVIE_GAME_SE
#define MD Game::get_game()->get_game_type()==NUVIE_GAME_MD

static const uint8 ACTION_BUTTON = 3;

PartyView::PartyView(Configuration *cfg) : View(cfg)
{
 player = NULL; view_manager = NULL;
 party_view_targeting = false;
 row_offset = 0;
 sun_moon_widget = NULL;
}

PartyView::~PartyView()
{

}

bool PartyView::init(void *vm, uint16 x, uint16 y, Font *f, Party *p, Player *pl, TileManager *tm, ObjManager *om)
{
 View::init(x,y,f,p,tm,om);

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean_4x = font_manager && font_manager->is_korean_enabled() && font_manager->get_korean_font();

 // PartyView is 8px wider than other Views, for the arrows
 // ...and 3px taller, for the sky (SB-X)
 if(U6)
 {
   if(use_korean_4x && Game::get_game()->is_original_plus())
   {
     // 4x scaled: add 8*4 width and 3*4 height
     SetRect(area.x, area.y, area.w + 8*4, area.h + 3*4);
   }
   else
   {
     SetRect(area.x, area.y, area.w+8, area.h+3);
   }
 }
 else
   SetRect(area.x, area.y, area.w, area.h);

 view_manager = vm;
 player = pl;

 if(U6)
 {
   sun_moon_widget = new SunMoonStripWidget(player, tile_manager);
   if(use_korean_4x && Game::get_game()->is_original_plus())
   {
     sun_moon_widget->init(area.x, area.y); // Will need to handle 4x in SunMoonStripWidget
   }
   else
   {
     sun_moon_widget->init(area.x,area.y);
   }
   AddWidget(sun_moon_widget);
 }

 config->value("config/input/party_view_targeting", party_view_targeting, false);

 return true;
}

GUI_status PartyView::MouseWheel(sint32 x, sint32 y)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	int xpos, ypos;
    screen->get_mouse_location(&xpos, &ypos);

    xpos -= area.x;
    ypos -= area.y;
	if(xpos < 0 || ypos > area.y + area.h - 6)
		return GUI_PASS; // goes to MsgScrollw
#endif
 if(y > 0)
   {
    if(up_arrow())
      Redraw();
   }
  if(y < 0)
   {
    if(down_arrow())
      Redraw();
   }

 return GUI_YUM;
}

GUI_status PartyView::MouseUp(int x,int y,int button)
{
 x -= area.x;
 y -= area.y;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean_4x = font_manager && font_manager->is_korean_enabled() &&
                      font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int scale = use_korean_4x ? 4 : 1;

 if(y < 18 * scale && U6) // clicked on skydisplay
   return GUI_PASS;
 if(y < 4 && MD)
   return GUI_PASS;

 int rowH = 16 * scale;
 if(MD)
     rowH = 24;

 uint8 party_size = party->get_party_size();
 if(SE)
 {
     if(party_size > 7) party_size = 7;
 }
 else if(party_size > 5) party_size = 5; // can only display/handle 5 at a time

 SDL_Rect arrow_rects_U6[2] = {{0, (Sint16)(18*scale), (Uint16)(8*scale), (Uint16)(8*scale)},
                               {0, (Sint16)(90*scale), (Uint16)(8*scale), (Uint16)(8*scale)}};
 SDL_Rect arrow_rects[2] = {{0,6,7,8},{0,102,7,8}};
 SDL_Rect arrow_up_rect_MD[1] = {{0,15,7,8}};

 if(HitRect(x,y,U6? arrow_rects_U6[0]: (MD ? arrow_up_rect_MD[0] : arrow_rects[0]))) //up arrow hit rect
   {
    if(up_arrow())
      Redraw();
    return GUI_YUM;
   }
  if(HitRect(x,y,U6? arrow_rects_U6[1]: arrow_rects[1])) //down arrow hit rect
   {
    if(down_arrow())
      Redraw();
    return GUI_YUM;
   }

 int x_offset = 7 * scale;
 int y_offset = 18 * scale;
 if(SE)
 {
     x_offset = 6;
     y_offset = 2;
 }
 else if(MD)
     y_offset = 4;
 if(y > party_size * rowH + y_offset-1) // clicked below actors
   return GUI_YUM;

 if(x >= x_offset)
  {
   Event *event = Game::get_game()->get_event();
   CommandBar *command_bar = Game::get_game()->get_command_bar();

   if(button == ACTION_BUTTON && event->get_mode() == MOVE_MODE
      && command_bar->get_selected_action() > 0) // Exclude attack mode too
   {
      if(command_bar->try_selected_action() == false) // start new action
         return GUI_PASS; // false if new event doesn't need target
   }
   if((party_view_targeting || (button == ACTION_BUTTON
      && command_bar->get_selected_action() > 0)) && event->can_target_icon())
   {
      x += area.x;
      y += area.y;
      Actor *actor = get_actor(x, y);
      if(actor)
      {
         event->select_actor(actor);
         return GUI_YUM;
      }
   }
   set_party_member(((y - y_offset) / rowH) + row_offset);
   if(x >= x_offset + 17 * scale) // clicked an actor name
     {
      actorViewButtonCallback(view_manager);
     }
   else // clicked an actor icon
     {
      inventoryViewButtonCallback(view_manager);
     }
  }
 return GUI_YUM;
}

Actor *PartyView::get_actor(int x, int y)
{
    x -= area.x;
    y -= area.y;

    uint8 party_size = party->get_party_size();
    int rowH = 16;
    int y_offset = 18;
    if(MD)
    {
        rowH = 24; y_offset = 0;
    }
    if(SE)
    {
        y_offset = 2;
        if(party_size > 7) party_size = 7;
    }
    else if(party_size > 5) party_size = 5; // can only display/handle 5 at a time

    if(y > party_size * rowH + y_offset) // clicked below actors
      return NULL;

    if(x >= 8)
     {
    	return party->get_actor(((y - y_offset) / rowH) + row_offset);
     }

    return NULL;
}

bool PartyView::drag_accept_drop(int x, int y, int message, void *data)
{
	GUI::get_gui()->force_full_redraw();
	DEBUG(0,LEVEL_DEBUGGING,"PartyView::drag_accept_drop()\n");
	if(message == GUI_DRAG_OBJ)
	{
		MsgScroll *scroll = Game::get_game()->get_scroll();

		Obj *obj = (Obj*)data;
		Actor *actor = get_actor(x, y);
		if(actor)
		{
			Event *event = Game::get_game()->get_event();
			event->display_move_text(actor, obj);
			if(!obj->is_in_inventory()
               && !Game::get_game()->get_map_window()->can_get_obj(actor, obj))
			{
				Game::get_game()->get_scroll()->message("\n\nblocked\n\n");
				return false;
			}
			if((!Game::get_game()->get_usecode()->has_getcode(obj)
			    || Game::get_game()->get_usecode()->get_obj(obj, actor))
			   && event->can_move_obj_between_actors(obj, player->get_actor(), actor))
			{
				if(actor == player->get_actor()) // get
					player->subtract_movement_points(3);
				else // get plus move
					player->subtract_movement_points(8);
				if(!obj->is_in_inventory() &&
				   obj_manager->obj_is_damaging(obj, Game::get_game()->get_player()->get_actor()))
					return false;
				DEBUG(0,LEVEL_DEBUGGING,"Drop Accepted\n");
				return true;
			}
		}
		scroll->display_string("\n\n");
	    scroll->display_prompt();
	}

	Redraw();

	DEBUG(0,LEVEL_DEBUGGING,"Drop Refused\n");
	return false;
}

void PartyView::drag_perform_drop(int x, int y, int message, void *data)
{
 DEBUG(0,LEVEL_DEBUGGING,"InventoryWidget::drag_perform_drop()\n");
 Obj *obj;

 if(message == GUI_DRAG_OBJ)
   {
    DEBUG(0,LEVEL_DEBUGGING,"Drop into inventory.\n");
    obj = (Obj *)data;

	Actor *actor = get_actor(x, y);
	if(actor)
	{
		obj_manager->moveto_inventory(obj, actor);
	}
    MsgScroll *scroll = Game::get_game()->get_scroll();
    scroll->display_string("\n\n");
    scroll->display_prompt();

    Redraw();
   }

 return;
}

void PartyView::Display(bool full_redraw)
{

 if(full_redraw || update_display || MD || Game::get_game()->is_original_plus_full_map() || Game::get_game()->is_original_plus())
  {
   uint8 i;
   uint8 hp_text_color;
   Actor *actor;
   Tile *actor_tile;
   char *actor_name;
   std::string translated_name;  // Buffer for Korean translated name
   char hp_string[4];
   uint8 party_size = party->get_party_size();

   // Check for Korean 4x mode (UI is 4x, font is 32x32 native)
   FontManager *font_manager = Game::get_game()->get_font_manager();
   KoreanFont *korean_font = font_manager ? font_manager->get_korean_font() : NULL;
   bool use_korean_4x = korean_font && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();

   int rowH = 16;
   int scale = 1;
   int font_scale = 1;
   if(use_korean_4x)
   {
     rowH = 16 * 4; // 64px per row at 4x UI scale
     scale = 4;     // UI scale
     font_scale = 1; // Korean font: 32x32 native
   }
   else if(MD)
      rowH = 24;

   update_display = false;
   uint8 end_offset = row_offset + 5;

   static int pv_debug = 0;
   if(pv_debug < 3) {
     DEBUG(0, LEVEL_INFORMATIONAL, "PartyView::Display area: x=%d y=%d w=%d h=%d\n", area.x, area.y, area.w, area.h);
     pv_debug++;
   }

   if(MD)
      fill_md_background(bg_color, area);
   else
      screen->fill(bg_color, area.x, area.y, area.w, area.h);
   //if(U6)
     // display_sun_moon_strip();

   display_arrows();

   if(SE)
	   end_offset = row_offset + 7;
   if(end_offset > party_size)
	   end_offset = party_size;

   for(i=row_offset;i<end_offset;i++)
     {
      actor = party->get_actor(i);
      actor_tile = tile_manager->get_tile(actor->get_downward_facing_tile_num());

      int x_offset = 8 * scale;
      int y_offset = 18 * scale;
      hp_text_color = 0; //standard text color

      if(U6)
        hp_text_color = 0x48; //standard text color
      if (SE)
      {
        x_offset = 6; y_offset = 1;
      }
      if(MD)
      {
        x_offset = 8;
        y_offset = 6;
        GameClock *clock = Game::get_game()->get_clock();
        if(clock->get_purple_berry_counter(actor->get_actor_num()) > 0)
        {
          screen->blit(area.x+x_offset+16,area.y + y_offset + (i-row_offset)*rowH,tile_manager->get_tile(TILE_MD_PURPLE_BERRY_MARKER)->data,8,16,16,16,true);
        }
        if(clock->get_green_berry_counter(actor->get_actor_num()) > 0)
        {
          screen->blit(area.x+x_offset+32,area.y + y_offset + (i-row_offset)*rowH,tile_manager->get_tile(TILE_MD_GREEN_BERRY_MARKER)->data,8,16,16,16,true);
        }
        if(clock->get_brown_berry_counter(actor->get_actor_num()) > 0)
        {
          screen->blit(area.x+x_offset+32,area.y + y_offset + (i-row_offset)*rowH,tile_manager->get_tile(TILE_MD_BROWN_BERRY_MARKER)->data,8,16,16,16,true);
        }

      }

      // Draw actor tile (sprite) - use 4x scaling in Korean mode
      if(use_korean_4x)
      {
        screen->blit4x(area.x+x_offset, area.y + y_offset + (i-row_offset)*rowH, actor_tile->data, 8, 16, 16, 16, true);
      }
      else
      {
        screen->blit(area.x+x_offset,area.y + y_offset + (i-row_offset)*rowH,actor_tile->data,8,16,16,16,true);
      }
      actor_name = party->get_actor_name(i);

      // Translate actor name if Korean mode is enabled
      const char *display_name = actor_name;
      if(use_korean_4x)
      {
        KoreanTranslation *korean = Game::get_game()->get_korean_translation();
        if(korean && korean->isEnabled())
        {
          translated_name = korean->translate(actor_name);
          display_name = translated_name.c_str();
        }
      }

      if(SE)
      {
        x_offset = 4; y_offset = 0;
      }
      if(MD)
        y_offset = -3;

      // Draw actor name - use Korean font with 2x scaling (16x16*2=32)
      if(use_korean_4x)
      {
        // In 4x mode: name position is (x_offset + 24*4) from area.x
        korean_font->drawStringUTF8(screen, display_name,
          area.x + x_offset + 24 * scale,
          area.y + y_offset + (i-row_offset) * rowH + 8 * scale,
          font->getDefaultColor(), 0, font_scale);
      }
      else
      {
        // FIXME: Martian Dreams text is somewhat center aligned
        font->drawString(screen, actor_name, area.x + x_offset + 24, area.y + y_offset + (i-row_offset) * rowH + 8);
      }

      sprintf(hp_string,"%3d",actor->get_hp());
      hp_text_color = actor->get_hp_text_color();
      if(SE)
      {
        x_offset = -7; y_offset = 3;
      }
      if(MD)
      {
        x_offset = -16; y_offset = 14;
      }

      // Draw HP string
      // For U6: x_offset stays at initial value (8*scale), y_offset stays at 18*scale
      // SE/MD modify x_offset and y_offset above
      if(use_korean_4x)
      {
        // U6 Korean: Right-align HP at right edge of view
        // Calculate string width and position from right edge
        uint16 hp_width = korean_font->getStringWidthUTF8(hp_string, font_scale);
        // Right edge is area.x + area.w, place HP with small margin
        int hp_x = area.x + area.w - hp_width - 8;
        korean_font->drawStringUTF8(screen, hp_string,
          hp_x,
          area.y + y_offset + (i-row_offset) * rowH,
          hp_text_color, 0, font_scale);
      }
      else
      {
        font->drawString(screen, hp_string, strlen(hp_string), area.x + x_offset + 112, area.y + y_offset + (i-row_offset) * rowH, hp_text_color, 0);
      }
     }
   DisplayChildren(full_redraw);
   screen->update(area.x, area.y, area.w, area.h);
  }

 return;
}

bool PartyView::up_arrow()
{
    if(row_offset > 0)
    {
        row_offset--;
        return(true);
    }
    return(false);
}


bool PartyView::down_arrow()
{
    if((row_offset+ (SE? 7:5)) < party->get_party_size())
    {
        row_offset++;
        return(true);
    }
    return(false);
}


void PartyView::display_arrows()
{
    // Check for Korean 4x mode
    FontManager *font_manager = Game::get_game()->get_font_manager();
    KoreanFont *korean_font = font_manager ? font_manager->get_korean_font() : NULL;
    bool use_korean_4x = korean_font && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();

    int x_offset = 0; int y_offset = 0;
    int scale = use_korean_4x ? 4 : 1;
    int font_scale = use_korean_4x ? 1 : 1; // Korean font: 32x32 native

    if(SE || MD)
    {
        x_offset = 2;
        y_offset = 12;
    }
    uint8 max_party_size = 5;
    uint8 party_size = party->get_party_size();
    if(SE)
        max_party_size = 7;
    if(party_size <= max_party_size) // reset
        row_offset = 0;

    if((party_size - row_offset) > max_party_size) // display bottom arrow
    {
        if(use_korean_4x)
        {
            // Use U6 font's special arrow character with 4x scaling
            U6Font *u6font = static_cast<U6Font*>(font);
            u6font->drawCharScaled(screen, 25, area.x - x_offset, area.y + 90 * scale + y_offset, font->getDefaultColor(), scale);
        }
        else
            font->drawChar(screen, 25, area.x - x_offset, area.y + 90 + y_offset);
    }
    if(MD)
        y_offset = 3;
    if(row_offset > 0) // display top arrow
    {
        if(use_korean_4x)
        {
            // Use U6 font's special arrow character with 4x scaling
            U6Font *u6font = static_cast<U6Font*>(font);
            u6font->drawCharScaled(screen, 24, area.x - x_offset, area.y + 18 * scale - y_offset, font->getDefaultColor(), scale);
        }
        else
            font->drawChar(screen, 24, area.x - x_offset, area.y + 18 - y_offset);
    }
}
// </SB-X> 
