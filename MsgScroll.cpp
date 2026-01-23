/*
 *  MsgScroll.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Thu Mar 13 2003.
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
#include <stdarg.h>
#include <string>
#include <ctype.h>
#include <iostream>

#include "nuvieDefs.h"
#include "Configuration.h"
#include "U6misc.h"
#include "FontManager.h"
#include "Font.h"
#include "U6Font.h"
#include "KoreanFont.h"
#include "KoreanTranslation.h"
#include "GamePalette.h"
#include "GUI.h"
#include "MsgScroll.h"
#include "Event.h"
#include "Game.h"
#include "Effect.h"
#include "Keys.h"

// MsgText Class
MsgText::MsgText()
{
 font = NULL;
 color = 0;
}

MsgText::MsgText(std::string new_string, Font *f)
{
 s.assign(new_string);
 font = f;
 color = 0;
 if(font)
 {
   color = font->getDefaultColor();
 }
}

MsgText::~MsgText()
{
}

void MsgText::append(std::string new_string)
{
 s.append(new_string);
}

void MsgText::copy(MsgText *msg_text)
{
 s.assign(msg_text->s);
 font = msg_text->font;
 color = msg_text->color;
}

uint32 MsgText::length()
{
 return (uint32)s.length();
}

uint16 MsgText::getDisplayWidth()
{
  return font->getStringWidth(s.c_str());
}

MsgLine::~MsgLine()
{
 std::list<MsgText *>::iterator iter;

 for(iter=text.begin(); iter != text.end(); iter++)
  {
   delete *iter;
  }
}

void MsgLine::append(MsgText *new_text)
{
 MsgText *msg_text = NULL;

 if(text.size() > 0)
    msg_text = text.back();

 if(msg_text && msg_text->font == new_text->font && msg_text->color == new_text->color && new_text->s.length() == 1 && new_text->s[0] != ' ')
   msg_text->s.append(new_text->s);
 else
  {
   msg_text = new MsgText();
   msg_text->copy(new_text);
   text.push_back(msg_text);
  }

 total_length += new_text->s.length();

 return;
}

// remove the last char in the line.
void MsgLine::remove_char()
{
 MsgText *msg_text;

 if(total_length == 0)
   return;

 msg_text = text.back();

 // Remove last UTF-8 character (handles multi-byte chars like Korean)
 size_t len = msg_text->s.length();
 if(len > 0)
 {
   size_t char_start = len - 1;

   // Find the start of the last UTF-8 character
   // UTF-8 continuation bytes start with 10xxxxxx (0x80-0xBF)
   while(char_start > 0 && (msg_text->s[char_start] & 0xC0) == 0x80)
   {
     char_start--;
   }

   // Calculate how many bytes this character takes
   size_t char_bytes = len - char_start;

   // Erase the UTF-8 character
   msg_text->s.erase(char_start);

   // Decrease total_length by the number of bytes removed
   if(total_length >= char_bytes)
     total_length -= char_bytes;
   else
     total_length = 0;
 }

 if(msg_text->s.length() == 0)
   {
    text.pop_back();
    delete msg_text;
   }

 return;
}

uint32 MsgLine::length()
{
 return total_length;
}

// gets the MsgText object that contains the text at line position pos
MsgText *MsgLine::get_text_at_pos(uint16 pos)
{
 uint16 i;
 std::list<MsgText *>::iterator iter;

 if(pos > total_length)
   return NULL;

 for(i=0,iter=text.begin(); iter != text.end(); iter++)
  {
   if(i + (*iter)->s.length() >= pos)
     return (*iter);

   i += (*iter)->s.length();
  }

 return NULL;
}

uint16 MsgLine::get_display_width()
{
  uint16 total_width = 0;
  std::list<MsgText *>::iterator iter;

  // Check if Korean font is enabled
  FontManager *font_manager = Game::get_game()->get_font_manager();
  KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
  KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
  bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
  bool compact_ui = Game::get_game()->is_compact_ui();
  KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;

  for(iter=text.begin();iter != text.end() ; iter++)
  {
    MsgText *token = *iter;

    if(use_korean)
    {
      // Korean font: 24x24 for compact_ui, 32x32 for normal (scale 1)
      total_width += korean_font->getStringWidthUTF8(token->s.c_str(), 1);
    }
    else
    {
      total_width += token->font->getStringWidth(token->s.c_str());
    }
  }
  return total_width;
}

// MsgScroll Class

void MsgScroll::init(Configuration *cfg, Font *f)
{
	font = f;

	config = cfg;
	config->value("config/GameType",game_type);

	input_char = 0;
	yes_no_only = false;
	aye_nay_only = false;
	numbers_only = false;
	scroll_updated = false;
	discard_whitespace = true;
	page_break = false;
	show_cursor = true;
	talking = false;
	autobreak = false;
	just_finished_page_break = false;
	using_target_cursor = false;

	callback_target = NULL;
	callback_user_data = NULL;

	scrollback_height = MSGSCROLL_SCROLLBACK_HEIGHT;
  capitalise_next_letter = false;

  font_color = FONT_COLOR_U6_NORMAL;
  font_highlight_color = FONT_COLOR_U6_HIGHLIGHT;
  if(game_type!=NUVIE_GAME_U6)
  {
     font_color = FONT_COLOR_WOU_NORMAL;
     font_highlight_color = FONT_COLOR_WOU_HIGHLIGHT;
  }
}

MsgScroll::MsgScroll(Configuration *cfg, Font *f) : GUI_Widget(NULL, 0, 0, 0, 0)
{
 uint16 x, y;

 init(cfg, f);

 switch(game_type)
   {
    case NUVIE_GAME_MD : scroll_width = MSGSCROLL_MD_WIDTH;
                         scroll_height = MSGSCROLL_MD_HEIGHT;
                         x = 184;
                         y = 128;
                         break;

    case NUVIE_GAME_SE : scroll_width = MSGSCROLL_SE_WIDTH;
                         scroll_height = MSGSCROLL_SE_HEIGHT;
                         x = 184;
                         y = 128;
                         break;
    case NUVIE_GAME_U6 :
	default :
						 scroll_width = MSGSCROLL_U6_WIDTH;
                         scroll_height = MSGSCROLL_U6_HEIGHT;
                         x = 176;
                         y = 112;
                         break;
   }

 // Check if Korean mode - need scaled UI positioning
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean = font_manager && font_manager->is_korean_enabled() && font_manager->get_korean_font();
 bool compact_ui = Game::get_game()->is_compact_ui();
 // Background scale is based on game_width/320, not compact_ui setting
 // compact_ui only affects font size, not UI positioning
 uint16 bg_scale = Game::get_game()->get_game_width() / 320;
 if(bg_scale == 0) bg_scale = 1;
 int ui_scale = (use_korean && Game::get_game()->is_original_plus()) ? bg_scale : 1;

 if(use_korean && Game::get_game()->is_original_plus())
 {
   // In Korean mode, the UI panel (152px) is scaled by bg_scale (same as Background)
   // panel_start = screen_width - 152*scale
   // MsgScroll x = panel_start + 8*scale (8 pixels margin)
   uint16 screen_width = Game::get_game()->get_screen()->get_width();
   uint16 screen_height = Game::get_game()->get_screen()->get_height();
   uint16 panel_start = screen_width - 152 * ui_scale;
   x = panel_start + 8 * ui_scale; // 8 pixels margin, scaled

   // y position: Original MsgScroll y=112, scale it by bg_scale to match Background
   y = 112 * ui_scale;  // 112 * 4 = 448 for 4x bg_scale
   DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll: screen=%dx%d, bg_scale=%d, ui_scale=%d, x=%d, y=%d\n",
         screen_width, screen_height, bg_scale, ui_scale, x, y);
 }
 else if(Game::get_game()->is_original_plus())
 {
   x += Game::get_game()->get_game_width() - 320;
 }

 uint16 x_off = Game::get_game()->get_game_x_offset();
 uint16 y_off = Game::get_game()->get_game_y_offset();

 // For Korean mode with original_plus, use scaled dimensions for 16px font
 uint16 area_width, area_height;
 if(use_korean && Game::get_game()->is_original_plus())
 {
   // Korean font: 16px native, UI scaled
   // Original area: scroll_width * 8 and scroll_height * 8
   // At scale: multiply by scale
   area_width = scroll_width * 8 * ui_scale;  // 18 * 8 * scale
   area_height = scroll_height * 8 * ui_scale; // 10 * 8 * scale
   // Korean mode: x is already screen-relative (from panel_start), y includes y_off
   // Don't add x_off again, but add y_off for vertical centering
   DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll Init: x=%d, y=%d, x_off=%d, y_off=%d, final_x=%d, final_y=%d\n",
         x, y, x_off, y_off, x, y + y_off);
   GUI_Widget::Init(NULL, x, y + y_off, area_width, area_height);
 }
 else
 {
   area_width = scroll_width * 8;
   area_height = scroll_height * 8;
   GUI_Widget::Init(NULL, x+x_off, y+y_off, area_width, area_height);
 }

 cursor_char = 0;
 cursor_x = 0;
 cursor_y = scroll_height-1;
 line_count = 0;

 cursor_wait = 0;

 display_pos = 0;
 
 bg_color = Game::get_game()->get_palette()->get_bg_color();

 capitalise_next_letter = false;

 left_margin = 0;

 add_new_line();
}

MsgScroll::~MsgScroll()
{
 std::list<MsgLine *>::iterator msg_line;
 std::list<MsgText *>::iterator msg_text;

 // delete the scroll buffer
 for(msg_line = msg_buf.begin(); msg_line != msg_buf.end(); msg_line++)
   delete *msg_line;

 // delete the holding buffer
 for(msg_text = holding_buffer.begin(); msg_text != holding_buffer.end(); msg_text++)
   delete *msg_text;

}

bool MsgScroll::init(char *player_name)
{
 std::string prompt_string;

 // Player name should not be translated - it's user input
 prompt_string.append(player_name);
 if(game_type==NUVIE_GAME_U6)
 {
   prompt_string.append(":\n");
 }

 prompt_string.append(">");

 if(set_prompt((char *)prompt_string.c_str(),font) == false)
   return false;

 set_input_mode(false);

 return true;
}

void MsgScroll::set_scroll_dimensions(uint16 w, uint16 h)
{
  scroll_width = w;
  scroll_height = h;

  cursor_char = 0;
  cursor_x = 0;
  cursor_y = scroll_height-1;
  line_count = 0;

  cursor_wait = 0;

  display_pos = 0;
}

int MsgScroll::printf(std::string format, ...)
{

  va_list ap;
  int printed=0;
  static size_t bufsize=128; // will be resized if needed
  static char * buffer=(char *) malloc(bufsize); // static so we don't have to reallocate all the time.
  
  while(1) {
    if (buffer==NULL) {
      DEBUG(0,LEVEL_ALERT,"MsgScroll::printf: Couldn't allocate %d bytes for buffer\n",bufsize);
      /* try to shrink the buffer to at least have a change next time,
       * but if we're low on memory probably have worse issues...
       */
      bufsize>>=1;
      buffer=(char *) malloc(bufsize); // no need to check now.
      /*
       * We don't retry, or if we need e.g. 4 bytes for the format and
       * there's only 3 available, we'd grow and shrink forever.
       */
      return printed;
    }

    /* try formatting */
    va_start(ap,format);
    printed=vsnprintf(buffer,bufsize,format.c_str(),ap);
    va_end(ap);

    if (printed<0) {
      DEBUG(0,LEVEL_ERROR,"MsgScroll::printf: vsnprintf returned < 0: either output error or glibc < 2.1\n");
      free(buffer);
      bufsize*=2; // In case of an output error, we'll just keep doubling until the malloc fails.
      buffer=(char *) malloc(bufsize); // if this fails, will be caught later on
      /* try again */
      continue;
    } else if ((size_t)printed>=bufsize) {
      DEBUG(0,LEVEL_DEBUGGING,"MsgScroll::printf: needed buffer of %d bytes, only had %d bytes.\n",printed+1,bufsize);
      bufsize=printed+1; //this should be enough
      free(buffer);
      buffer=(char *) malloc(bufsize); // if this fails, will be caught later on
      /* try again */
      continue;
    }
    /* if we're here, formatting probably worked. We can stop looping */
    break;
  }
  /* use the string */
  display_string(buffer);

  return printed;
}

