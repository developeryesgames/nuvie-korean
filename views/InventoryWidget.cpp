/*
 *  InventoryWidget.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Mon Sep 01 2003.
 *  Copyright (c) 2003 The Nuvie Team. All rights reserved.
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
#include <cassert>
#include "nuvieDefs.h"
#include "U6LList.h"
#include "Configuration.h"

#include "GUI.h"
#include "GamePalette.h"
#include "InventoryWidget.h"
#include "Actor.h"
#include "Font.h"
#include "GameClock.h"
#include "Event.h"
#include "MsgScroll.h"
#include "KoreanTranslation.h"
#include "TimedEvent.h"
#include "UseCode.h"
#include "MapWindow.h"
#include "Player.h"
#include "CommandBar.h"

#include "InventoryFont.h"
#include "ViewManager.h"
#include "FontManager.h"

#define USE_BUTTON 1 /* FIXME: put this in a common location */
#define ACTION_BUTTON 3
#define DRAG_BUTTON 1

static SDL_Rect arrow_rects[2] = {{0,16,8,8},{0,3*16+8,8,8}};

InventoryWidget::InventoryWidget(Configuration *cfg, GUI_CallBack *callback): GUI_Widget(NULL, 0, 0, 0, 0)
{
 config = cfg;
 callback_object = callback;

 container_obj = NULL; tile_manager = NULL; obj_manager = NULL;
 selected_obj = NULL; font = NULL; actor = NULL;
 target_obj = NULL; target_cont = NULL; empty_tile = NULL;
 ready_obj = NULL; // FIXME: this is unused but I might need it again -- SB-X
 row_offset = 0;
 
 config->value("config/GameType",game_type);
}

InventoryWidget::~InventoryWidget()
{

}

bool InventoryWidget::init(Actor *a, uint16 x, uint16 y, TileManager *tm, ObjManager *om, Font *f)
{
 tile_manager = tm;
 obj_manager = om;
 font = f;

 bg_color = Game::get_game()->get_palette()->get_bg_color();
 obj_font_color = 0;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int scale = use_4x ? 4 : 1;

 if(Game::get_game()->get_game_type()==NUVIE_GAME_U6)
 {
	icon_x = 32 * scale;
	obj_font_color = 0x48;
	objlist_offset_x = 8 * scale;
 }
 else
 {
	icon_x = 23 * scale;
	objlist_offset_x = 0;
 }
 objlist_offset_y = 16 * scale;

 if(Game::get_game()->get_game_type() == NUVIE_GAME_U6)
 {
	empty_tile = tile_manager->get_tile(410);
	if(use_4x) {
	  GUI_Widget::Init(NULL, x, y, 72 * 4, 64 * 4); //72 =  4 * 16 + 8
	} else {
	  GUI_Widget::Init(NULL, x, y, 72, 64); //72 =  4 * 16 + 8
	}
 }
 else if(Game::get_game()->get_game_type() == NUVIE_GAME_MD) // FIXME: different depending on npc
 {
	empty_tile = tile_manager->get_tile(273);
	GUI_Widget::Init(NULL, x, y, 64, 82);
 }
 else // SE
 {
	empty_tile = tile_manager->get_tile(392);
	GUI_Widget::Init(NULL, x+2, y, 64, 82);
 }

 set_actor(a);
 set_accept_mouseclick(true, USE_BUTTON); // accept [double]clicks from button1 (even if double-click disabled we need clicks)

 return true;
}

void InventoryWidget::set_actor(Actor *a)
{
 actor = a;
 container_obj = NULL;
 Redraw();
}

Obj *InventoryWidget::get_prev_container()
{
	if(container_obj && container_obj->get_engine_loc() == OBJ_LOC_CONT)
		return (Obj *)container_obj->parent;

	return NULL;
}

void InventoryWidget::set_prev_container()
{
  if(!container_obj)
    return;
  
  if(container_obj->get_engine_loc() == OBJ_LOC_CONT)
    set_container((Obj *)container_obj->parent);
  else
    set_container(NULL);

  return;
}

