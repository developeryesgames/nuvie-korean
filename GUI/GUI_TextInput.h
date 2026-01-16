#ifndef __GUI_TextInput_h__
#define __GUI_TextInput_h__

/*
 *  GUI_TextInput.h
 *  Nuvie
 *
 *  Created by Eric Fry on Sat Jun 26 2004.
 *  Copyright (c) Nuvie Team 2004. All rights reserved.
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

#include "GUI_text.h"
#include <string>

class GUI_Font;
class KoreanFont;


#define TEXTINPUT_CB_TEXT_READY 0x1

class GUI_TextInput : public GUI_Text
{
 protected:
 uint16 max_height;
 uint16 pos;
 uint16 length;

 GUI_CallBack *callback_object;

 Uint32 cursor_color;
 Uint32 selected_bgcolor;

 // Korean input support
 std::string composing_text;  // IME composition text (Korean input preview)
 std::string utf8_text;       // UTF-8 text buffer for Korean
 bool use_korean;
 uint16 korean_cursor_x;      // Cached cursor x position for Korean mode
 bool read_only;              // If true, text cannot be edited

 public:

 GUI_TextInput(int x, int y, Uint8 r, Uint8 g, Uint8 b,
               char *str, GUI_Font *gui_font, uint16 width, uint16 height, GUI_CallBack *callback);
 ~GUI_TextInput();

 void release_focus();
 void grab_focus();

 GUI_status MouseUp(int x, int y, int button);
 GUI_status KeyDown(SDL_Keysym key);
 GUI_status TextInput(const char *text);
 GUI_status TextEditing(const char *text, int start, int length);

 void add_char(char c);
 void add_utf8_char(const char *utf8_char);
 void remove_char();
 void set_text(const char *new_text);
 void set_read_only(bool readonly) { read_only = readonly; }
 bool is_read_only() { return read_only; }
 char *get_text() { return text; }
 std::string get_utf8_text();
void SetDisplay(Screen *s);
void display_cursor();
void UpdateAreaSize(); // Update area size after SetTextScale

	/* Show the widget  */
	virtual void Display(bool full_redraw);

};

#endif /* __GUI_TextInput_h__ */

