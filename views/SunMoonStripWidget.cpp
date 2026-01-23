/*
 *  SunMoonStripWidget.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Fri Nov 29 2013.
 *  Copyright (c) 2013. All rights reserved.
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

#include <math.h>

#include "nuvieDefs.h"
#include "Player.h"
#include "Weather.h"
#include "GameClock.h"
#include "FontManager.h"
#include "GamePalette.h"
#include "Background.h"
#include "U6Shape.h"
#include "SunMoonStripWidget.h"


SunMoonStripWidget::SunMoonStripWidget(Player *p, TileManager *tm): GUI_Widget(NULL, 0, 0, 0, 0)
{
  player = p;
  tile_manager = tm;
}

SunMoonStripWidget::~SunMoonStripWidget()
{

}


void SunMoonStripWidget::init(sint16 x, sint16 y)
{
  // Check for Korean scaling mode (4x normal, 3x compact_ui)
  FontManager *font_manager = Game::get_game()->get_font_manager();
  bool use_korean = font_manager && font_manager->is_korean_enabled() &&
                    font_manager->get_korean_font() && Game::get_game()->is_original_plus();
  bool compact_ui = Game::get_game()->is_compact_ui();
  int scale = use_korean ? (compact_ui ? 3 : 4) : 1;

  if(scale > 1)
  {
    // Scaled: 100x20 -> scale * dimensions
    GUI_Widget::Init(NULL, x, y, 100*scale, 20*scale);
  }
  else
  {
    GUI_Widget::Init(NULL,x,y,100,20);
  }
}

void SunMoonStripWidget::Display(bool full_redraw)
{
 //if(full_redraw || update_display)
 // {
   update_display = false;
   uint8 level = player->get_location_level();

   if(level == 0 || level == 5)
     display_surface_strip();
   else
     display_dungeon_strip();

   screen->update(area.x, area.y, area.w, area.h);
//  }

}


void SunMoonStripWidget::display_surface_strip()
{
 uint8 i;
 Tile *tile;
 GameClock *clock = Game::get_game()->get_clock();
 Weather *weather = Game::get_game()->get_weather();
 bool eclipse = weather->is_eclipse();

 // Check for Korean scaling mode (4x normal, 3x compact_ui)
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean = font_manager && font_manager->is_korean_enabled() &&
                   font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 int scale = use_korean ? (compact_ui ? 3 : 4) : 1;

 // Clear the sky strip area where sun/moon can appear
 uint16 strip_x = area.x + 8 * scale;
 uint16 strip_w = 144 * scale;  // 9 tiles * 16 pixels, scaled

 if(compact_ui) {
   // In compact_ui mode, fill with background color
   // Only clear the strip area itself (no extra above to avoid overlapping UI)
   uint16 strip_y = area.y;
   uint16 strip_h = 16 * scale;  // Just the strip height
   uint8 bg_color = Game::get_game()->get_palette()->get_bg_color();
   screen->fill(bg_color, strip_x, strip_y, strip_w, strip_h);
 } else {
   uint16 strip_y = area.y - 4 * scale;
   uint16 strip_h = (4 + 16) * scale;  // 4 pixels above + 16 pixels strip height, scaled
   // For non-compact modes, redraw with paper texture
   Background *bg = Game::get_game()->get_background();
   if(bg && bg->get_bg_shape()) {
     U6Shape *bg_shape = bg->get_bg_shape();
     uint16 bg_w = bg->get_bg_w();
     unsigned char *bg_data = bg_shape->get_data();

     uint16 game_x_off = Game::get_game()->get_game_x_offset();
     uint16 game_y_off = Game::get_game()->get_game_y_offset();

     // Source position in 1x background texture
     uint16 src_x = (strip_x - game_x_off) / scale;
     uint16 src_y = (strip_y - game_y_off) / scale;

     // Point to the correct row in background data
     unsigned char *src_ptr = bg_data + (src_y * bg_w) + src_x;

     if(scale >= 4)
       screen->blit4x(strip_x, strip_y, src_ptr, 8, 144, 20, bg_w, true);
     else if(scale == 3)
       screen->blit3x(strip_x, strip_y, src_ptr, 8, 144, 20, bg_w, true);
     else
       screen->blit(strip_x, strip_y, src_ptr, 8, 144, 20, bg_w, true);
   }
 }

 display_sun(clock->get_hour(), 0/*minutes*/, eclipse);

 if(!eclipse)
   display_moons(clock->get_day(), clock->get_hour());

 for(i=0;i<9;i++)
   {
    tile = tile_manager->get_tile(352+i);
    if(scale >= 4)
      screen->blit4x(area.x + 8*scale + i*16*scale, area.y, tile->data, 8, 16, 16, 16, true);
    else if(scale == 3)
      screen->blit3x(area.x + 8*scale + i*16*scale, area.y, tile->data, 8, 16, 16, 16, true);
    else
      screen->blit(area.x+8 +i*16,area.y,tile->data,8,16,16,16,true);
   }

 return;
}