void InventoryWidget::Display(bool full_redraw)
{
 if(full_redraw || update_display)
  {
//   screen->fill(bg_color, area.x, area.y, area.w, area.h);
   display_inventory_container();
   if(Game::get_game()->get_game_type() == NUVIE_GAME_U6)
     display_arrows();
  }
   //screen->blit(area.x+40,area.y+16,portrait_data,8,56,64,56,false);

 //clear the screen first inventory icons, 4 x 3 tiles
// screen->fill(bg_color, area.x +objlist_offset_x, area.y + objlist_offset_y, 16 * 4, 16 * 3); // doesn't seem to be needed
 display_inventory_list();

 if(full_redraw || update_display)
  {
   update_display = false;
   screen->update(area.x, area.y, area.w, area.h);
  }
 else
  {
   screen->update(area.x+objlist_offset_x,area.y+objlist_offset_y,area.w-objlist_offset_x,area.h-objlist_offset_y); // update only the inventory list
  }

}

//either an Actor or an object container.
void InventoryWidget::display_inventory_container()
{
 Tile *tile;
 if(!container_obj) //display actor
   tile = tile_manager->get_tile(actor->get_downward_facing_tile_num());
 else // display container object
   tile = tile_manager->get_tile(obj_manager->get_obj_tile_num(container_obj)+container_obj->frame_n);

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();

 if(use_4x) {
   screen->blit4x(area.x+icon_x, area.y, tile->data, 8, 16, 16, 16, true);
 } else {
   screen->blit(area.x+icon_x, area.y, tile->data, 8, 16, 16, 16, true);
 }

 return;
}

void InventoryWidget::display_inventory_list()
{
 const Tile *tile;
 U6LList *inventory;
 U6Link *link;
 Obj *obj = NULL;
 uint16 i,j;
 uint16 skip_num;
 int max_rows = 4;
 if(Game::get_game()->get_game_type() == NUVIE_GAME_U6)
   max_rows = 3;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int tile_size = use_4x ? 64 : 16;

 if(container_obj)
   inventory = container_obj->container;
 else
   inventory = actor->get_inventory_list();
 if(inventory == NULL)
   link = NULL;
 else
   link = inventory->start();

 //skip row_offset rows of objects.
 skip_num = row_offset * 4;
 for(i=0;link != NULL && i < skip_num; link = link->next)
   {
    obj = (Obj *)link->data;
    if(obj->is_readied() == false)
      i++;
   }





  for(i=0;i<max_rows;i++)
   {
    for(j=0;j<4;j++)
      {
       if(link != NULL)
         {
          obj = (Obj *)link->data;
          if(obj->is_readied()) //skip any readied objects
            {
             for(;link != NULL && obj->is_readied(); link = link->next)
                obj = (Obj *)link->data;
            }
          else
            link = link->next;

          tile = tile_manager->get_tile(obj_manager->get_obj_tile_num(obj)+obj->frame_n);
          if(link == NULL)
            {
             if(obj->is_readied()) //last object is readied so skip it.
                tile = empty_tile;
            }
         }
        else
          tile = empty_tile;

       //tile = tile_manager->get_tile(actor->indentory_tile());
       if(tile == empty_tile) {
         if(use_4x) {
           screen->blit4x((area.x+objlist_offset_x)+j*tile_size,area.y+objlist_offset_y+i*tile_size,(unsigned char *)empty_tile->data,8,16,16,16,true);
         } else {
           screen->blit((area.x+objlist_offset_x)+j*tile_size,area.y+objlist_offset_y+i*tile_size,(unsigned char *)empty_tile->data,8,16,16,16,true);
         }
       }

       // Draw tile first
       if(use_4x) {
         screen->blit4x((area.x+objlist_offset_x)+j*tile_size,area.y+objlist_offset_y+i*tile_size,(unsigned char *)tile->data,8,16,16,16,true);
       } else {
         screen->blit((area.x+objlist_offset_x)+j*tile_size,area.y+objlist_offset_y+i*tile_size,(unsigned char *)tile->data,8,16,16,16,true);
       }

       // Draw qty/special char AFTER tile so they appear on top
       if(tile != empty_tile)
        {
         //draw qty string for stackable items
         if(obj_manager->is_stackable(obj))
           display_qty_string((area.x+objlist_offset_x)+j*tile_size,area.y+objlist_offset_y+i*tile_size,obj->qty);

         //draw special char for Keys.
         if(game_type == NUVIE_GAME_U6 && obj->obj_n == 64)
           display_special_char((area.x+objlist_offset_x)+j*tile_size,area.y+objlist_offset_y+i*tile_size,obj->quality);
        }
      }
   }
}

