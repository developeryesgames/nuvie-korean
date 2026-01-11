/*
 *  GUI_TextInput.cpp
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

#include "nuvieDefs.h"

#include "GUI_TextInput.h"
#include "GUI_font.h"
#include "Keys.h"
#include "Game.h"
#include "FontManager.h"
#include "KoreanFont.h"
#include "Screen.h"
#include <stdlib.h>
#include <cstring>

GUI_TextInput:: GUI_TextInput(int x, int y, Uint8 r, Uint8 g, Uint8 b, char *str,
                              GUI_Font *gui_font, uint16 width, uint16 height, GUI_CallBack *callback)
 : GUI_Text(x, y, r, g, b, gui_font, width)
{
 max_height = height;
 callback_object = callback;
 cursor_color = 0;
 selected_bgcolor = 0;

 // Check for Korean mode
 FontManager *fm = Game::get_game()->get_font_manager();
 use_korean = fm && fm->is_korean_enabled() && Game::get_game()->is_original_plus();

 text = (char *)malloc(max_width * max_height + 1);

 if(text == NULL)
  {
   DEBUG(0,LEVEL_ERROR,"GUI_TextInput failed to allocate memory for text\n");
   return;
  }

 strncpy(text, str, max_width * max_height);
 text[max_width * max_height] = '\0';

 // Initialize UTF-8 buffer for Korean
 if(use_korean) {
   utf8_text = str;
   korean_cursor_x = 0;
 }

 pos = strlen(text);
 length = pos;

 int scale = text_scale > 1 ? text_scale : 1;
 area.w = max_width * font->CharWidth() * scale;
 area.h = max_height * font->CharHeight() * scale;
}

GUI_TextInput::~GUI_TextInput()
{
 return;
}

void GUI_TextInput::release_focus()
{
 GUI_Widget::release_focus();
 composing_text.clear();
 SDL_StopTextInput();
}

void GUI_TextInput::grab_focus()
{
 GUI_Widget::grab_focus();
 composing_text.clear();
 SDL_StartTextInput();
}

GUI_status GUI_TextInput::MouseUp(int x, int y, int button)
{
 //release focus if we click outside the text box.
 if(focused && !HitRect(x, y))
   release_focus();
 else
  {
   if(!focused)
     {
      grab_focus();
     }
  }

 return(GUI_PASS);
}

GUI_status GUI_TextInput::KeyDown(SDL_Keysym key)
{
 char ascii = get_ascii_char_from_keysym(key);
 bool is_printable = isprint(ascii);

 if(!focused)
   return GUI_PASS;


 if(!is_printable && key.sym != SDLK_BACKSPACE)
 {
    KeyBinder *keybinder = Game::get_game()->get_keybinder();
    ActionType a = keybinder->get_ActionType(key);
    switch(keybinder->GetActionKeyType(a))
    {
      case NORTH_KEY: key.sym = SDLK_UP; break;
      case SOUTH_KEY: key.sym = SDLK_DOWN; break;
      case WEST_KEY: key.sym = SDLK_LEFT; break;
      case EAST_KEY: key.sym = SDLK_RIGHT; break;
      case TOGGLE_CURSOR_KEY: release_focus(); return GUI_PASS; // can tab through to SaveDialog
      case DO_ACTION_KEY: key.sym = SDLK_RETURN; break;
      case CANCEL_ACTION_KEY: key.sym = SDLK_ESCAPE; break;
      case HOME_KEY: key.sym = SDLK_HOME;
      case END_KEY: key.sym = SDLK_END;
      default : if(keybinder->handle_always_available_keys(a)) return GUI_YUM; break;
    }
 }

 switch(key.sym)
   {
    case SDLK_LSHIFT   :
    case SDLK_RSHIFT   :
    case SDLK_LCTRL    :
    case SDLK_RCTRL    :
    case SDLK_CAPSLOCK : break;

    case SDLK_KP_ENTER:
    case SDLK_RETURN :
                       // Finalize any composing text before submitting
                       if(!composing_text.empty())
                       {
                         add_utf8_char(composing_text.c_str());
                         composing_text.clear();
                       }
                       if(callback_object)
                         callback_object->callback(TEXTINPUT_CB_TEXT_READY, this, use_korean ? (void*)utf8_text.c_str() : (void*)text);
                       release_focus();
                       break;

    case SDLK_ESCAPE :
                       // Clear composing text on escape
                       if(!composing_text.empty())
                       {
                         composing_text.clear();
                       }
                       release_focus();
                       break;

    case SDLK_HOME : pos = 0; break;
    case SDLK_END  : pos = length; break;

    case SDLK_KP_4  :
    case SDLK_LEFT : if(pos > 0)
                       pos--;
                     break;

    case SDLK_KP_6   :
    case SDLK_RIGHT : if(pos < length)
                       pos++;
                      break;

    case SDLK_DELETE    : if(pos < length) //delete the character to the right of the cursor
                            {
                             pos++;
                             remove_char(); break;
                            }
                          break;

    case SDLK_BACKSPACE :
                          // If composing, clear composing text first
                          if(!composing_text.empty())
                          {
                            composing_text.clear();
                          }
                          else if(use_korean)
                          {
                            // Remove last UTF-8 character from utf8_text
                            if(!utf8_text.empty())
                            {
                              // Find start of last UTF-8 character
                              size_t i = utf8_text.length() - 1;
                              while(i > 0 && (utf8_text[i] & 0xC0) == 0x80)
                                i--;
                              utf8_text.erase(i);
                            }
                          }
                          else
                          {
                            remove_char(); //delete the character to the left of the cursor
                          }
                          break;

    case SDLK_UP :
    case SDLK_KP_8 :
                    // Disable up/down cycling in Korean mode
                    if(use_korean) break;
                    if(pos == length)
                    {
                        if(length+1 > max_width * max_height)
                            break;
                        length++;
                        if(pos == 0 || text[pos-1] == ' ')
                            text[pos] = 'A';
                        else
                            text[pos] = 'a';
                        break;
                    }
                    text[pos]++;
                    // We want alphanumeric characters or space
                    if(text[pos] < ' ' || text[pos] > 'z')
                    {
                        text[pos] = ' ';
                        break;
                    }
                    while(!isalnum(text[pos]))
                        text[pos]++;
                    break;

    case SDLK_KP_2 :
    case SDLK_DOWN :
                     // Disable up/down cycling in Korean mode
                     if(use_korean) break;
                     if(pos == length)
                     {
                         if(length+1 > max_width * max_height)
                             break;
                         length++;
                         if(pos == 0 || text[pos-1] == ' ')
                             text[pos] = 'Z';
                         else
                             text[pos] = 'z';
                         break;
                     }
                     text[pos]--;
                     // We want alphanumeric characters or space
                     if(text[pos] < ' ' || text[pos] > 'z')
                     {
                         text[pos] = 'z';
                         break;
                     }
                     else if(text[pos] < '0')
                     {
                         text[pos] = ' ';
                         break;
                     }
                     while(!isalnum(text[pos]))
                         text[pos]--;
                     break;

    default :
              // In Korean mode, let TextInput() handle printable characters
              if(use_korean && is_printable)
                return GUI_YUM;
              if(is_printable)
                  add_char(ascii);
              break;
   }



 return(GUI_YUM);
}

void GUI_TextInput::add_char(char c)
{
 uint16 i;

 if(length+1 > max_width * max_height)
   return;

 if(pos < length) //shuffle chars to the right if required.
  {
   for(i=length; i > pos; i--)
     text[i] = text[i-1];
  }

 length++;

 text[pos] = c;
 pos++;

 text[length] = '\0';

 return;
}

void GUI_TextInput::remove_char()
{
 uint16 i;

 if(pos == 0)
  return;

 for(i=pos-1;i<length;i++)
  text[i] = text[i+1];

 pos--;
 length--;

 return;
}

void GUI_TextInput::set_text(const char *new_text)
{
	if(new_text)
	{
		strncpy(text, new_text, max_width * max_height);

		pos = strlen(text);
		length = pos;
	}
}

/* Map the color to the display */
void GUI_TextInput::SetDisplay(Screen *s)
{
	GUI_Widget::SetDisplay(s);
	cursor_color = SDL_MapRGB(surface->format, 0xff, 0, 0);
    selected_bgcolor = SDL_MapRGB(surface->format, 0x5a, 0x6e, 0x91);
}