void SunMoonStripWidget::display_dungeon_strip()
{
 uint8 i;
 Tile *tile;

 // Check for Korean scaling mode (4x normal, 3x compact_ui)
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean = font_manager && font_manager->is_korean_enabled() &&
                   font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 int scale = use_korean ? (compact_ui ? 3 : 4) : 1;

 tile = tile_manager->get_tile(372);
 if(scale >= 4)
   screen->blit4x(area.x + 8*scale, area.y, tile->data, 8, 16, 16, 16, true);
 else if(scale == 3)
   screen->blit3x(area.x + 8*scale, area.y, tile->data, 8, 16, 16, 16, true);
 else
   screen->blit(area.x+8,area.y,tile->data,8,16,16,16,true);

 tile = tile_manager->get_tile(373);

 for(i=1;i<8;i++)
   {
    if(scale >= 4)
      screen->blit4x(area.x + 8*scale + i*16*scale, area.y, tile->data, 8, 16, 16, 16, true);
    else if(scale == 3)
      screen->blit3x(area.x + 8*scale + i*16*scale, area.y, tile->data, 8, 16, 16, 16, true);
    else
      screen->blit(area.x+8 +i*16,area.y,tile->data,8,16,16,16,true);
   }

 tile = tile_manager->get_tile(374);
 if(scale >= 4)
   screen->blit4x(area.x + 8*scale + 7*16*scale + 8*scale, area.y, tile->data, 8, 16, 16, 16, true);
 else if(scale == 3)
   screen->blit3x(area.x + 8*scale + 7*16*scale + 8*scale, area.y, tile->data, 8, 16, 16, 16, true);
 else
   screen->blit(area.x+8 +7*16+8,area.y,tile->data,8,16,16,16,true);

 return;
}
// <SB-X>
void SunMoonStripWidget::display_sun_moon(Tile *tile, uint8 pos)
{
    // Check for Korean scaling mode (4x normal, 3x compact_ui)
    FontManager *font_manager = Game::get_game()->get_font_manager();
    bool use_korean = font_manager && font_manager->is_korean_enabled() &&
                      font_manager->get_korean_font() && Game::get_game()->is_original_plus();
    bool compact_ui = Game::get_game()->is_compact_ui();
    int scale = use_korean ? (compact_ui ? 3 : 4) : 1;

    struct { sint16 x, y; } skypos[15] = // sky positions relative to area
    {
        { 8 + 7*16 - 0*8, 6 }, // 7*16 is the first position on the right side
        { 8 + 7*16 - 1*8, 3 },
        { 8 + 7*16 - 2*8, 1 },
        { 8 + 7*16 - 3*8, -1 },
        { 8 + 7*16 - 4*8, -2 },
        { 8 + 7*16 - 5*8, -3 },
        { 8 + 7*16 - 6*8, -4 },
        { 8 + 7*16 - 7*8, -4 },
        { 8 + 7*16 - 8*8, -4 },
        { 8 + 7*16 - 9*8, -3 },
        { 8 + 7*16 - 10*8, -2 },
        { 8 + 7*16 - 11*8, -1 },
        { 8 + 7*16 - 12*8, 1 },
        { 8 + 7*16 - 13*8, 3 },
        { 8 + 7*16 - 14*8, 6 }
    };

    int height = 16;
    uint16 x = area.x + skypos[pos].x * scale;
    uint16 y = area.y + skypos[pos].y * scale;
    if (skypos[pos].y == 6) // goes through the bottom if not reduced
      height = 10;

    if(scale >= 4)
      screen->blit4x(x, y, tile->data, 8, 16, height, 16, true);
    else if(scale == 3)
      screen->blit3x(x, y, tile->data, 8, 16, height, 16, true);
    else
      screen->blit(x, y, tile->data, 8, 16, height, 16, true);
}

void SunMoonStripWidget::display_sun(uint8 hour, uint8 minute, bool eclipse)
{
  uint16 sun_tile = 0;
  if(eclipse)
    sun_tile = 363; //eclipsed sun
  else if(hour == 5 || hour == 19)
    sun_tile = 361; //orange sun
  else if(hour > 5 && hour < 19)
    sun_tile = 362; //yellow sun
  else return; //no sun
  display_sun_moon(tile_manager->get_tile(sun_tile), hour - 5);
}

void SunMoonStripWidget::display_moons(uint8 day, uint8 hour, uint8 minute)
{
    uint8 phase = 0;
    // trammel (starts 1 hour ahead of sun)
    phase = uint8(nearbyint((day-1)/TRAMMEL_PHASE)) % 8;
    Tile *tileA = tile_manager->get_tile((phase == 0) ? 584 : 584 + (8-phase)); // reverse order in tilelist
    uint8 posA = ((hour + 1) + 3*phase) % 24; // advance 3 positions each phase-change

    // felucca (starts 1 hour behind sun)
    // ...my FELUCCA_PHASE may be wrong but this method works with it...
    sint8 phaseb = (day-1) % uint8(nearbyint(FELUCCA_PHASE*8)) - 1;
    phase = (phaseb >= 0) ? phaseb : 0;
    Tile *tileB = tile_manager->get_tile((phase == 0) ? 584 : 584 + (8-phase)); // reverse order in tilelist
    uint8 posB = ((hour - 1) + 3*phase) % 24; // advance 3 positions per phase-change

    if(posA >= 5 && posA <= 19)
        display_sun_moon(tileA, posA - 5);
    if(posB >= 5 && posB <= 19)
        display_sun_moon(tileB, posB - 5);
}

