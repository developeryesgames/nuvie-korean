/*
 *  Background.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Sun Aug 24 2003.
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

#include <ctype.h>
#include <string>

#include "nuvieDefs.h"
#include "Configuration.h"
#include "U6misc.h"
#include "U6Lib_n.h"
#include "U6Bmp.h"
#include "Dither.h"
#include "Background.h"
#include "MapWindow.h"
#include "GUI.h"
#include "FontManager.h"
#include "Console.h"

Background::Background(Configuration *cfg) : GUI_Widget(NULL)
{
 config = cfg;
 config->value("config/GameType",game_type);

 bg_w = 0; bg_h = 0; border_width = 0;
 background = NULL;
 x_off = Game::get_game()->get_game_x_offset();
 y_off = Game::get_game()->get_game_y_offset();


 Init(NULL, 0,0,Game::get_game()->get_screen()->get_width(),Game::get_game()->get_screen()->get_height());
}

Background::~Background()
{
 if(background)
  delete background;
}

bool Background::init()
{
 std::string filename;

 if(!Game::get_game()->is_new_style())
 {
	 switch(game_type)
	   {
		case NUVIE_GAME_U6 : config_get_path(config,"paper.bmp",filename);
					 background = (U6Shape *) new U6Bmp();
					 if(background->load(filename) == false)
					   return false;
					 break;


		case NUVIE_GAME_MD :
							 background = new U6Shape();
							 background->load_WoU_background(config, game_type);
							 if(Game::get_game()->is_original_plus()) {
								 border_width = 144;
								 left_bg_x_off = x_off + Game::get_game()->get_game_width() - border_width;
							 }
							 break;

		case NUVIE_GAME_SE :
							 background = new U6Shape();
							 background->load_WoU_background(config, game_type);
							 if(Game::get_game()->is_original_plus()) {
								 border_width = 142;
								 left_bg_x_off = x_off + Game::get_game()->get_game_width() - border_width;
							 }
							 break;
	   }

	 background->get_size(&bg_w,&bg_h);
	 DEBUG(0, LEVEL_INFORMATIONAL, "Background: paper size %ux%u\n", bg_w, bg_h);

	 if(game_type == NUVIE_GAME_U6 && Game::get_game()->is_original_plus()) {
		 uint16 bg_scale = bg_w > 0 ? (uint16)(bg_w / 320) : 1;
		 if(bg_scale == 0) bg_scale = 1;
		 uint16 ui_scale = Game::get_game()->get_game_width() / 320;
		 if(ui_scale == 0) ui_scale = 1;
		 uint16 draw_scale = (bg_scale == 1 && ui_scale > 1) ? ui_scale : 1;
		 uint16 panel_width = (uint16)(158 * bg_scale * draw_scale);
		 uint16 main_width = (uint16)(152 * bg_scale * draw_scale);
		 border_width = panel_width;
		 right_bg_x_off = x_off + Game::get_game()->get_game_width() - main_width;
		 left_bg_x_off = x_off + Game::get_game()->get_game_width() - border_width;
	 }

	 Game::get_game()->get_dither()->dither_bitmap(background->get_data(), bg_w, bg_h, DITHER_NO_TRANSPARENCY);
 }
 return true;
}

void Background::Display(bool full_redraw)
{
 if(full_redraw || update_display || Game::get_game()->is_original_plus_full_map())
   {
    uint16 ui_scale = Game::get_game()->get_game_width() / 320;
    if(ui_scale == 0) ui_scale = 1;
    bool use_ui_scale = (ui_scale > 1);

    if(Game::get_game()->is_original_plus()) {
        if(Game::get_game()->is_original_plus_cutoff_map())
            screen->clear(area.x,area.y,area.w,area.h,NULL);
        else if(full_redraw || update_display) { // need to clear null background when we have a game size smaller than the screen
            uint16 game_width = Game::get_game()->get_game_width();
            uint16 game_height = Game::get_game()->get_game_height();
            // In Korean 4x mode, background covers the entire right side (632px),
            // so we should not clear the right side area
            if(x_off > 0) { // centered
                screen->clear(area.x, area.y, x_off, area.h, NULL); // left side
                if(!use_ui_scale) {
                    screen->clear(x_off + game_width, area.y, x_off, area.h, NULL); // right side (skip in Korean 4x mode)
                }
            } else if(area.w > game_width && !use_ui_scale) { // upper_left position (skip in Korean 4x mode)
                screen->clear(game_width, area.y, area.w - game_width, area.h, NULL); // right side
            }
            if(y_off > 0) {  // centered
                screen->clear(area.x, area.y, area.w, y_off, NULL); // top
                screen->clear(area.x, y_off + game_height, area.w, y_off, NULL); // bottom
            } else if(area.h > game_height) { // upper_left position
                screen->clear(area.x, game_height, area.w, area.h - game_height, NULL); // bottom
            }
        }
        unsigned char *ptr = background->get_data();
        if(game_type == NUVIE_GAME_U6) {
            uint16 scale = bg_w > 0 ? (uint16)(bg_w / 320) : 1;
            if(scale == 0) scale = 1;
            uint16 main_width = (uint16)(152 * scale);
            uint16 panel_width = (uint16)(158 * scale);
            uint16 draw_scale = (scale == 1 && use_ui_scale) ? ui_scale : 1;
            uint16 panel_x = x_off + Game::get_game()->get_game_width() - (uint16)(panel_width * draw_scale);

            static int bg_debug_count = 0;
            if(bg_debug_count < 3) {
                ConsoleAddInfo("Background U6: bg=%ux%u game=%ux%u screen=%ux%u x_off=%u y_off=%u scale=%u draw_scale=%u panel_width=%u panel_x=%u",
                      bg_w, bg_h, Game::get_game()->get_game_width(), Game::get_game()->get_game_height(),
                      screen->get_width(), screen->get_height(), x_off, y_off, scale, draw_scale, panel_width, panel_x);
                bg_debug_count++;
            }


            if(scale > 1) {
                // Background is already scaled; blit the full panel at native size.
                unsigned char *panel_ptr = background->get_data() + (bg_w - panel_width);
                screen->blit(panel_x, y_off, panel_ptr, 8, panel_width, bg_h, bg_w, true);
            } else if(use_ui_scale) {
                // Scale the full paper background so the map can use a proper frame.
                if(ui_scale == 4)
                    screen->blit4x(x_off, y_off, background->get_data(), 8, bg_w, bg_h, bg_w, false);
                else if(ui_scale == 2)
                    screen->blit2x(x_off, y_off, background->get_data(), 8, bg_w, bg_h, bg_w, false);
                else
                    screen->blit(x_off, y_off, background->get_data(), 8, bg_w, bg_h, bg_w, true);
            } else {
                ptr += (bg_w - main_width);
                screen->blit(right_bg_x_off, y_off, ptr, 8, main_width, bg_h, bg_w, true);
                screen->blit(left_bg_x_off, y_off, background->get_data(), 8, (uint16)(6 * scale), bg_h, bg_w, true);
            }
        } else {
            if(game_type == NUVIE_GAME_MD)
                screen->fill(0, left_bg_x_off, y_off, border_width, bg_h); // background has transparent parts that should be black
            ptr += (bg_w - border_width);
            screen->blit(left_bg_x_off, y_off, ptr, 8, border_width, bg_h, bg_w, true);
        }
    } else {
        screen->clear(area.x,area.y,area.w,area.h,NULL);
        if(Game::get_game()->is_orig_style())
            screen->blit(x_off, y_off, background->get_data(), 8,  bg_w, bg_h, bg_w, true);
    }
    update_display = false;
    screen->update(0,0,area.w,area.h);
   }

 return;
}

bool Background::drag_accept_drop(int x, int y, int message, void *data)
{
	GUI::get_gui()->force_full_redraw();
	DEBUG(0,LEVEL_DEBUGGING,"Background::drag_accept_drop()\n");
	if(Game::get_game()->is_original_plus_full_map() && message == GUI_DRAG_OBJ) { // added to gui before the map window so we need to redirect
		MapWindow *map_window = Game::get_game()->get_map_window();
		if(!map_window) // should be initialized before drops occur but we will play it safe
			return false;
		if(Game::get_game()->get_game_width() > x - x_off && x >= x_off // make sure we are on the map window
		   && Game::get_game()->get_game_height() > y - y_off && y >= y_off) {
			if(x >= left_bg_x_off && y <= 200 + y_off) // over background image
				return false;
			return map_window->drag_accept_drop(x, y, message, data);
		}
	}
	return false;
}

void Background::drag_perform_drop(int x, int y, int message, void *data)
{
	DEBUG(0,LEVEL_DEBUGGING,"Background::drag_perform_drop()\n");
	if(message == GUI_DRAG_OBJ) // should only happen with original_plus_full_map
		Game::get_game()->get_map_window()->drag_perform_drop(x, y, message, data);
}
