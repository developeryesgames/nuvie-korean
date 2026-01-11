
#include <stdlib.h>
#include <cmath>
#include <misc/U6misc.h>
#include <misc/SDLUtils.h>

#include "GUI_font.h"
#include "GUI_loadimage.h"

/* use default 8x8 font */
GUI_Font::GUI_Font(Uint8 fontType)
{
  SDL_Surface *temp;

  w_data = NULL;

  if(fontType == GUI_FONT_6X8)
	  temp = GUI_Font6x8();
  else if(fontType == GUI_FONT_GUMP)
  {
	  temp = GUI_FontGump();
	  w_data = GUI_FontGumpWData();
  }
  else
	  temp = GUI_DefaultFont();


  fontStore=SDL_ConvertSurface(temp,temp->format,SDL_SWSURFACE);
  charh = fontStore->h/16;
  charw = fontStore->w/16;
  freefont=1;
  SetTransparency(1);
}

/* open named BMP file */
GUI_Font::GUI_Font(char *name)
{
  fontStore=SDL_LoadBMP(name);
  if (fontStore!=NULL)
  {
    charh = fontStore->h/16;
    charw = fontStore->w/16;
    freefont=1;
  }
  else
  {
    freefont=0;
    DEBUG(0,LEVEL_EMERGENCY,"Could not load font.\n");
    exit(1);
  }
  SetTransparency(1);
  w_data = NULL;
}

/* use given YxY surface */
GUI_Font::GUI_Font(SDL_Surface *bitmap)
{
  if (bitmap==NULL)
    fontStore=GUI_DefaultFont();
  else
    fontStore=bitmap;
  charh = fontStore->h/16;
  charw = fontStore->w/16;
  freefont=0;
  SetTransparency(1);
  w_data = NULL;
}

/* copy constructor */
GUI_Font::GUI_Font(GUI_Font& font)
{
  SDL_Surface *temp=font.fontStore;
  fontStore=SDL_ConvertSurface(temp,temp->format,SDL_SWSURFACE);
  charh = fontStore->h/16;
  charw = fontStore->w/16;
  freefont=1;
  SetTransparency(1);
  w_data = NULL;
}

GUI_Font::~GUI_Font()
{
  if (freefont)
    SDL_FreeSurface(fontStore);
}

/* determine drawing style */
void GUI_Font::SetTransparency(int on)
{
  if ( (transparent=on) )  // single "=" is correct
    SDL_SetColorKey(fontStore,SDL_TRUE,0);
  else
    SDL_SetColorKey(fontStore,0,0);
}

/* determine foreground and background color values RGB*/
void GUI_Font::SetColoring(Uint8 fr, Uint8 fg, Uint8 fb, Uint8 br, Uint8 bg, Uint8 bb)
{
  SDL_Color colors[3]={{br,bg,bb,0},{fr,fg,fb,0}};
  SDL_SetColors(fontStore,colors,0,2);
}

void GUI_Font::SetColoring(Uint8 fr, Uint8 fg, Uint8 fb, Uint8 fr1, Uint8 fg1, Uint8 fb1, Uint8 br, Uint8 bg, Uint8 bb)
{
  SDL_Color colors[4]={{br,bg,bb,0},{fr,fg,fb,0},{fr1,fg1,fb1,0}};
  SDL_SetColors(fontStore,colors,0,3);
}

/* put the text onto the given surface using the preset mode and colors */
void GUI_Font::TextOut(SDL_Surface* context,int x, int y, const char* text, int line_wrap)
{
  int i;
  int j;
  Uint8 ch;
  SDL_Rect src;
  SDL_Rect dst;

  src.w = charw;
  src.h = charh-1;
  dst.w = charw;
  dst.h = charh-1;
  i=0;
  j=0;
  while ( (ch=text[i]) )  // single "=" is correct!
  {
    if(line_wrap && j == line_wrap)
       {
        j=0;
        y += charh;
       }

    src.x = (ch%16)*charw;
    src.y = (ch/16)*charh;
    if(w_data)
    {
    	dst.x = x;
    	dst.w = w_data[ch];
    	x += dst.w;
    }
    else
    	dst.x = x+(j*charw);
    dst.y = y;
    SDL_BlitSurface(fontStore, &src, context, &dst);
    i++;
    j++;
  }
}