void GUI_TextInput::UpdateAreaSize()
{
 int scale = text_scale > 1 ? text_scale : 1;
 area.w = max_width * font->CharWidth() * scale;
 area.h = max_height * font->CharHeight() * scale;
}


/* Show the widget  */
void GUI_TextInput:: Display(bool full_redraw)
{
 SDL_Rect r;

 if(full_redraw && focused)
   {
    r = area;
    SDL_FillRect(surface, &r, selected_bgcolor);
   }

 // In Korean mode, draw UTF-8 text using KoreanFont
 if(use_korean)
 {
   FontManager *fm = Game::get_game()->get_font_manager();
   KoreanFont *korean_font = fm ? fm->get_korean_font() : NULL;
   Screen *scr = Game::get_game()->get_screen();

   if(korean_font && scr)
   {
     // Use scale 1 for KoreanFont - it has its own internal scaling
     uint8 font_scale = 1;
     uint16 draw_x = area.x;
     uint16 draw_y = area.y;

     // Use white color (0x0F) for save slot text
     uint8 font_color = 0x0F;

     // Draw the UTF-8 text (includes both ASCII and Korean)
     // Use drawStringUTF8 return value for accurate width
     uint16 text_width = 0;
     if(!utf8_text.empty())
     {
       text_width = korean_font->drawStringUTF8(scr, utf8_text.c_str(), draw_x, draw_y, font_color, 0, font_scale);
       draw_x += text_width;
     }

     // Draw composing text with underline
     uint16 composing_width = 0;
     if(!composing_text.empty() && focused)
     {
       composing_width = korean_font->drawStringUTF8(scr, composing_text.c_str(),
                                  draw_x, draw_y, font_color, 0, font_scale);

       // Draw underline under composing text
       SDL_Rect underline;
       underline.x = draw_x;
       underline.y = draw_y + korean_font->getCharHeight() - 1;
       underline.w = composing_width;
       underline.h = 1;
       SDL_FillRect(surface, &underline, cursor_color);
     }

     // Cache cursor x position for display_cursor()
     korean_cursor_x = area.x + text_width + composing_width;

     if(focused)
       display_cursor();

     return;
   }
 }

 GUI_Text::Display(full_redraw);

 if(focused)
  display_cursor();

}