void InventoryWidget::display_qty_string(uint16 x, uint16 y, uint16 qty)
{
 uint8 len, i, offset;
 char buf[6];

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int scale = use_4x ? 4 : 1;

 sprintf(buf,"%d",qty);
 len = strlen(buf);

 offset = ((16 - len*4) / 2) * scale;

 if(use_4x) {
   // 4x scaled: each 3x5 bitmap becomes 12x20
   for(i=0;i<len;i++)
    screen->blitbitmap4x(x+offset+4*4*i,y+11*4,inventory_font[buf[i]-48],3,5,obj_font_color,bg_color);
 } else {
   for(i=0;i<len;i++)
    screen->blitbitmap(x+offset+4*i,y+11,inventory_font[buf[i]-48],3,5,obj_font_color,bg_color);
 }

 return;
}

void InventoryWidget::display_special_char(uint16 x, uint16 y, uint8 quality)
{
 if(quality + 9 >= NUVIE_MICRO_FONT_COUNT)
   return;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();

 if(use_4x) {
   screen->blitbitmap4x(x+6*4,y+11*4,inventory_font[quality + 9],3,5,obj_font_color,bg_color);
 } else {
   screen->blitbitmap(x+6,y+11,inventory_font[quality + 9],3,5,obj_font_color,bg_color);
 }
}

// Up arrow bitmap (5x5)
static const unsigned char up_arrow_bitmap[25] = {
  0,0,1,0,0,
  0,1,1,1,0,
  1,1,1,1,1,
  0,0,1,0,0,
  0,0,1,0,0
};

// Down arrow bitmap (5x5)
static const unsigned char down_arrow_bitmap[25] = {
  0,0,1,0,0,
  0,0,1,0,0,
  1,1,1,1,1,
  0,1,1,1,0,
  0,0,1,0,0
};

void InventoryWidget::display_arrows()
{
 uint32 num_objects;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int scale = use_4x ? 4 : 1;

 if(is_showing_container())
 {
	 if(container_obj->container)
		 num_objects = container_obj->container->count();
	 else
		 num_objects = 0;
 }
 else
	 num_objects = actor->inventory_count_objects(false);

 if(num_objects <= 12) //reset row_offset if we only have one page of objects
   row_offset = 0;

 if(use_4x) {
   // 4x mode: use scaled bitmap arrows
   if(row_offset > 0) //display top arrow
      screen->blitbitmap4x(area.x, area.y + 16 * 4 + 4, up_arrow_bitmap, 5, 5, obj_font_color, bg_color);

   if(num_objects - row_offset * 4 > 12) //display bottom arrow
      screen->blitbitmap4x(area.x, area.y + (3 * 16 + 8) * 4 + 4, down_arrow_bitmap, 5, 5, obj_font_color, bg_color);
 } else {
   if(row_offset > 0) //display top arrow
      font->drawChar(screen, 24, area.x, area.y + 16);

   if(num_objects - row_offset * 4 > 12) //display bottom arrow
      font->drawChar(screen, 25, area.x, area.y + 3 * 16 + 8);
 }
}