void MsgScroll::display_fmt_string(const char *format, ...)
{
	char buf[1024];
	const char *use_format = format;
	std::string translated;
	KoreanTranslation *kt = Game::get_game() ? Game::get_game()->get_korean_translation() : NULL;
	if (kt && kt->isEnabled())
	{
		translated = kt->translate(format);
		use_format = translated.c_str();
	}
	memset(buf, 0, 1024);
	va_list args;
	va_start(args, format);
	vsnprintf(buf, 1024, use_format, args);
	va_end(args);

	display_string(buf);
}

void MsgScroll::display_string(std::string s, uint16 length, uint8 lang_num)
{

}

void MsgScroll::display_string(std::string s, bool include_on_map_window)
{
 display_string(s,font, include_on_map_window);
}

void MsgScroll::display_string(std::string s, uint8 color, bool include_on_map_window)
{
  display_string(s,font, color, include_on_map_window);
}

void MsgScroll::display_string(std::string s, Font *f, bool include_on_map_window)
{
  display_string(s,f,font_color,include_on_map_window);
}

void MsgScroll::display_string(std::string s, Font *f, uint8 color, bool include_on_map_window)
{
  MsgText *msg_text;

 if(s.empty())
   return;

 KoreanTranslation *kt = Game::get_game() ? Game::get_game()->get_korean_translation() : NULL;
 if (kt && kt->isEnabled())
   s = kt->translate(s);

 if(f == NULL)
   f = font;

 msg_text = new MsgText(s, f);
 msg_text->color = color;
 fprintf(stdout,"%s",msg_text->s.c_str());
 fflush(stdout);

 holding_buffer.push_back(msg_text);

 process_holding_buffer();

}

