/*
 *  ViewManager.cpp
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
#include <math.h>
#include "nuvieDefs.h"
#include "Configuration.h"
#include "U6misc.h"

#include "GUI.h"

#include "ViewManager.h"

#include "Actor.h"

#include "ActorView.h"
#include "PortraitView.h"
#include "InventoryView.h"
#include "DollViewGump.h"
#include "ContainerViewGump.h"
#include "PortraitViewGump.h"
#include "SignViewGump.h"
#include "ScrollViewGump.h"
#include "PartyView.h"
#include "SpellView.h"
#include "SpellViewGump.h"
#include "SunMoonRibbon.h"
#include "MapWindow.h"
#include "MapEditorView.h"
#include "MsgScroll.h"
#include "Party.h"
#include "Event.h"
#include "FontManager.h"
#include "Portrait.h"
#include "UseCode.h"
#include "NuvieBmpFile.h"
#include "MDSkyStripWidget.h"

ViewManager::ViewManager(Configuration *cfg)
{
 config = cfg;
 config->value("config/GameType",game_type);
 current_view = NULL; gui = NULL; font = NULL;
 tile_manager = NULL; obj_manager = NULL; party = NULL;
 portrait = NULL; actor_view = NULL; inventory_view = NULL;
 portrait_view = NULL; party_view = NULL; spell_view = NULL;
 doll_next_party_member = 0;
 ribbon = NULL;
 mdSkyWidget = NULL;
 party_view_permanent = false;
}

ViewManager::~ViewManager()
{
 // only delete the views that are not currently active
 if (current_view != actor_view)     delete actor_view;
 if (current_view != inventory_view) delete inventory_view;
 if (current_view != party_view)     delete party_view;
 if (current_view != portrait_view)  delete portrait_view;
 if (current_view != spell_view)  delete spell_view;

}

bool ViewManager::init(GUI *g, Font *f, Party *p, Player *player, TileManager *tm, ObjManager *om, Portrait *por)
{
 gui = g;
 font = f;
 party = p;
 tile_manager = tm;
 obj_manager = om;
 portrait = por;

 uint16 x_off = Game::get_game()->get_game_x_offset();
 uint16 y_off = Game::get_game()->get_game_y_offset();

 // Check for Korean mode and compact_ui
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean = font_manager && font_manager->is_korean_enabled() &&
                   font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 int ui_scale = (use_korean ? (compact_ui ? 3 : 4) : 1);

 // Calculate view positions
 uint16 view_x, view_y, party_x, party_y;
 uint16 inv_x, inv_y;  // Separate inventory view position for compact_ui
 if(use_korean)
 {
   // Scaled positions (3x for compact_ui, 4x for normal)
   // Original: view at 176,8 within 320x200, party at 168,6
   // Panel starts at screen_width - 152*scale
   uint16 screen_width = Game::get_game()->get_screen()->get_width();
   uint16 panel_start = screen_width - 152 * ui_scale;
   view_x = panel_start + (176 - 168) * ui_scale; // 8 pixels from panel edge
   view_y = 8 * ui_scale; // 8 pixels from top

   if(compact_ui)
   {
     // In compact_ui, party view width is reduced by half + 8 pixels
     // PartyView should move LEFT by 8 pixels total (6+2)
     int party_width_reduction = (116 * ui_scale) / 2;
     party_x = panel_start + party_width_reduction + 12 * ui_scale + 8 * ui_scale;  // shift left 8 pixels
     party_y = 0;  // keep at top edge (negative y causes clipping issues)

     // In compact_ui (3x scale), use bottom-right alignment instead of top-left
     // 4x mode inventory bottom edge: 8*4 + 104*4 = 448 pixels from top
     // 3x mode: keep same bottom edge, so y = 448 - 104*3 = 136
     // For x: 4x mode panel_start was at screen_width - 152*4
     // 3x mode panel_start is at screen_width - 152*3, which is further right
     // We need to offset by the difference: (152*4 - 152*3) = 152
     // Move up by 12 pixels (1x basis) to better align with party view
     inv_x = panel_start - 152;  // Shift left to match 4x visual position
     inv_y = 448 - 104 * ui_scale - 12 * ui_scale;  // Bottom-align with 4x mode, up 12 pixels
   }
   else
   {
     party_x = panel_start; // party view starts at panel edge
     party_y = 6 * ui_scale; // 6 pixels from top
     inv_x = view_x;
     inv_y = view_y;
   }
 }
 else
 {
   if(Game::get_game()->is_original_plus())
     x_off += Game::get_game()->get_game_width() - 320;
   view_x = 176 + x_off;
   view_y = 8 + y_off;
   party_x = 168 + x_off;
   party_y = 6 + y_off;
   inv_x = view_x;
   inv_y = view_y;
 }

 inventory_view = new InventoryView(config);
 inventory_view->init(gui->get_screen(), this, inv_x, inv_y, font, party, tile_manager, obj_manager);

 portrait_view = new PortraitView(config);
 // In compact_ui Korean mode, use same position as inventory (left-bottom aligned)
 portrait_view->init((use_korean && compact_ui) ? inv_x : view_x,
                     (use_korean && compact_ui) ? inv_y : view_y,
                     font, party, player, tile_manager, obj_manager, portrait);

 if(!Game::get_game()->is_new_style())
 {
	 //inventory_view = new InventoryView(config);
	 //inventory_view->init(gui->get_screen(), this, 176+x_off,8+y_off, font, party, tile_manager, obj_manager);
	 actor_view = new ActorView(config);
	 // In compact_ui Korean mode, use same position as inventory (left-bottom aligned)
	 actor_view->init(gui->get_screen(), this,
	                  (use_korean && compact_ui) ? inv_x : view_x,
	                  (use_korean && compact_ui) ? inv_y : view_y,
	                  font, party, tile_manager, obj_manager, portrait);

	 party_view = new PartyView(config);
	 if(game_type==NUVIE_GAME_U6)
	 {
	   party_view->init(this, party_x, party_y, font, party, player, tile_manager, obj_manager);
	   spell_view = new SpellView(config);
	 }
	 else
	 {
	   party_view->init(this, view_x, party_y, font, party, player, tile_manager, obj_manager);
	 }
	 if(game_type == NUVIE_GAME_MD)
	 {
		 if(Game::get_game()->is_new_style() == false)
		 {
		   mdSkyWidget = new MDSkyStripWidget(config, Game::get_game()->get_clock(), player);
		   mdSkyWidget->init(32 + x_off, 2 + y_off);
		   gui->AddWidget(mdSkyWidget);
		   if(Game::get_game()->is_original_plus())
		     mdSkyWidget->Hide();
		 }
	 }

	 // In compact_ui Korean mode, party_view is always visible
	 if(use_korean && compact_ui)
	 {
	   party_view_permanent = true;
	 }

 }

 else
 {
	 //inventory_view = new InventoryViewGump(config);
	 //inventory_view->init(gui->get_screen(), this, 176+x_off,8+y_off, font, party, tile_manager, obj_manager);
	 if(game_type==NUVIE_GAME_U6)
	 {
	   spell_view = new SpellViewGump(config);
	   ribbon = new SunMoonRibbon(player, Game::get_game()->get_weather(), tile_manager);
	   ribbon->init(gui->get_screen());
	   gui->AddWidget(ribbon);
	   ribbon->Hide(); //will be shown on first call to update()
	 }
 }

 uint16 spell_x_offset = 168+x_off;
 uint16 spell_y_offset = 6+y_off;
 if(Game::get_game()->is_new_style())
 {
	spell_x_offset = Game::get_game()->get_game_width() - SPELLVIEWGUMP_WIDTH + x_off;
 }
 else if(use_korean)
 {
   if(compact_ui)
   {
     // In compact_ui Korean mode, use same position as inventory (left-bottom aligned)
     spell_x_offset = inv_x;
     spell_y_offset = inv_y;
   }
   else
   {
     // Scaled position for SpellView (4x for normal Korean mode)
     uint16 screen_width = Game::get_game()->get_screen()->get_width();
     uint16 panel_start = screen_width - 152 * ui_scale;
     spell_x_offset = panel_start; // Spell view starts at panel edge
     spell_y_offset = 6 * ui_scale; // 6 pixels from top
   }
 }

 if(spell_view)
 {
   spell_view->init(gui->get_screen(), this, spell_x_offset, spell_y_offset, font, party, tile_manager, obj_manager);
 }

 //set_current_view((View *)party_view);

 return true;
}

void ViewManager::reload()
{
 if(!Game::get_game()->is_new_style())
   actor_view->set_party_member(0);
 inventory_view->lock_to_actor(false);
 inventory_view->set_party_member(0);

 // In compact_ui mode, default to inventory view (party view is drawn separately)
 if(party_view_permanent)
 {
   // Add party_view to GUI first (behind current view)
   party_view->Show();
   gui->AddWidget((GUI_Widget *)party_view);
   set_inventory_mode();
 }
 else
   set_party_mode();
 update();
}

bool ViewManager::set_current_view(View *view)
{
 uint8 cur_party_member;

 //actor_view->set_party_member(cur_party_member);
 if(view == NULL ) // || game_type != NUVIE_GAME_U6) //HACK! remove this when views support MD and SE
   return false;

 if(current_view == view) // nothing to do if view is already the current_view.
   return false;

 if(current_view != NULL)
   {
    gui->removeWidget((GUI_Widget *)current_view);//remove current widget from gui

    cur_party_member = current_view->get_party_member_num();
    view->set_party_member(cur_party_member);
   }

 current_view = view;
 view->Show();
 gui->AddWidget((GUI_Widget *)view);
 view->Redraw();

 gui->Display();

  if(actor_view)
  {
    if(view != actor_view)
    {
      actor_view->set_show_cursor(false);
      actor_view->release_focus();
    } 
  }

  if(inventory_view)
  {
    if(view != inventory_view)
    {
      inventory_view->set_show_cursor(false);
      inventory_view->release_focus();
    } 
  }

 return true;
}

void ViewManager::close_current_view()
{
	if(current_view == NULL)
		return;

	gui->removeWidget((GUI_Widget *)current_view);//remove current widget from gui
	current_view = NULL;
}

void ViewManager::hide_party_view()
{
	if(party_view)
	{
		party_view->Hide();
		gui->removeWidget((GUI_Widget *)party_view);
	}
}

void ViewManager::update()
{
 if(current_view)
   current_view->Redraw();

 if(ribbon && ribbon->Status() == WIDGET_HIDDEN)
 {
	 ribbon->Show();
 }

 if(mdSkyWidget)
 {
   mdSkyWidget->Redraw();
 }

 return;
}

// We only change to portrait mode if the actor has a portrait.
void ViewManager::set_portrait_mode(Actor *actor, const char *name)
{
 if(portrait_view->set_portrait(actor, name) == true)
  {
   set_current_view((View *)portrait_view);
  }
}

void ViewManager::set_inventory_mode()
{
 set_current_view((View *)inventory_view);
 Event *event = Game::get_game()->get_event();
 if(event->get_mode() == EQUIP_MODE || event->get_mode() == INPUT_MODE
    || event->get_mode() == ATTACK_MODE)
	inventory_view->set_show_cursor(true);
}

void ViewManager::set_party_mode()
{
 Event *event = Game::get_game()->get_event();
 if(event->get_mode() == EQUIP_MODE)
	 event->cancelAction();
 else if(event->get_mode() == INPUT_MODE || event->get_mode() == ATTACK_MODE)
	 event->moveCursorToMapWindow();

 if(!Game::get_game()->is_new_style())
 {
   // In compact_ui mode, party_view is always visible separately, so show inventory instead
   if(party_view_permanent)
     set_current_view((View *)inventory_view);
   else
     set_current_view((View *)party_view);
 }
 return;
}

void ViewManager::set_actor_mode()
{
 set_current_view((View *)actor_view);
 Event *event = Game::get_game()->get_event();
 if(event->get_mode() == EQUIP_MODE || event->get_mode() == INPUT_MODE
    || event->get_mode() == ATTACK_MODE)
 {
	actor_view->set_show_cursor(true);
	actor_view->moveCursorToButton(2);
 }

}

void ViewManager::set_spell_mode(Actor *caster, Obj *spell_container, bool eventMode)
{
 if(spell_view != NULL)
 {
   spell_view->set_spell_caster(caster, spell_container, eventMode);
   set_current_view((View *)spell_view);
 }
 return;
}

void ViewManager::close_spell_mode()
{
  if(spell_view)
  {
	  //FIXME this should set previous view. Don't default to inventory view.
	  spell_view->release_focus();
	  if(!Game::get_game()->is_new_style())
		  set_inventory_mode();
	  else
		  close_current_view();
  }
}

void ViewManager::open_doll_view(Actor *actor)
{
	Screen *screen = Game::get_game()->get_screen();

	if(Game::get_game()->is_new_style())
	{
		if(actor == NULL)
		{
			actor = doll_view_get_next_party_member();
		}
		DollViewGump *doll = get_doll_view(actor);
		if(doll == NULL)
		{
			// Check for Korean 3x scaling in new_style mode
			FontManager *font_manager = Game::get_game()->get_font_manager();
			bool korean_enabled = font_manager && font_manager->is_korean_enabled();
			int gump_scale = korean_enabled ? 3 : 1;

			uint16 x_off = Game::get_game()->get_game_x_offset();
			uint16 y_off = Game::get_game()->get_game_y_offset();
			uint8 num_doll_gumps = doll_gumps.size();
			doll = new DollViewGump(config);
			uint16 x = 12 * num_doll_gumps * gump_scale;
			uint16 y = 12 * num_doll_gumps * gump_scale;

			uint16 scaled_height = DOLLVIEWGUMP_HEIGHT * gump_scale;
			if(y + scaled_height > screen->get_height())
				y = screen->get_height() - scaled_height;

			doll->init(Game::get_game()->get_screen(), this, x + x_off, y + y_off, actor, font, party, tile_manager, obj_manager);

			add_view((View *)doll);
			add_gump(doll);
			doll_gumps.push_back(doll);
		}
		else
		{
			move_gump_to_top(doll);
		}
	}
}

Actor *ViewManager::doll_view_get_next_party_member()
{
	if(doll_gumps.empty())
	{
		doll_next_party_member = 0; //reset to first party member when there are no doll gumps on screen.
	}
	Actor *a = party->get_actor(doll_next_party_member);
	doll_next_party_member = (doll_next_party_member + 1) % party->get_party_size();

	return a;
}

DollViewGump *ViewManager::get_doll_view(Actor *actor)
{
	std::list<DraggableView *>::iterator iter;
	for(iter=doll_gumps.begin(); iter != doll_gumps.end();iter++)
	{
		DollViewGump *view = (DollViewGump *)*iter;
		if(view->get_actor() == actor)
		{
			return view;
		}
	}

	return NULL;
}

ContainerViewGump *ViewManager::get_container_view(Actor *actor, Obj *obj)
{
	std::list<DraggableView *>::iterator iter;
	for(iter=container_gumps.begin(); iter != container_gumps.end();iter++)
	{
		ContainerViewGump *view = (ContainerViewGump *)*iter;
		if(actor)
		{
			if(view->is_actor_container() && view->get_actor() == actor)
			{
				return view;
			}
		}
		else if(obj)
		{
			if(!view->is_actor_container() && view->get_container_obj() == obj)
			{
				return view;
			}
		}
	}

	return NULL;
}

void ViewManager::open_container_view(Actor *actor, Obj *obj)
{
	ContainerViewGump *view = get_container_view(actor, obj);

	if(view == NULL)
	{
		uint16 x_off = Game::get_game()->get_game_x_offset();
		uint16 y_off = Game::get_game()->get_game_y_offset();
		uint16 container_x, container_y;
		if(!Game::get_game()->is_new_style()) {
			container_x = x_off;
			container_y = y_off;
		} else {
			container_x = Game::get_game()->get_game_width() - 120 + x_off;
			container_y = 20 + y_off;
		}

		view = new ContainerViewGump(config);
		view->init(Game::get_game()->get_screen(), this, container_x, container_y, font, party, tile_manager, obj_manager, obj);
		if(actor)
			view->set_actor(actor);
		else
			view->set_container_obj(obj);

		container_gumps.push_back(view);
		add_gump(view);
		add_view((View *)view);
	}
	else
	{
		move_gump_to_top(view);
	}
}

void ViewManager::close_container_view(Actor *actor)
{
	ContainerViewGump *view = get_container_view(actor, NULL);

	if(view)
	{
		close_gump(view);
	}
}

void ViewManager::open_mapeditor_view()
{
	if(Game::get_game()->is_new_style() && Game::get_game()->is_roof_mode())
	{
		uint16 x_off = Game::get_game()->get_game_x_offset();
		uint16 y_off = Game::get_game()->get_game_y_offset();
		x_off += Game::get_game()->get_game_width() - 90;
		MapEditorView *view = new MapEditorView(config);
		view->init(Game::get_game()->get_screen(), this, x_off , y_off, font, party, tile_manager, obj_manager);
		add_view((View *)view);
		view->grab_focus();
	}
}

void ViewManager::open_portrait_gump(Actor *a)
{
	if(Game::get_game()->is_new_style())
	{
		uint16 x_off = Game::get_game()->get_game_x_offset();
		uint16 y_off = Game::get_game()->get_game_y_offset();
		PortraitViewGump *view = new PortraitViewGump(config);
		view->init(Game::get_game()->get_screen(), this, 62 + x_off, y_off, font, party, tile_manager, obj_manager, portrait, a);
		add_view((View *)view);
		add_gump(view);
		view->grab_focus();
	}
}

void ViewManager::open_sign_gump(const char *sign_text, uint16 length)
{
	if(Game::get_game()->is_using_text_gumps()) // check should be useless
	{
		SignViewGump *view = new SignViewGump(config);
		view->init(Game::get_game()->get_screen(), this, font, party, tile_manager, obj_manager, sign_text, length);
		add_view((View *)view);
		add_gump(view);
		view->grab_focus();
	}
}

void ViewManager::open_scroll_gump(const char *text, uint16 length)
{
  if(Game::get_game()->is_using_text_gumps()) // check should be useless
  {
    ScrollViewGump *view = new ScrollViewGump(config);
    view->init(Game::get_game()->get_screen(), this, font, party, tile_manager, obj_manager, string(text,length));
    add_view((View *)view);
    add_gump(view);
    view->grab_focus();
  }
}

void ViewManager::add_view(View *view)
{
	view->Show();
	gui->AddWidget((GUI_Widget *)view);
	if(Game::get_game()->is_new_style())
	{
		Game::get_game()->get_scroll()->moveToFront();
	}
	view->Redraw();
	gui->Display();
}

void ViewManager::add_gump(DraggableView *gump)
{
	gumps.push_back(gump);
	Game::get_game()->get_map_window()->set_walking(false);
	if(ribbon)
	{
	  ribbon->extend();
	}
}

void ViewManager::close_gump(DraggableView *gump)
{
	gumps.remove(gump);
	container_gumps.remove(gump);
	doll_gumps.remove(gump);

	gump->close_view();
	gump->Delete();
	//gui->removeWidget((GUI_Widget *)gump);

	if(gumps.empty() && ribbon != NULL)
	{
	  ribbon->retract();
	}
}

void ViewManager::close_all_gumps()
{
	std::list<DraggableView *>::iterator iter;
	for(iter=gumps.begin(); iter != gumps.end();)
	{
		DraggableView *gump = *iter;
		iter++;

		close_gump(gump);
	}
	//TODO make sure all gump objects have been deleted by GUI.
}

void ViewManager::move_gump_to_top(DraggableView *gump)
{
	gump->moveToFront();
	Game::get_game()->get_scroll()->moveToFront();
}

// callbacks for switching views

GUI_status partyViewButtonCallback(void *data)
{
 ViewManager *view_manager;

 view_manager = (ViewManager *)data;

 view_manager->set_party_mode();

 return GUI_YUM;
}

GUI_status actorViewButtonCallback(void *data)
{
 ViewManager *view_manager;

 view_manager = (ViewManager *)data;

 view_manager->set_actor_mode();

 return GUI_YUM;
}

GUI_status inventoryViewButtonCallback(void *data)
{
 ViewManager *view_manager;

 view_manager = (ViewManager *)data;

 view_manager->set_inventory_mode();

 return GUI_YUM;
}

void ViewManager::double_click_obj(Obj *obj)
{
    Event *event = Game::get_game()->get_event();
    if(Game::get_game()->get_usecode()->is_readable(obj)) // look at a scroll or book
    {
        event->set_mode(LOOK_MODE);
        event->look(obj);
        event->endAction(false); // FIXME: should be in look()
    }
    else if(event->newAction(USE_MODE))
        event->select_obj(obj);
}

unsigned int ViewManager::get_display_weight(float weight)
{
	if(weight > 1)
		return static_cast<unsigned int>(roundf(weight));
	else if(weight > 0)
		return 1;
	else // weight == 0 (or somehow negative)
		return 0;
}

// beginning of custom doll functions shared between DollWidget and DollViewGump
std::string ViewManager::getDollDataDirString()
{
  if(!DollDataDirString.empty())
    return DollDataDirString;
  DollDataDirString = GUI::get_gui()->get_data_dir();
  std::string path;
  build_path(DollDataDirString, "images", path);
  DollDataDirString = path;
  build_path(DollDataDirString, "gumps", path);
  DollDataDirString = path;
  build_path(DollDataDirString, "doll", path);
  DollDataDirString = path;

  return DollDataDirString;
}

SDL_Surface *ViewManager::loadAvatarDollImage(SDL_Surface *avatar_doll, bool orig) {
	char filename[17]; //avatar_nn_nn.bmp\0
	std::string imagefile;
	uint8 portrait_num = Game::get_game()->get_portrait()->get_avatar_portrait_num();

	sprintf(filename, "avatar_%s_%02d.bmp", get_game_tag(Game::get_game()->get_game_type()), portrait_num);
	if(orig) {
		build_path(getDollDataDirString(), "orig_style", imagefile);
		build_path(imagefile, filename, imagefile);
	} else {
		build_path(getDollDataDirString(), filename, imagefile);
	}
	if(avatar_doll != NULL)
		SDL_FreeSurface(avatar_doll);
	NuvieBmpFile bmp;
	avatar_doll = bmp.getSdlSurface32(imagefile);
	if(avatar_doll == NULL)
		avatar_doll = loadGenericDollImage(orig);
	return avatar_doll;
}

SDL_Surface *ViewManager::loadCustomActorDollImage(SDL_Surface *actor_doll, uint8 actor_num, bool orig) {
  char filename[17]; //actor_nn_nnn.bmp\0
  std::string imagefile;

  if(actor_doll != NULL)
    SDL_FreeSurface(actor_doll);

  sprintf(filename, "actor_%s_%03d.bmp", get_game_tag(Game::get_game()->get_game_type()), actor_num);
  if(orig) {
    build_path(getDollDataDirString(), "orig_style", imagefile);
    build_path(imagefile, filename, imagefile);
  } else {
    build_path(getDollDataDirString(), filename, imagefile);
  }
  NuvieBmpFile bmp;
  actor_doll = bmp.getSdlSurface32(imagefile);

  if(actor_doll == NULL)
    actor_doll = loadGenericDollImage(orig);
  return actor_doll;
}

SDL_Surface *ViewManager::loadGenericDollImage(bool orig) {
  char filename[14]; //avatar_nn.bmp\0
  std::string imagefile;

  sprintf(filename, "actor_%s.bmp", get_game_tag(Game::get_game()->get_game_type()));
  if(orig) {
    build_path(getDollDataDirString(), "orig_style", imagefile);
    build_path(imagefile, filename, imagefile);
  } else {
    build_path(getDollDataDirString(), filename, imagefile);
  }
  NuvieBmpFile bmp;
  return bmp.getSdlSurface32(imagefile);
}