GUI_status InventoryWidget::MouseDown(int x, int y, int button)
{
 Event *event = Game::get_game()->get_event();
 CommandBar *command_bar = Game::get_game()->get_command_bar();

 // Scale input coordinates to match widget coordinates (Korean 4x mode)
 SDL_Surface *sdl_surface = screen->get_sdl_surface();
 int logical_w = screen->get_width();
 int coord_scale = (logical_w > 0) ? (sdl_surface->w / logical_w) : 1;
 if (coord_scale < 1) coord_scale = 1;
 x *= coord_scale;
 y *= coord_scale;

 // Check for Korean 4x mode for hit test sizes
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int hit_scale = use_4x ? 4 : 1;

 x -= area.x;
 y -= area.y;

 if( y < 17 * hit_scale)
    return GUI_PASS;

 Obj *obj = get_obj_at_location(x,y);
 
 if(button == ACTION_BUTTON && event->get_mode() == MOVE_MODE
    && command_bar->get_selected_action() > 0) // Exclude attack mode too
 {
    if(command_bar->try_selected_action() == false) // start new action
        return GUI_PASS; // false if new event doesn't need target
 }
 if(button == ACTION_BUTTON && command_bar->get_selected_action() > 0
    && event->get_mode() == INPUT_MODE)
 {
    if(obj)
       event->select_obj(obj); // the returned location
    else
    {
       { KoreanTranslation *kt = Game::get_game()->get_korean_translation(); Game::get_game()->get_scroll()->display_string((kt && kt->isEnabled()) ? "아무것도 없음!\n" : "nothing!\n"); }
       event->endAction(true);
       event->set_mode(MOVE_MODE);
    }
    return  GUI_PASS;
 }

 // ABOEING
 if(actor && (button == USE_BUTTON || button == ACTION_BUTTON || button == DRAG_BUTTON))
   {
    if(obj) // FIXME: duplicating code in DollWidget
      {
       // send to View
       if(callback_object->callback(INVSELECT_CB, this, obj) == GUI_PASS
          && button == DRAG_BUTTON)
           selected_obj = obj; // start dragging
       return GUI_YUM;
      }
   }
 return GUI_PASS;
}

inline uint16 InventoryWidget::get_list_position(int x, int y)
{
 uint16 list_pos;

 // Check for Korean 4x mode
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int tile_size = use_4x ? 64 : 16;

 list_pos = ((y - objlist_offset_y) / tile_size) * 4 + (x - objlist_offset_x) / tile_size;
 list_pos += row_offset * 4;

 return list_pos;
}

Obj *InventoryWidget::get_obj_at_location(int x, int y)
{
 uint8 location;
 U6LList *inventory;
 U6Link *link;
 Obj *obj =  NULL;
 uint16 i;

 if(x >= objlist_offset_x && y >= objlist_offset_y)
   {
    location = get_list_position(x,y); //find the postion of the object we hit in the inventory

    if(container_obj)
      inventory = container_obj->container;
    else
      inventory = actor->get_inventory_list();
    if(inventory == NULL)
      link = NULL;
    else
      link = inventory->start();

    for(i=0;link != NULL && i <= location;link=link->next)
     {
      obj = (Obj *)link->data;
      if(obj->is_readied() == false)
        i++;
     }

    if(i > location && obj && obj->is_readied() == false) // don't return readied or non existent objects
      return obj;
   }

 return NULL;
}

GUI_status InventoryWidget::MouseWheel(sint32 x, sint32 y)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	int xpos, ypos;
    screen->get_mouse_location(&xpos, &ypos);

    xpos -= area.x;
    ypos -= area.y;
	if(xpos < 0 || ypos > area.y + area.h - 10)
		return GUI_PASS; // goes to InventoryView
#endif
    if(Game::get_game()->get_game_type() == NUVIE_GAME_U6)
    {
        if (y > 0)
        {
            up_arrow();
        }
        if (y < 0)
        {
            down_arrow();
        }

        selected_obj = NULL;
    }
    return GUI_YUM;
}