// process text tokens till we either run out or hit a page break.

 void MsgScroll::process_holding_buffer()
 {
  MsgText *token;

  if(!page_break)
    {
     token = holding_buffer_get_token();

     for( ; token != NULL && !page_break; )
       {
        parse_token(token);
        delete token;
        scroll_updated = true;

        if(!page_break)
          token = holding_buffer_get_token();
       }
    }
 }

 MsgText *MsgScroll::holding_buffer_get_token()
 {
  MsgText *input, *token;
  int i;

  if(holding_buffer.empty())
    return NULL;

  input = holding_buffer.front();

  if(input->font == NULL)
    {
     line_count = 0;
     holding_buffer.pop_front();
     delete input;
     return NULL;
    }

  i = input->s.find_first_of(" \t\n*<>`",0);
  if(i == 0) i++;

  if(i == -1)
    i = input->s.length();

  if(i > 0)
   {
    token = new MsgText(input->s.substr(0,i), font); // FIX maybe a format flag. // input->font);
    token->color = input->color;
	input->s.erase(0,i);
	if(input->s.length() == 0)
	  {
	   holding_buffer.pop_front();
	   delete input;
	  }
	return token;
   }

  return NULL;
 }

bool MsgScroll::can_fit_token_on_msgline(MsgLine *msg_line, MsgText *token)
{
  // Check if Korean font is enabled - use pixel-based width
  FontManager *font_manager = Game::get_game()->get_font_manager();
  KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
  KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
  bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
  bool compact_ui = Game::get_game()->is_compact_ui();
  KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;

  if(use_korean)
  {
    // Use pixel-based width calculation for Korean
    uint16 line_width_px = msg_line->get_display_width();
    uint16 token_width_px = korean_font->getStringWidthUTF8(token->s.c_str(), 1);
    // area.w is the actual scroll area width (scaled in Korean mode)
    if(line_width_px + token_width_px > area.w)
    {
      return false;
    }
  }
  else
  {
    // Original byte-based calculation for ASCII
    if(msg_line->total_length + token->length() > scroll_width)
    {
      return false; //token doesn't fit on the current line.
    }
  }

  return true;
}

bool MsgScroll::parse_token(MsgText *token)
{
 MsgLine *msg_line = NULL;

 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 uint16 visible_lines = scroll_height;
 if(use_korean)
 {
   // Korean font height: 24 for compact_ui, 32 for normal
   uint16 font_height = korean_font->getCharHeight();
   if(font_height > 0)
     visible_lines = area.h / font_height;
   if(visible_lines == 0)
     visible_lines = 1;
 }

 if (!msg_buf.empty())
   msg_line = msg_buf.back(); // retrieve the last line from the scroll buffer.

 switch(token->s[0])
   {
    case '\n' :  add_new_line();
                 break;

    case '\t' :  set_autobreak(false);
                 break;

    case '`'  :  capitalise_next_letter = true;
                 break;

    case '<'  :  set_font(NUVIE_FONT_GARG); // runic / gargoyle font
                 break;

    case '>'  :  if(is_garg_font())
                    {
                     set_font(NUVIE_FONT_NORMAL); // english font
                     break;
                    }
                  // Note fall through. ;-) We fall through if we haven't already seen a '<' char

    default   :  if(msg_line)
    {
      if(can_fit_token_on_msgline(msg_line, token) == false) // the token is to big for the current line
      {
        msg_line = add_new_line();
      }
      // This adds extra newlines. (SB-X)
      //                 if(msg_line->total_length + token->length() == scroll_width) //we add a new line but write to the old line.
      //                    add_new_line();

      if(msg_line->total_length == 0 && token->s[0] == ' ' && discard_whitespace) // discard whitespace at the start of a line.
        return true;
    }
    if(token->s[0] == '*')
    {
      if(just_finished_page_break == false) //we don't want to add another break if we've only just completed an autobreak
        set_page_break();
    }
    else
    {
      if(capitalise_next_letter)
      {
        token->s[0] = toupper(token->s[0]);
        capitalise_next_letter = false;
      }

      if(msg_line == NULL)
      {
        msg_line = add_new_line();
      }

      add_token(token);
      if(talking || Game::get_game()->get_event()->get_mode() == INPUT_MODE
          || Game::get_game()->get_event()->get_mode() == KEYINPUT_MODE)
      {
        bool line_full = false;
        if(use_korean)
        {
          // Korean font: 16px wide per character
          uint16 line_width_px = msg_line->get_display_width();
          line_full = (line_width_px >= area.w);
        }
        else
        {
          line_full = (msg_line->total_length == scroll_width);
        }
        if(line_full)
        {
          msg_line = add_new_line();
        }
      }
    }
    break;
   }

if(msg_buf.size() > visible_lines)
   display_pos = (uint16)(msg_buf.size() - visible_lines);
 just_finished_page_break = false;
 just_displayed_prompt = false;
 return true;
}

void MsgScroll::add_token(MsgText *token)
{
	msg_buf.back()->append(token);
}

bool MsgScroll::remove_char()
{
 MsgLine *msg_line;

 msg_line = msg_buf.back(); // retrieve the last line from the scroll buffer.
 msg_line->remove_char();

 if(msg_line->total_length == 0) // remove empty line from scroll buffer
   {
    msg_buf.pop_back();
    delete msg_line;
   }

 return true;
}

void MsgScroll::set_font(uint8 font_type)
{
	font = Game::get_game()->get_font_manager()->get_font(font_type); // 0 = normal english; 1 = runic / gargoyle font
}

bool MsgScroll::is_garg_font()
{
	return (font == Game::get_game()->get_font_manager()->get_font(NUVIE_FONT_GARG));
}

void MsgScroll::clear_scroll()
{
	std::list<MsgLine *>::iterator iter;

	for(iter=msg_buf.begin();iter !=msg_buf.end();iter++)
	{
		MsgLine *line = *iter;
		delete line;
	}

	msg_buf.clear();
	line_count = 0;
  display_pos=0;
  scroll_updated = true;
	add_new_line();
}

void MsgScroll::delete_front_line()
{
	MsgLine* msg_line_front = msg_buf.front();
	msg_buf.pop_front();
	delete msg_line_front;
}

