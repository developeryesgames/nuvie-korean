/*
 *  GUI_text.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Thr Aug 14 2003.
 *  Copyright (c) Nuvie Team 2003. All rights reserved.
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

#include "nuvieDefs.h"

#include "GUI_text.h"
#include "GUI_font.h"
#include "Game.h"
#include "FontManager.h"
#include "KoreanFont.h"
#include "Screen.h"
#include <stdlib.h>

GUI_Text:: GUI_Text(int x, int y, Uint8 r, Uint8 g, Uint8 b, GUI_Font *gui_font, uint16 line_length)
 : GUI_Widget(NULL, x, y, 0, 0)
{
 R = r;
 G = g;
 B = b;
 text = NULL;
 max_width = line_length;
 text_scale = 1;

 font = gui_font;
}


GUI_Text:: GUI_Text(int x, int y, Uint8 r, Uint8 g, Uint8 b, const char *str, GUI_Font *gui_font, uint16 line_length)
 : GUI_Widget(NULL, x, y, 0, 0)
{
 int w,h;

 R = r;
 G = g;
 B = b;
 text = NULL;
 max_width = line_length;
 text_scale = 1;

 font = gui_font;

 text = (char *)strdup(str);

 if(text == NULL)
  {
   DEBUG(0,LEVEL_ERROR,"GUI_Text: failed to allocate memory for text\n");
   return;
  }

 font->TextExtent(text, &w, &h, max_width);

 area.w = w;
 area.h = h;
}

GUI_Text::~GUI_Text()
{
 if(text)
  free(text);

 return;
}


/* Show the widget  */
void
GUI_Text:: Display(bool full_redraw)
{
 // Check if Korean mode is enabled and text contains Korean characters
 Game *game = Game::get_game();
 FontManager *fm = game ? game->get_font_manager() : NULL;

 if (fm && fm->is_korean_enabled() && text_scale > 1) {
   // Use Korean font for scaled text in Korean mode
   KoreanFont *korean_font = fm->get_korean_font();
   if (korean_font) {
     Screen *screen = game->get_screen();
     // Use black color (0) for menu text
     uint8 color = 0;
     // Korean font is 16px, use scale 1 for menu (text_scale 3 means menu mode)
     uint8 font_scale = 1;

     korean_font->drawStringUTF8(screen, text, area.x, area.y, color, 0, font_scale);
     DisplayChildren();
     return;
   }
 }

 font->SetTransparency(1);
 font->SetColoring(R,G,B);
 if (text_scale > 1)
   font->TextOutScaled(surface, area.x, area.y, text, text_scale);
 else
   font->TextOut(surface,area.x,area.y,text,max_width);

 DisplayChildren();
}