// change container, ready/unready object, activate arrows
GUI_status InventoryWidget::MouseUp(int x,int y,int button)
{

 CommandBar *command_bar = Game::get_game()->get_command_bar();

 // Scale input coordinates to match widget coordinates (Korean 4x mode)
 SDL_Surface *sdl_surface = screen->get_sdl_surface();
 int logical_w = screen->get_width();
 int coord_scale = (logical_w > 0) ? (sdl_surface->w / logical_w) : 1;
 if (coord_scale < 1) coord_scale = 1;
 x *= coord_scale;
 y *= coord_scale;

 // Check for Korean 4x mode for hit test sizes
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_4x = font_manager && font_manager->is_korean_enabled() &&
               font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 int hit_scale = use_4x ? 4 : 1;

 if(button == USE_BUTTON || (button == ACTION_BUTTON
    && command_bar->get_selected_action() > 0)) // Exclude attack mode too
   {
    x -= area.x;
    y -= area.y;
    if(x >= icon_x && x <= icon_x + 15 * hit_scale && // hit top icon either actor or container
       y >= 0 && y <= 15 * hit_scale)
      {
		Event *event = Game::get_game()->get_event();

		if(button == ACTION_BUTTON && event->get_mode() == MOVE_MODE)
		{
			if(command_bar->try_selected_action() == false) // start new action
				return GUI_PASS; // false if new event doesn't need target
		}

		if(event->can_target_icon())
		{
			if(is_showing_container() && event->get_last_mode() != PUSH_MODE)
				event->select_obj((Obj *)container_obj, actor);
			else if(is_showing_container() && get_container()->get_engine_loc() == OBJ_LOC_CONT)
				event->select_obj((Obj *)get_container()->parent, actor);
			else
				event->select_actor(actor);
			 return GUI_YUM;
		}

			 if(is_showing_container())
				 set_prev_container(); //return to previous container or main actor inventory
			 else if(!event->using_control_cheat())
				 Game::get_game()->get_view_manager()->set_party_mode();

       Redraw();
      }
	if(Game::get_game()->get_game_type() == NUVIE_GAME_U6)
	{
		if(HitRect(x,y,arrow_rects[0])) //up arrow hit rect
			up_arrow();
		else if(HitRect(x,y,arrow_rects[1])) //down arrow hit rect
			down_arrow();
	}

    // only act now if objects can't be used with DoubleClick
    if(selected_obj && !Game::get_game()->get_map_window()->is_doubleclick_enabled())
        try_click();
    else if(selected_obj)
      {
        wait_for_mouseclick(USE_BUTTON);
        ready_obj = selected_obj;
      }
   }

 selected_obj = NULL;

 return GUI_YUM;
}

bool InventoryWidget::up_arrow()
{
 if(row_offset > 0)
  {
   row_offset--;
   Redraw();
   return true;
  }

 return false;
}

bool InventoryWidget::down_arrow()
{
 uint32 num_objects;
 if(container_obj)
    num_objects = container_obj->container_count_objects();
 else
    num_objects = actor->inventory_count_objects(false);

 if(num_objects - row_offset * 4 > 12)
   {
    row_offset++;
    Redraw();
    return true;
   }

 return false;
}

GUI_status InventoryWidget::MouseMotion(int x,int y,Uint8 state)
{
 Tile *tile;

 if(selected_obj && !dragging && Game::get_game()->is_dragging_enabled())
   {
    dragging = true;
    tile = tile_manager->get_tile(obj_manager->get_obj_tile_num(selected_obj)+selected_obj->frame_n);
    return gui_drag_manager->start_drag(this, GUI_DRAG_OBJ, selected_obj, tile->data, 16, 16, 8);
   }

	return GUI_PASS;
}

void InventoryWidget::drag_drop_success(int x, int y, int message, void *data)
{
 DEBUG(0,LEVEL_DEBUGGING,"InventoryWidget::drag_drop_success()\n");
 dragging = false;

// handled by drop target
// if(container_obj)
//   container_obj->container->remove(selected_obj);
// else
//   actor->inventory_remove_obj(selected_obj);

 selected_obj = NULL;
 Redraw();
}

void InventoryWidget::drag_drop_failed(int x, int y, int message, void *data)
{
 DEBUG(0,LEVEL_DEBUGGING,"InventoryWidget::drag_drop_failed()\n");
 dragging = false;
 selected_obj = NULL;
}