MsgLine *MsgScroll::add_new_line()
{
 MsgLine *msg_line = new MsgLine();
 msg_buf.push_back(msg_line);
 line_count++;

 if(msg_buf.size() > scrollback_height)
 {
   delete_front_line();
 }

 uint16 visible_lines = scroll_height;
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 if(use_korean)
 {
   // Korean font height: 24 for compact_ui, 32 for normal
   uint16 font_height = korean_font->getCharHeight();
   if(font_height > 0)
     visible_lines = area.h / font_height;
   if(visible_lines == 0)
     visible_lines = 1;
 }

 if(autobreak && line_count > visible_lines - 1)
     set_page_break();

 return msg_line;
}

bool MsgScroll::set_prompt(const char *new_prompt, Font *f)
{

 prompt.s.assign(new_prompt);
 prompt.font = f;

 return true;
}

void MsgScroll::display_prompt()
{
 if(!talking && !just_displayed_prompt)
  { 
 //line_count = 0;
	   display_string(prompt.s, prompt.font, MSGSCROLL_NO_MAP_DISPLAY);
 //line_count = 0;

   clear_page_break();
   just_displayed_prompt = true;
  }
}

void MsgScroll::display_converse_prompt()
{
  FontManager *fm = Game::get_game()->get_font_manager();
  if (fm && fm->is_korean_enabled() && fm->get_korean_font())
    display_string("\n말하기:", MSGSCROLL_NO_MAP_DISPLAY);
  else
    display_string("\nyou say:", MSGSCROLL_NO_MAP_DISPLAY);
}

void MsgScroll::set_keyword_highlight(bool state)
{
 keyword_highlight = state;
}

void MsgScroll::set_permitted_input(const char *allowed)
{
	permit_input = allowed;
	if(permit_input) {
		if(strcmp(permit_input, "yn") == 0)
			yes_no_only = true;
		else if(strncmp(permit_input, "0123456789", strlen(permit_input)) == 0)
			numbers_only = true;
		else if(game_type == NUVIE_GAME_U6 && strcmp(permit_input, "ayn") == 0) // Heftimus npc 47
			aye_nay_only = true;
	}
}

void MsgScroll::clear_permitted_input()
{
	permit_input = NULL;
	yes_no_only = false;
	numbers_only = false;
	aye_nay_only = false;
}

void MsgScroll::set_input_mode(bool state, const char *allowed, bool can_escape, bool use_target_cursor, bool set_numbers_only_to_true)
{
 bool do_callback = false;

 input_mode = state;
 clear_permitted_input();
 permit_inputescape = can_escape;
 using_target_cursor = use_target_cursor;
 if(set_numbers_only_to_true)
   numbers_only = true;

 line_count = 0;

 clear_page_break();

 if(input_mode == true)
 {
   if(allowed && strlen(allowed))
      set_permitted_input(allowed);
   //FIXME SDL2 SDL_EnableUNICODE(1); // allow character translation
   input_buf.erase(0,input_buf.length());
   composing_text.clear();  // Clear IME composition text
   SDL_StartTextInput();  // Enable text input for Korean IME
 }
 else
 {
   //FIXME SDL2 SDL_EnableUNICODE(0); // reduce translation overhead when not needed
   SDL_StopTextInput();  // Disable text input
   if(callback_target)
     do_callback = true; // **DELAY until end-of-method so callback can set_input_mode() again**
 }
 Game::get_game()->get_gui()->lock_input((input_mode && !using_target_cursor) ? this : NULL);

 // send whatever input was collected to target that requested it
 if(do_callback)
 {
   CallBack *requestor = callback_target; // copy to temp
   char *user_data = callback_user_data;
   cancel_input_request(); // clear originals (callback may request again)

   std::string input_str = input_buf;
   requestor->set_user_data(user_data); // use temp requestor/user_data
   requestor->callback(MSGSCROLL_CB_TEXT_READY, this, &input_str);
 }
}

void MsgScroll::move_scroll_down()
{
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 uint16 visible_lines = scroll_height;
 if(use_korean)
 {
   // Korean font height: 24 for compact_ui, 32 for normal
   uint16 font_height = korean_font->getCharHeight();
   if(font_height > 0)
     visible_lines = area.h / font_height;
   if(visible_lines == 0)
     visible_lines = 1;
 }

 if(msg_buf.size() > visible_lines && display_pos < msg_buf.size() - visible_lines)
 {
   display_pos++;
   scroll_updated = true;
 }
}

void MsgScroll::move_scroll_up()
{
 if(display_pos > 0)
 {
   display_pos--;
   scroll_updated = true;
 }
}

void MsgScroll::page_up()
{
 uint8 i=0;
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 uint16 visible_lines = scroll_height;
 if(use_korean)
 {
   // Korean font height: 24 for compact_ui, 32 for normal
   uint16 font_height = korean_font->getCharHeight();
   if(font_height > 0)
     visible_lines = area.h / font_height;
   if(visible_lines == 0)
     visible_lines = 1;
 }

 for(; display_pos > 0 && i < visible_lines; i++)
   display_pos--;
 if(i > 0)
   scroll_updated = true;
}

void MsgScroll::page_down()
{
 uint8 i=0;
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 uint16 visible_lines = scroll_height;
 if(use_korean)
 {
   // Korean font height: 24 for compact_ui, 32 for normal
   uint16 font_height = korean_font->getCharHeight();
   if(font_height > 0)
     visible_lines = area.h / font_height;
   if(visible_lines == 0)
     visible_lines = 1;
 }

 for(; msg_buf.size() > visible_lines && i < visible_lines
     && display_pos < msg_buf.size() - visible_lines ; i++)
   display_pos++;
 if(i > 0)
   scroll_updated = true;
}

void MsgScroll::process_page_break()
{
    page_break = false;
    just_finished_page_break = true;
    if(!input_mode)
        Game::get_game()->get_gui()->unlock_input();
    process_holding_buffer(); // Process any text in the holding buffer.
    just_displayed_prompt = true;
}

/* Take input from the main event handler and do something with it
 * if necessary.
 */