void GUI_TextInput::display_cursor()
{
 SDL_Rect r;
 uint16 x, y;
 uint16 cw, ch;

 x = pos % max_width;
 y = pos / max_width;

 int scale = text_scale > 1 ? text_scale : 1;
 cw = font->CharWidth() * scale;
 ch = font->CharHeight() * scale;

 r.x = area.x + x * cw;
 r.y = area.y + y * ch;
 r.w = scale;
 r.h = ch;

 // In Korean mode, use cached cursor position from Display()
 if(use_korean)
 {
   FontManager *fm = Game::get_game()->get_font_manager();
   KoreanFont *korean_font = fm ? fm->get_korean_font() : NULL;
   if(korean_font)
   {
     r.x = korean_cursor_x;
     r.y = area.y;  // Always at top row for single-line input
     r.w = 2;  // Slightly wider cursor for visibility
     r.h = korean_font->getCharHeight();
   }
 }

 SDL_FillRect(surface, &r, cursor_color);

 return;
}

GUI_status GUI_TextInput::TextInput(const char *input_text)
{
 if(!focused || input_text == NULL || input_text[0] == '\0')
   return GUI_PASS;

 // Clear composing text since we got a finalized character
 composing_text.clear();

 // Add the UTF-8 text
 add_utf8_char(input_text);

 return GUI_YUM;
}

GUI_status GUI_TextInput::TextEditing(const char *input_text, int start, int len)
{
 if(!focused)
   return GUI_PASS;

 // Store the composing text for display
 // This is the text being composed by the IME (e.g., Korean "ㄱ" before becoming "그")
 std::string new_composing = (input_text != NULL) ? input_text : "";

 if(composing_text != new_composing)
 {
   composing_text = new_composing;
 }

 return GUI_YUM;
}

void GUI_TextInput::add_utf8_char(const char *utf8_char)
{
 if(utf8_char == NULL || utf8_char[0] == '\0')
   return;

 // Add to UTF-8 buffer (used in Korean mode)
 utf8_text += utf8_char;
}

std::string GUI_TextInput::get_utf8_text()
{
 return utf8_text;
}