bool InventoryWidget::drag_set_target_obj(int x, int y)
{
	if(x >= 32 && x < 48 && y < 16)
	{
		target_obj = NULL;
		target_cont = get_prev_container(); //returns parent container or NULL if we're back at the inventory.
	}
	else if(x >= objlist_offset_x && y >= objlist_offset_y)
	{
		target_obj = get_obj_at_location(x,y);
		target_cont = get_container();
	}
	else
	{
		return false;
	}

	return true;
}

bool InventoryWidget::drag_accept_drop(int x, int y, int message, void *data)
{
 GUI::get_gui()->force_full_redraw();
 DEBUG(0,LEVEL_DEBUGGING,"InventoryWidget::drag_accept_drop()\n");
 if(message == GUI_DRAG_OBJ)
   {
    Obj *obj = (Obj*)data;
    x -= area.x;
    y -= area.y;
    if(target_obj == NULL) //we need to check this so we don't screw up target_obj on subsequent calls
    {
      if(drag_set_target_obj(x, y) == false)
      {
          DEBUG(0,LEVEL_WARNING,"InventoryWidget: Didn't hit any widget object targets!\n");
          return false;
      }
    }
    Actor *src_actor = Game::get_game()->get_player()->get_actor();

    if(obj->get_actor_holding_obj() == actor)
        src_actor = actor;
    else if(!obj->is_in_inventory() && actor == Game::get_game()->get_player()->get_actor())
    {
        Game::get_game()->get_scroll()->display_string("Get-");
        Game::get_game()->get_scroll()->display_string(obj_manager->look_obj(obj, OBJ_SHOW_PREFIX));
    }
    else
        Game::get_game()->get_event()->display_move_text(actor, obj);

    if(!obj->is_in_inventory()
       && !Game::get_game()->get_map_window()->can_get_obj(actor, obj))
    {
        Game::get_game()->get_scroll()->message("\n\nblocked\n\n");
        return false;
    }
    if((Game::get_game()->get_usecode()->has_getcode(obj)
        && !Game::get_game()->get_usecode()->get_obj(obj, actor))
       || !Game::get_game()->get_event()->can_move_obj_between_actors(obj, src_actor, actor))
    {
        Game::get_game()->get_scroll()->message("\n\n");
    	return false;
    }
    else if(!obj->is_in_inventory()
            && obj_manager->obj_is_damaging(obj, Game::get_game()->get_player()->get_actor()))
    {
        Game::get_game()->get_player()->subtract_movement_points(3);
        return false;
    }
    else if(src_actor != actor || !obj->is_in_inventory())
        Game::get_game()->get_scroll()->message("\n\n");

    if(src_actor != actor) // get plus move
         Game::get_game()->get_player()->subtract_movement_points(8);
    else if(!obj->is_in_inventory()) // get
         Game::get_game()->get_player()->subtract_movement_points(3);

    UseCode *usecode = Game::get_game()->get_usecode();
    if(usecode->is_chest(obj) && obj->frame_n == 0) //open chest
        obj->frame_n = 1; //close the chest

    DEBUG(0,LEVEL_DEBUGGING,"Drop Accepted\n");
    return true;
   }

 DEBUG(0,LEVEL_DEBUGGING,"Drop Refused\n");
 return false;
}

void InventoryWidget::drag_perform_drop(int x, int y, int message, void *data)
{
 DEBUG(0,LEVEL_DEBUGGING,"InventoryWidget::drag_perform_drop()\n");
 Obj *obj;

 x -= area.x;
 y -= area.y;

 if(message == GUI_DRAG_OBJ)
   {
    DEBUG(0,LEVEL_DEBUGGING,"Drop into inventory.\n");
    obj = (Obj *)data;

	if(target_obj && obj_manager->can_store_obj(target_obj, obj))
	{
		obj_manager->moveto_container(obj, target_obj);
	}
	else if(target_cont && obj_manager->can_store_obj(target_cont, obj))
	{
		obj_manager->moveto_container(obj, target_cont);
	}
    else
    {
    	if(obj->is_readied())
    		Game::get_game()->get_event()->unready(obj);
    	else
    		obj_manager->moveto_inventory(obj, actor);
    }

    Redraw();
   }

 Game::get_game()->get_map_window()->updateBlacking();
 target_obj = NULL;

 return;
}