GUI_status MsgScroll::KeyDown(SDL_Keysym key)
{
    char ascii = get_ascii_char_from_keysym(key);

    if(page_break == false && input_mode == false)
        return(GUI_PASS);

    bool is_printable = isprint(ascii);
    KeyBinder *keybinder = Game::get_game()->get_keybinder();
    ActionType a = keybinder->get_ActionType(key);
    ActionKeyType action_key_type = keybinder->GetActionKeyType(a);

    if(using_target_cursor && !is_printable && action_key_type <= DO_ACTION_KEY) // directional keys, toggle_cursor, and do_action
        return GUI_PASS;

    if(!input_mode || !is_printable)
    {
        switch(action_key_type)
        {
            case WEST_KEY: key.sym = SDLK_LEFT; break;
            case EAST_KEY: key.sym = SDLK_RIGHT; break;
            case SOUTH_KEY: key.sym = SDLK_DOWN; break;
            case NORTH_KEY: key.sym = SDLK_UP; break;
            case CANCEL_ACTION_KEY: key.sym = SDLK_ESCAPE; break;
            case DO_ACTION_KEY: key.sym = SDLK_RETURN; break;
            case MSGSCROLL_UP_KEY: key.sym = SDLK_PAGEUP; break;
            case MSGSCROLL_DOWN_KEY: key.sym = SDLK_PAGEDOWN; break;
            default: if(keybinder->handle_always_available_keys(a)) return GUI_YUM; break;
        }
    }
    switch(key.sym)
     {
      case SDLK_UP : if(input_mode) break; //will select printable ascii
                     move_scroll_up();
                     return (GUI_YUM);

      case SDLK_DOWN: if(input_mode) break; //will select printable ascii
                      move_scroll_down();
                      return (GUI_YUM);
      case SDLK_PAGEUP: if(Game::get_game()->is_new_style())
                            move_scroll_up();
                        else page_up();
                        return (GUI_YUM);
      case SDLK_PAGEDOWN: if(Game::get_game()->is_new_style())
                              move_scroll_down();
                          else page_down();
                          return (GUI_YUM);
      default : break;
     }

    if(page_break)
      {
       process_page_break();
       return(GUI_YUM);
      }

    switch(key.sym)
    {
        case SDLK_ESCAPE: if(permit_inputescape)
                          {
                            // reset input buffer
                            permit_input = NULL;
                            if(input_mode)
                              set_input_mode(false);
                          }
                          return(GUI_YUM);
        case SDLK_KP_ENTER:
        case SDLK_RETURN: if(permit_inputescape || input_char != 0 || !composing_text.empty())
                          {
                            // Add any composing text (Korean IME) to input buffer before finalizing
                            if(!composing_text.empty())
                            {
                              // Y/N mode: map Korean composing text to Y or N
                              if(yes_no_only)
                              {
                                char mapped_char = 'N';  // Default to N
                                // Yes keywords: ㅛ, 예, 응, 어, 네, ㅇ
                                if(composing_text == "ㅛ" || composing_text == "예" ||
                                   composing_text == "응" || composing_text == "어" ||
                                   composing_text == "네" || composing_text == "ㅇ")
                                  mapped_char = 'Y';
                                input_buf_add_char(mapped_char);
                              }
                              else
                              {
                                input_buf_add_string(composing_text.c_str());
                              }
                              composing_text.clear();
                            }
                            if(input_char != 0)
                              input_buf_add_char(get_char_from_input_char());
                            if(input_mode)
                              set_input_mode(false);
                          }
                          return(GUI_YUM);
        case SDLK_RIGHT:
                        if(input_char != 0 && permit_input == NULL)
                          input_buf_add_char(get_char_from_input_char());
                        break;
        case SDLK_DOWN:
                       increase_input_char();
                       break;
        case SDLK_UP:
                     decrease_input_char();
                     break;
        case SDLK_LEFT :
        case SDLK_BACKSPACE :
                            if(input_mode) {
                              if(input_char != 0)
                                  input_char = 0;
                              else if(!composing_text.empty())
                              {
                                  // Clear IME composing text first
                                  composing_text.clear();
                                  scroll_updated = true;
                              }
                              else
                                  input_buf_remove_char();
                            }
                            break;
        case SDLK_LSHIFT :
            return(GUI_YUM);
        case SDLK_RSHIFT :
            return(GUI_YUM);
        default: // alphanumeric characters
#if SDL_VERSION_ATLEAST(2, 0, 0)
                 // SDL2: printable characters are handled by TextInput() to support IME
                 // Only handle non-printable special keys here
                 if(input_mode && is_printable)
                 {
                   // Let TextInput handle printable characters
                   return(GUI_YUM);
                 }
#else
                 if(input_mode && is_printable)
                  {
                   if(permit_input == NULL)
                   {
                     if(!numbers_only || isdigit(ascii))
                     {
                       if(input_char != 0)
                         input_buf_add_char(get_char_from_input_char());
                       input_buf_add_char(ascii);
                     }
                   }
                   else if(strchr(permit_input, ascii) || strchr(permit_input, tolower(ascii)))
                   {
                    input_buf_add_char(toupper(ascii));
                    set_input_mode(false);
                   }
                  }
#endif
            break;
    }

 return(GUI_YUM);
}

GUI_status MsgScroll::TextInput(const char *text)
{
    DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll::TextInput: received text='%s', input_mode=%d, numbers_only=%d\n",
          text ? text : "(null)", input_mode, numbers_only);
    if(!input_mode || text == NULL || text[0] == '\0')
        return GUI_PASS;

    // Clear composing text since we got a finalized character
    composing_text.clear();

    // In converse mode (permit_input == NULL), accept any text
    // In limited input mode (permit_input != NULL), only accept single chars
    if(permit_input == NULL)
    {
        if(!numbers_only)
        {
            input_buf_add_string(text);
        }
        else
        {
            // numbers_only mode - only accept if text is digits
            for(const char *p = text; *p; ++p)
            {
                DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll::TextInput: checking char '%c' (0x%02x), isdigit=%d\n",
                      *p, (unsigned char)*p, isdigit(*p));
                if(isdigit(*p))
                    input_buf_add_char(*p);
            }
        }
        return GUI_YUM;
    }
    else
    {
        // Limited input mode - check each character
        for(const char *p = text; *p; ++p)
        {
            char c = *p;
            if(strchr(permit_input, c) || strchr(permit_input, tolower(c)))
            {
                input_buf_add_char(toupper(c));
                set_input_mode(false);
                return GUI_YUM;
            }
        }

        // Y/N mode: Korean input handling
        // Yes keywords -> Y, everything else -> N
        if(yes_no_only && text != NULL && strlen(text) > 0)
        {
            char mapped_char = 'N';  // Default to N for unrecognized input

            // Check for Yes keywords
            // Korean keyboard raw: j = ㅛ
            // UTF-8 Korean: ㅛ, 예, 응, 어, 네, ㅇ
            if(strlen(text) == 1 && (text[0] == 'j' || text[0] == 'J'))
                mapped_char = 'Y';
            else if(strcmp(text, "ㅛ") == 0 || strcmp(text, "예") == 0 ||
                    strcmp(text, "응") == 0 || strcmp(text, "어") == 0 ||
                    strcmp(text, "네") == 0 || strcmp(text, "ㅇ") == 0)
                mapped_char = 'Y';

            if(strchr(permit_input, mapped_char))
            {
                input_buf_add_char(mapped_char);
                set_input_mode(false);
                return GUI_YUM;
            }
        }

        // Consume invalid input to prevent error messages
        return GUI_YUM;
    }

    return GUI_PASS;
}