void GUI_Font:: TextExtent(const char *text, int *w, int *h, int line_wrap)
{
 int len = strlen(text);
 if(w_data) //variable width font.
 {
	 //FIXME we're not handling line_wrap properly for variable width fonts!
	 *w = 0;
	 for(int i=0;i < len;i++)
	 {
		 *w += w_data[text[i]];
	 }
 }
 else
 {
	 if(line_wrap && len > line_wrap)
		 *w = line_wrap * charw;
	 else
		 *w=len * charw;
 }

 if(line_wrap && len > line_wrap)
 {
	 *h = (int)ceil((float)len / (float)line_wrap);
	 *h *= charh-1;
 }
 else
	 *h=charh-1;
 return;
}

uint16 GUI_Font::get_center(const char *text, uint16 width)
{
	int w,h;
	TextExtent(text, &w, &h);
	if(w < width)
		return ((width - w) / 2);
	else
		return 0;
}

/* put the text onto the given surface with scaling */
void GUI_Font::TextOutScaled(SDL_Surface* context, int x, int y, const char* text, int scale)
{
  if (scale <= 1) {
    TextOut(context, x, y, text);
    return;
  }

  int i = 0;
  int j = 0;
  Uint8 ch;

  int scaled_charw = charw * scale;
  int scaled_charh = (charh - 1) * scale;

  // Lock fontStore to read pixels directly
  SDL_LockSurface(fontStore);
  SDL_LockSurface(context);

  while ((ch = text[i]))
  {
    int src_x = (ch % 16) * charw;
    int src_y = (ch / 16) * charh;

    int dest_base_x;
    if (w_data) {
      dest_base_x = x;
      x += w_data[ch] * scale;
    } else {
      dest_base_x = x + (j * scaled_charw);
    }
    int dest_base_y = y;

    // For each pixel in the source character
    for (int py = 0; py < charh - 1; py++) {
      for (int px = 0; px < charw; px++) {
        // Read source pixel from fontStore (8-bit paletted)
        Uint8 *src_ptr = (Uint8*)fontStore->pixels + (src_y + py) * fontStore->pitch + (src_x + px);
        Uint8 palette_idx = *src_ptr;

        if (palette_idx == 0) continue; // Skip transparent pixels

        // Get color from palette
        SDL_Color col = fontStore->format->palette->colors[palette_idx];
        Uint32 pixel = SDL_MapRGB(context->format, col.r, col.g, col.b);

        // Draw scaled pixel to context
        for (int sy = 0; sy < scale; sy++) {
          for (int sx = 0; sx < scale; sx++) {
            int dest_x = dest_base_x + px * scale + sx;
            int dest_y = dest_base_y + py * scale + sy;

            // Bounds check
            if (dest_x < 0 || dest_x >= context->w || dest_y < 0 || dest_y >= context->h)
              continue;

            if (context->format->BytesPerPixel == 2) {
              Uint16 *dest_ptr = (Uint16*)((Uint8*)context->pixels + dest_y * context->pitch + dest_x * 2);
              *dest_ptr = (Uint16)pixel;
            } else if (context->format->BytesPerPixel == 4) {
              Uint32 *dest_ptr = (Uint32*)((Uint8*)context->pixels + dest_y * context->pitch + dest_x * 4);
              *dest_ptr = pixel;
            }
          }
        }
      }
    }

    i++;
    j++;
  }

  SDL_UnlockSurface(context);
  SDL_UnlockSurface(fontStore);
}