void InventoryWidget::drag_draw(int x, int y, int message, void* data)
{
	Tile* tile;

	if (!selected_obj)
		return;

	tile = tile_manager->get_tile(obj_manager->get_obj_tile_num(selected_obj) + selected_obj->frame_n);

	// Get actual SDL surface dimensions (1280x800 in Korean 4x mode)
	// x, y are from get_mouse_location() in screen->get_width() coords (640x400)
	// Need to convert to actual surface coords (1280x800)
	SDL_Surface *sdl_surface = screen->get_sdl_surface();
	int screen_w = sdl_surface->w;
	int screen_h = sdl_surface->h;
	int logical_w = screen->get_width();
	int coord_scale = (logical_w > 0) ? (screen_w / logical_w) : 1;  // 1280/640=2
	if (coord_scale < 1) coord_scale = 1;
	int tile_scale = screen_w / 320;  // For tile size: 1280/320=4
	if (tile_scale < 1) tile_scale = 1;

	int tile_size = 16 * tile_scale;
	int half_tile = 8 * tile_scale;

	// Convert from logical coords to surface coords
	int nx = x * coord_scale - half_tile;
	int ny = y * coord_scale - half_tile;

	// Clamp to screen bounds
	if (nx + tile_size >= screen_w)
		nx = screen_w - tile_size - 1;
	else if (nx < 0)
		nx = 0;

	if (ny + tile_size >= screen_h)
		ny = screen_h - tile_size - 1;
	else if (ny < 0)
		ny = 0;

	if (tile_scale >= 4)
		screen->blit4x(nx, ny, tile->data, 8, 16, 16, 16, true);
	else if (tile_scale >= 2)
		screen->blit2x(nx, ny, tile->data, 8, 16, 16, 16, true);
	else
		screen->blit(nx, ny, tile->data, 8, 16, 16, 16, true);

	screen->update(nx, ny, tile_size, tile_size);
}

void InventoryWidget::try_click()
{
	Event *event = Game::get_game()->get_event();
	UseCode *usecode = Game::get_game()->get_usecode();
	if(!selected_obj)
		selected_obj = ready_obj;
	if(!selected_obj)
		return;
	bool locked_chest = (usecode->is_chest(selected_obj) && selected_obj->frame_n > 1);
	if(event->get_mode() == ATTACK_MODE)
		event->cancelAction();
	else if(usecode->is_container(selected_obj) && !locked_chest) // open up the container.
	{
		container_obj = selected_obj;
		if(usecode->is_chest(container_obj) && selected_obj->frame_n == 1)
			usecode->process_effects(container_obj, actor);
		Redraw();
	}
	else // attempt to ready selected object.
	{
		event->ready(selected_obj, actor);
		Redraw();
	}
	ready_obj = NULL;
	selected_obj = NULL;
}

/* Use object. */
GUI_status InventoryWidget::MouseDouble(int x, int y, int button)
{
    // we have to check if double-clicks are allowed here, since we use single-clicks
    if(!Game::get_game()->get_map_window()->is_doubleclick_enabled())
        return(GUI_PASS);
    Obj *obj = selected_obj;

    ready_obj = NULL;
    selected_obj = NULL;

    if(!actor)
        return(GUI_YUM);
    if(!obj)
        return(MouseUp(x, y, button)); // probably hit an arrow

    Game::get_game()->get_view_manager()->double_click_obj(obj);
    return(GUI_PASS);
}

GUI_status InventoryWidget::MouseClick(int x, int y, int button)
{
    return(MouseUp(x, y, button));
}

// change container, ready/unready object, activate arrows
GUI_status InventoryWidget::MouseDelayed(int x, int y, int button)
{
    if(ready_obj)
        try_click();
    return GUI_PASS;
}