GUI_status MsgScroll::TextEditing(const char *text, int start, int length)
{
    if(!input_mode)
        return GUI_PASS;

    // Store the composing text for display
    // This is the text being composed by the IME (e.g., Korean "ㄱ" before becoming "그")
    std::string new_composing = (text != NULL) ? text : "";

    if(composing_text != new_composing)
    {
        composing_text = new_composing;

        // Force redraw to show/hide composing text
        scroll_updated = true;
    }

    return GUI_YUM;
}

GUI_status MsgScroll::MouseWheel(sint32 x, sint32 y)
{
    if(page_break) // any click == scroll-to-end
    {
        process_page_break();
        return(GUI_YUM);
    }

    Game *game = Game::get_game();

    if(game->is_new_style())
    {
        if(!input_mode)
            return GUI_PASS;
        if (y > 0)
            move_scroll_up();
        if (y < 0)
            move_scroll_down();
    }
    else
    {
        if (y > 0)
            page_up();
        if (y < 0)
            page_down();
    }
    return GUI_YUM;
}

GUI_status MsgScroll::MouseUp(int x, int y, int button)
{
 uint16 i;
 std::string token_str;

    if(page_break) // any click == scroll-to-end
    {
        process_page_break();
        return(GUI_YUM);
    }

    if(button == 1) // left click == select word
    {
     if(input_mode)
       {
        token_str = get_token_string_at_pos(x,y);
        if(permit_input != NULL) {
            if(strchr(permit_input, token_str[0])
               || strchr(permit_input, tolower(token_str[0])))
            {
                input_buf_add_char(token_str[0]);
                set_input_mode(false);
            }
            return(GUI_YUM);
        }

        for(i=0;i < token_str.length(); i++)
        {
        	if(isalnum(token_str[i]))
        		input_buf_add_char(token_str[i]);
        }

       }
     else if(!Game::get_game()->is_new_style())
        Game::get_game()->get_event()->cancelAction();
    }
    else if(button == 3) // right click == send input
    {
      if(input_mode)
      {
        if(permit_inputescape)
        {
            set_input_mode(false);
            return(GUI_YUM);
        }
      }
      else if(!Game::get_game()->is_new_style())
            Game::get_game()->get_event()->cancelAction();
    }
    return(GUI_PASS);
}

std::string MsgScroll::get_token_string_at_pos(uint16 x, uint16 y)
{
 uint16 i;
 sint32 buf_x, buf_y;
 MsgText *token = NULL;
 std::list<MsgLine *>::iterator iter;

 // Check if Korean font is enabled
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 // Korean font height: 24 for compact_ui, 32 for normal
 uint16 font_height = use_korean ? korean_font->getCharHeight() : 8;
 uint16 visible_lines = scroll_height;
 if(use_korean)
 {
   if(font_height > 0)
     visible_lines = area.h / font_height;
   if(visible_lines == 0)
     visible_lines = 1;
 }

 buf_y = (y - area.y) / font_height;

 if(use_korean)
 {
   // For Korean, we need pixel-based x position
   buf_x = x - area.x;
   if(buf_x < 0 || buf_x >= (sint32)(scroll_width * 8) ||
      buf_y < 0 || buf_y >= visible_lines)
       return "";
 }
 else
 {
   buf_x = (x - area.x) / 8;
   if(buf_x < 0 || buf_x >= scroll_width || // click not in MsgScroll area.
      buf_y < 0 || buf_y >= scroll_height)
       return "";
 }

 if(msg_buf.size() <= visible_lines)
   {
    if((sint32)msg_buf.size() < buf_y + 1)
      return "";
   }
 else
   {
    buf_y = display_pos + buf_y;
   }

 for(i=0,iter=msg_buf.begin(); i < buf_y && iter != msg_buf.end();)
  {
   iter++;
   i++;
  }

 if(iter != msg_buf.end())
   {
    if(use_korean)
    {
      // For Korean, find token by pixel position (16x16 * 2 = 32)
      MsgLine *line = *iter;
      uint16 pos_x = 0;
      std::list<MsgText *>::iterator text_iter;
      for(text_iter = line->text.begin(); text_iter != line->text.end(); text_iter++)
      {
        MsgText *t = *text_iter;
        uint16 token_width = korean_font->getStringWidthUTF8(t->s.c_str(), 1);
        if(buf_x >= pos_x && buf_x < pos_x + token_width)
        {
          DEBUG(0,LEVEL_DEBUGGING,"Token at pixel (%d,%d) = %s\n",buf_x, buf_y, t->s.c_str());
          return t->s;
        }
        pos_x += token_width;
      }
    }
    else
    {
      token = (*iter)->get_text_at_pos(buf_x);
      if(token)
      {
         DEBUG(0,LEVEL_DEBUGGING,"Token at (%d,%d) = %s\n",buf_x, buf_y, token->s.c_str());
         return token->s;
      }
    }
   }

 return "";
}

void MsgScroll::Display(bool full_redraw)
{
 uint16 i;
 std::list<MsgLine *>::iterator iter;
 MsgLine *msg_line = NULL;

 // Check if Korean font is enabled for pixel-based positioning
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 // Korean font height: 24 for compact_ui, 32 for normal
 uint16 font_height = use_korean ? korean_font->getCharHeight() : 8;

 // Calculate visible lines based on actual font height
 // area.h is total height in pixels, divide by font_height to get lines
 uint16 visible_lines = use_korean ? (area.h / font_height) : scroll_height;

 // In Korean 4x mode, always redraw since other UI elements may overlap
 if(scroll_updated || full_redraw || Game::get_game()->is_original_plus_full_map() || use_korean)
  {
   screen->fill(bg_color,area.x, area.y, area.w, area.h); //clear whole scroll

   iter=msg_buf.begin();
   for(i=0;i < display_pos; i++)
	  iter++;

   for(i=0;i< visible_lines && iter != msg_buf.end();i++,iter++)
     {
	  msg_line = *iter;
	  drawLine(screen, msg_line, i);
     }
   scroll_updated = false;

   screen->update(area.x,area.y, area.w, area.h);

   cursor_y = i-1;
   if(msg_line)
    {
     if(use_korean)
     {
       // For Korean, cursor_x is stored as pixel offset
       cursor_x = msg_line->get_display_width();
       // Account for cursor drawing offsets: text_padding + cursor_size (no extra offset)
       uint8 cursor_scale = compact_ui ? 2 : 4;
       uint16 cursor_size = 8 * cursor_scale;
       uint16 text_pad = compact_ui ? 8 : 0;
       uint16 cursor_offset = text_pad + cursor_size;
       uint16 scroll_width_px = area.w - left_margin - cursor_offset;
       if(cursor_x >= scroll_width_px)
       {
         // Cursor needs to wrap to next line
         if(cursor_y + 1 < visible_lines)
         {
           cursor_y++;
         }
         else
         {
           // At last visible line - scroll the message buffer
           display_pos++;
           scroll_updated = true;
         }
         cursor_x = 0;
       }
     }
     else
     {
       cursor_x = msg_line->total_length;
       if(cursor_x == scroll_width) // don't draw the cursor outside the scroll (SB-X)
       {
         if(cursor_y+1 < scroll_height)
           cursor_y++;
         cursor_x = 0;
       }
     }
    }
   else
     cursor_x = area.x;
  }
 else
  {
   if(use_korean)
   {
     // Clear a wider area to ensure cursor and composing text are fully erased
     // Cursor draws at: area.x + left_margin + cursor_x + text_padding + composing_width + 4 + 8 (internal offset)
     // We need to clear from cursor_x position with enough width for both composing text and cursor
     uint8 cursor_scale = compact_ui ? 2 : 4;
     uint16 cursor_size = 8 * cursor_scale;
     uint16 text_padding = compact_ui ? 8 : 0;
     // Clear starting from same position as cursor (with +8 internal offset)
     uint16 clear_x = area.x + left_margin + cursor_x + text_padding + 4 + 8;
     uint16 clear_y = area.y + cursor_y * font_height + 4;
     // Clear width: composing area (up to 64px) + cursor_size
     screen->fill(bg_color, clear_x, clear_y, 64 + cursor_size, cursor_size);
   }
   else
     clearCursor(area.x + 8 * cursor_x, area.y + cursor_y * 8);
  }

 // For Korean mode, always show cursor when input is active (cursor position is managed separately)
 // For original mode, only show cursor on last page
 bool show_cursor_now = show_cursor && (use_korean || msg_buf.size() <= visible_lines || display_pos == msg_buf.size() - visible_lines);
 if(show_cursor_now)
 {
   uint16 composing_width = 0;
   // Text padding from left edge: 8 pixels in compact_ui mode (same as drawLine)
   uint16 text_padding = (use_korean && compact_ui) ? 8 : 0;

   // Draw composing text (IME composition preview) before cursor
   if(use_korean && !composing_text.empty() && input_mode)
   {
     // Use same coordinate system as drawLine (text): area.x + left_margin + cursor_x + text_padding
     uint16 draw_x = area.x + left_margin + cursor_x + text_padding;
     uint16 draw_y = area.y + cursor_y * font_height;

     // Draw the composing text with underline to indicate it's being composed
     composing_width = korean_font->drawStringUTF8(screen, composing_text.c_str(),
                         draw_x, draw_y, get_input_font_color(), font_highlight_color, 1);

     // Draw underline under composing text to show it's not finalized
     uint16 underline_y = draw_y + korean_font->getCharHeight() - 2;
     screen->fill(get_input_font_color(), draw_x, underline_y, composing_width, 1);

     screen->update(draw_x, draw_y, composing_width, korean_font->getCharHeight());
   }

   if(use_korean)
   {
     // Cursor position: right after the text (no extra offset)
     uint16 cursor_draw_x = area.x + left_margin + cursor_x + text_padding + composing_width;
     uint16 cursor_draw_y = area.y + cursor_y * font_height + 4;
     uint8 cursor_scale = compact_ui ? 2 : 3;  // 3x for 4x UI mode (24px cursor)
     uint16 cursor_size = 8 * cursor_scale;
     // Check if cursor fits within area bounds
     bool x_ok = (cursor_draw_x + cursor_size <= area.x + area.w);
     // For y check, use font_height instead of cursor_size since cursor sits on the text line
     bool y_ok = (cursor_draw_y + font_height <= area.y + area.h);
     if(x_ok && y_ok)
       drawCursor(cursor_draw_x, cursor_draw_y);
   }
   else
     drawCursor(area.x + left_margin + 8 * cursor_x, area.y + cursor_y * 8);
 }

}

inline void MsgScroll::drawLine(Screen *screen, MsgLine *msg_line, uint16 line_y)
{
 MsgText *token;
 std::list<MsgText *>::iterator iter;
 uint16 total_x = 0;

 // Check if Korean font is available
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 // Select active font: 24x24 for 3x UI, 32x32 for 4x UI
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 uint8 korean_scale = 1;
 // Font height: 24 for compact_ui, 32 for normal Korean, 8 for English
 uint16 font_height = use_korean ? korean_font->getCharHeight() : 8;
 // Text padding from left edge: 8 pixels in compact_ui mode to avoid text overflow
 uint16 text_padding = (use_korean && compact_ui) ? 8 : 0;

 for(iter=msg_line->text.begin();iter != msg_line->text.end() ; iter++)
   {
    token = *iter;
    if(use_korean)
    {
      // Use Korean font for UTF-8 string rendering
      uint16 width = korean_font->drawStringUTF8(screen, token->s.c_str(),
                       area.x + left_margin + total_x + text_padding, area.y + line_y * font_height,
                       token->color, font_highlight_color, korean_scale);
      total_x += width;
    }
    else
    {
      // Original ASCII rendering
      token->font->drawString(screen, token->s.c_str(), area.x + left_margin + total_x, area.y+line_y*8, token->color, font_highlight_color);
      total_x += token->s.length() * 8;
    }
   }
}

void MsgScroll::clearCursor(uint16 x, uint16 y)
{
 // Use appropriate size based on cursor scale
 FontManager *font_manager = Game::get_game()->get_font_manager();
 bool use_korean = font_manager && font_manager->is_korean_enabled() &&
                   font_manager->get_korean_font() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 // Cursor size: 16 for compact_ui (8*2), 24 for normal 4x (8*3), 8 for original
 uint8 cursor_scale = use_korean ? (compact_ui ? 2 : 3) : 1;
 uint16 clear_size = 8 * cursor_scale;
 // x,y are already calculated by caller, just fill
 screen->fill(bg_color, x, y, clear_size, clear_size);
}

void MsgScroll::drawCursor(uint16 x, uint16 y)
{
 uint8 cursor_color = input_mode ? get_input_font_color() : font_color;

 // Get cursor sizing
 FontManager *font_manager = Game::get_game()->get_font_manager();
 KoreanFont *korean_font_32 = font_manager ? font_manager->get_korean_font() : NULL;
 KoreanFont *korean_font_24 = font_manager ? font_manager->get_korean_font_24() : NULL;
 bool use_korean = korean_font_32 && font_manager->is_korean_enabled() && Game::get_game()->is_original_plus();
 bool compact_ui = Game::get_game()->is_compact_ui();
 KoreanFont *korean_font = (compact_ui && korean_font_24) ? korean_font_24 : korean_font_32;
 // Cursor size: 16 for compact_ui (8*2), 24 for normal 4x (8*3), 8 for original
 uint8 cursor_scale = use_korean ? (compact_ui ? 2 : 3) : 1;
 uint16 cursor_size = 8 * cursor_scale;
 uint8 korean_scale = cursor_scale; // Scale for U6 font cursor
 uint16 cursor_x_offset = 0; // no extra offset - cursor draws right after text
 x += cursor_x_offset;

 if(input_char != 0) { // show letter selected by arrow keys
    // For input_char, draw at text position (without cursor offset)
    uint16 text_y = use_korean ? (y - 8) : y;
    if(use_korean)
      korean_font->drawCharUnicode(screen, get_char_from_input_char(), x - 8, text_y, cursor_color, 1);
    else
      font->drawChar(screen, get_char_from_input_char(), x, y, cursor_color);
    screen->update(use_korean ? (x - 8) : x, text_y, cursor_size, cursor_size);
    return;
 }
 // Special characters (arrows, spinning ankh) are not in Korean font charmap
 // Use original U6 font with scaling in Korean mode
 if(page_break)
    {
     if(cursor_wait <= 2) // flash arrow
     {
       if(use_korean)
       {
         U6Font *u6font = static_cast<U6Font*>(font);
         u6font->drawCharScaled(screen, 1, x, y, cursor_color, korean_scale); // down arrow scaled
       }
       else
         font->drawChar(screen, 1, x, y, cursor_color); // down arrow (control char)
     }
	}
 else
 {
    if(use_korean)
    {
      U6Font *u6font = static_cast<U6Font*>(font);
      u6font->drawCharScaled(screen, cursor_char + 5, x, y, cursor_color, korean_scale); //spinning ankh scaled
    }
    else
      font->drawChar(screen, cursor_char + 5, x, y, cursor_color); //spinning ankh (control char)
 }

  screen->update(x, y, cursor_size, cursor_size);
  if(cursor_wait == MSGSCROLL_CURSOR_DELAY)
    {
     cursor_char = (cursor_char + 1) % 4;
     cursor_wait = 0;
    }
  else
     cursor_wait++;
}


void MsgScroll::set_page_break()
{
 line_count = 1;
 page_break = true;

 if(!input_mode)
   {
    Game::get_game()->get_gui()->lock_input(this);
   }

 return;
}

bool MsgScroll::input_buf_add_char(char c)
{
 MsgText token;
 input_char = 0;
 DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll::input_buf_add_char: adding char='%c' (0x%02x), numbers_only=%d\n",
       isprint(c) ? c : '?', (unsigned char)c, numbers_only);
 if(permit_input != NULL)
	input_buf_remove_char();
 input_buf.append(&c, 1);
 DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll::input_buf_add_char: input_buf is now '%s'\n", input_buf.c_str());
 scroll_updated = true;

 // Add char to scroll buffer

 token.s.assign(&c, 1);
 token.font = font;
 token.color = get_input_font_color();

 parse_token(&token);

 return true;
}

bool MsgScroll::input_buf_add_string(const char *str)
{
 if(str == NULL || str[0] == '\0')
     return false;

 MsgText token;
 input_char = 0;
 if(permit_input != NULL)
     input_buf_remove_char();
 input_buf.append(str);
 scroll_updated = true;

 // Add string to scroll buffer

 token.s.assign(str);
 token.font = font;
 token.color = get_input_font_color();

 parse_token(&token);

 return true;
}

bool MsgScroll::input_buf_remove_char()
{
 if(input_buf.length())
   {
    // Remove last UTF-8 character (handles multi-byte chars like Korean)
    size_t len = input_buf.length();
    size_t char_start = len - 1;

    // Find the start of the last UTF-8 character
    // UTF-8 continuation bytes start with 10xxxxxx (0x80-0xBF)
    while(char_start > 0 && (input_buf[char_start] & 0xC0) == 0x80)
    {
      char_start--;
    }

    // Erase from char_start to end
    input_buf.erase(char_start);
    scroll_updated = true;
    remove_char();

    return true;
   }

 return false;
}

bool MsgScroll::has_input()
{
 if(input_mode == false) //we only have input ready after the user presses enter.
   return true;

 return false;
}

std::string MsgScroll::get_input()
{
 // MsgScroll sets input_mode to false when it receives SDLK_ENTER
 std::string s;

 if(input_mode == false)
   {
    s.assign(input_buf);
   }
 DEBUG(0, LEVEL_INFORMATIONAL, "MsgScroll::get_input: returning '%s' (len=%d), input_mode=%d\n",
       s.c_str(), (int)s.length(), input_mode);
 fprintf(stdout,"%s",s.c_str());
 fflush(stdout);
 return s;
}

void MsgScroll::clear_page_break()
{
  MsgText *msg_text = new MsgText("", NULL);
  holding_buffer.push_back(msg_text);

  process_holding_buffer();
}

/* Set callback & callback_user_data so that a message will be sent to the
 * caller when input has been gathered. */
void MsgScroll::request_input(CallBack *caller, void *user_data)
{
    callback_target = caller;
    callback_user_data = (char *)user_data;
}

// 0 is no char, 1 - 26 is alpha, 27 is space, 28 - 37 is numbers
void MsgScroll::increase_input_char()
{
	if(permit_input != NULL && strcmp(permit_input, "\n") == 0) // blame hacky PauseEffect
		return;
	if(yes_no_only)
		input_char = input_char == 25 ? 14 : 25; 
	else if(aye_nay_only)
		input_char = input_char == 1 ? 14 : 1;
	else if(numbers_only)
		input_char = (input_char == 0 || input_char == 37) ? 28 : input_char + 1;
	else
		input_char = (input_char + 1) % 38;
	if(permit_input != NULL && !strchr(permit_input, get_char_from_input_char())) // might only be needed for the teleport cheat menu
		increase_input_char();
	scroll_updated = true; // Force redraw to clear previous input_char
}

void MsgScroll::decrease_input_char()
{
	if(permit_input != NULL && strcmp(permit_input, "\n") == 0) // blame hacky PauseEffect
		return;
	if(yes_no_only)
		input_char = input_char == 25 ? 14 : 25;
	else if(numbers_only)
		input_char = (input_char == 0 || input_char == 28) ? 37 : input_char - 1;
	else if(aye_nay_only)
		input_char = input_char == 1 ? 14 : 1;
	else
		input_char = input_char == 0 ? 37 : input_char - 1;
	if(permit_input != NULL && !strchr(permit_input, get_char_from_input_char())) // might only be needed for the teleport cheat menu
		decrease_input_char();
	scroll_updated = true; // Force redraw to clear previous input_char
}

uint8 MsgScroll::get_char_from_input_char()
{

	if(input_char > 27)
		return (input_char - 28 + SDLK_0);
	else if(input_char == 27)
		return SDLK_SPACE;
	else
		return (input_char + SDLK_a - 1);
}
