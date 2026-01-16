/*
 *  WorldMapDialog.cpp
 *  Nuvie
 *
 *  World map viewer dialog with zoom and scroll support
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

#include "SDL.h"
#include "nuvieDefs.h"
#include "U6misc.h"
#include "Configuration.h"

#include "GUI.h"
#include "GUI_types.h"
#include "GUI_button.h"
#include "GUI_CallBack.h"
#include "GUI_TextInput.h"

#include "GUI_Dialog.h"
#include "GUI_ScrollBar.h"
#include "WorldMapDialog.h"
#include "Keys.h"
#include "Event.h"
#include "Game.h"
#include "Player.h"
#include "Screen.h"
#include "FontManager.h"
#include "KoreanFont.h"
#include "KoreanTranslation.h"

// Wider dialog to include memo list panel
#define WMD_WIDTH 420
#define WMD_HEIGHT 200

// Static variables to preserve view state between dialog openings
static bool s_has_saved_state = false;
static int s_saved_view_x = 0;
static int s_saved_view_y = 0;
static float s_saved_zoom_scale = 1.0f;

static int get_menu_scale() {
    FontManager *fm = Game::get_game()->get_font_manager();
    if (fm && fm->is_korean_enabled())
        return 3;  // 3x scale for Korean mode
    return 1;
}

WorldMapDialog::WorldMapDialog()
    : GUI_Dialog(Game::get_game()->get_game_x_offset() + (Game::get_game()->get_game_width() - WMD_WIDTH * get_menu_scale())/2,
                 Game::get_game()->get_game_y_offset() + (Game::get_game()->get_game_height() - WMD_HEIGHT * get_menu_scale())/2,
                 WMD_WIDTH * get_menu_scale(), WMD_HEIGHT * get_menu_scale(), 40, 60, 100, GUI_DIALOG_UNMOVABLE)
{
    worldmap_surface = NULL;
    close_button = NULL;
    zoom_in_button = NULL;
    zoom_out_button = NULL;
    add_memo_button = NULL;
    delete_memo_button = NULL;
    color_button = NULL;

    view_x = 0;
    view_y = 0;
    zoom_scale = 1.0f;
    map_width = 0;
    map_height = 0;

    // Map view area (left side, leaving room for memo list on right)
    // These are in BASE coordinates (before scale multiplication)
    map_view_x = 10;
    map_view_y = 26;
    map_view_width = 220;  // Narrower to make room for memo list
    map_view_height = WMD_HEIGHT - 34;

    // Memo list area (right side)
    memo_list_x = 240;
    memo_list_y = 26;
    memo_list_width = 170;
    memo_list_height = WMD_HEIGHT - 36;  // Full height (no bottom input)
    memo_scroll_offset = 0;
    memo_item_height = 16;  // Height per memo item (matches 32px Korean font at 2x scale)
    memo_visible_count = memo_list_height / memo_item_height;

    player_x = 0;
    player_y = 0;
    is_underground = false;

    dragging = false;
    drag_start_x = 0;
    drag_start_y = 0;
    drag_view_start_x = 0;
    drag_view_start_y = 0;

    anim_start_time = SDL_GetTicks();

    selected_marker = -1;
    editing_memo = false;

    last_click_time = 0;
    last_click_marker = -1;

    memo_scrollbar = NULL;

    loadMarkers();
}

WorldMapDialog::~WorldMapDialog()
{
    // Save view state for next opening
    s_saved_view_x = view_x;
    s_saved_view_y = view_y;
    s_saved_zoom_scale = zoom_scale;
    s_has_saved_state = true;

    saveMarkers();
    if(worldmap_surface)
        SDL_FreeSurface(worldmap_surface);
}

bool WorldMapDialog::loadWorldMap()
{
    // Load worldmap_4x.bmp from data/images folder
    Configuration *config = Game::get_game()->get_config();
    std::string datadir;
    config->value("config/datadir", datadir, "./data");

    std::string images_path, full_path;
    build_path(datadir, "images", images_path);
    build_path(images_path, "worldmap_4x.bmp", full_path);

    SDL_Surface *loaded = SDL_LoadBMP(full_path.c_str());
    if(loaded != NULL)
    {
        // Convert to 32-bit ARGB format for consistent handling
        worldmap_surface = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(loaded);

        if(worldmap_surface)
        {
            map_width = worldmap_surface->w;
            map_height = worldmap_surface->h;
            DEBUG(0, LEVEL_INFORMATIONAL, "Loaded world map: %s (%dx%d)\n", full_path.c_str(), map_width, map_height);
            return true;
        }
    }

    DEBUG(0, LEVEL_ERROR, "Failed to load world map image: %s\n", full_path.c_str());
    return false;
}

uint32 WorldMapDialog::getMarkerColor(int color_index)
{
    switch(color_index % MARKER_NUM_COLORS)
    {
        case MARKER_COLOR_RED:    return SDL_MapRGB(surface->format, 255, 50, 50);
        case MARKER_COLOR_BLUE:   return SDL_MapRGB(surface->format, 50, 100, 255);
        case MARKER_COLOR_GREEN:  return SDL_MapRGB(surface->format, 50, 200, 50);
        case MARKER_COLOR_ORANGE: return SDL_MapRGB(surface->format, 255, 150, 50);
        case MARKER_COLOR_PURPLE: return SDL_MapRGB(surface->format, 180, 80, 220);
        case MARKER_COLOR_CYAN:   return SDL_MapRGB(surface->format, 50, 200, 200);
        case MARKER_COLOR_YELLOW: return SDL_MapRGB(surface->format, 230, 230, 50);
        case MARKER_COLOR_WHITE:  return SDL_MapRGB(surface->format, 240, 240, 240);
        default:                  return SDL_MapRGB(surface->format, 255, 50, 50);
    }
}

bool WorldMapDialog::init()
{
    int scale = get_menu_scale();
    GUI *gui = GUI::get_gui();
    GUI_Font *font = gui->get_font();

    // Load world map image
    if(!loadWorldMap())
    {
        // The dialog will still work, just show empty area
    }

    // Get player position and check if underground
    Player *player = Game::get_game()->get_player();
    if(player)
    {
        player_x = player->get_actor()->get_x();
        player_y = player->get_actor()->get_y();
        is_underground = (player->get_actor()->get_z() != 0);
        DEBUG(0, LEVEL_INFORMATIONAL, "WorldMapDialog: player position = (%d, %d), underground = %d\n", player_x, player_y, is_underground ? 1 : 0);
    }

    // Get translated button texts for Korean mode
    FontManager *fm = Game::get_game()->get_font_manager();
    bool korean_mode = fm && fm->is_korean_enabled() && fm->get_korean_font();

    std::string close_text = korean_mode ? "닫기" : "X";
    std::string add_text = korean_mode ? "추가" : "+";
    std::string del_text = korean_mode ? "삭제" : "-";
    std::string color_text = korean_mode ? "색상" : "C";
    std::string zoomin_text = korean_mode ? "확대" : "+";
    std::string zoomout_text = korean_mode ? "축소" : "-";

    // Create buttons at top of dialog
    int button_y = 4 * scale;
    int btn_w = korean_mode ? 36*scale : 20*scale;  // Wider buttons for Korean text
    int btn_spacing = korean_mode ? 40 : 24;

    zoom_out_button = new GUI_Button(this, 8*scale, button_y, btn_w, 16*scale, zoomout_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
    if (scale > 1) { zoom_out_button->SetTextScale(scale); zoom_out_button->ChangeTextButton(-1,-1,-1,-1,zoomout_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
    AddWidget(zoom_out_button);

    zoom_in_button = new GUI_Button(this, (8 + btn_spacing)*scale, button_y, btn_w, 16*scale, zoomin_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
    if (scale > 1) { zoom_in_button->SetTextScale(scale); zoom_in_button->ChangeTextButton(-1,-1,-1,-1,zoomin_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
    AddWidget(zoom_in_button);

    close_button = new GUI_Button(this, (WMD_WIDTH - (korean_mode ? 46 : 28))*scale, button_y, btn_w, 16*scale, close_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
    if (scale > 1) { close_button->SetTextScale(scale); close_button->ChangeTextButton(-1,-1,-1,-1,close_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
    AddWidget(close_button);

    // Memo list buttons (at top of memo panel)
    add_memo_button = new GUI_Button(this, memo_list_x*scale, button_y, btn_w, 16*scale, add_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
    if (scale > 1) { add_memo_button->SetTextScale(scale); add_memo_button->ChangeTextButton(-1,-1,-1,-1,add_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
    AddWidget(add_memo_button);

    // Note: Add button is always visible - can add markers even when underground
    // (markers are just map coordinates, useful for "entrance above here" notes)

    delete_memo_button = new GUI_Button(this, (memo_list_x + btn_spacing)*scale, button_y, btn_w, 16*scale, del_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
    if (scale > 1) { delete_memo_button->SetTextScale(scale); delete_memo_button->ChangeTextButton(-1,-1,-1,-1,del_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
    AddWidget(delete_memo_button);

    color_button = new GUI_Button(this, (memo_list_x + btn_spacing*2)*scale, button_y, btn_w, 16*scale, color_text.c_str(), font, BUTTON_TEXTALIGN_CENTER, 0, this, 0);
    if (scale > 1) { color_button->SetTextScale(scale); color_button->ChangeTextButton(-1,-1,-1,-1,color_text.c_str(),BUTTON_TEXTALIGN_CENTER); }
    AddWidget(color_button);

    // Create scrollbar for memo list (on the right side of memo list)
    // Note: SCROLLBAR_WIDTH is in actual pixels (not scaled), so apply scale first then subtract
    int scrollbar_x = (memo_list_x + memo_list_width) * scale - SCROLLBAR_WIDTH;
    int scrollbar_y = memo_list_y * scale;
    int scrollbar_h = memo_list_height * scale;
    memo_scrollbar = new GUI_ScrollBar(scrollbar_x, scrollbar_y, scrollbar_h, this);
    AddWidget(memo_scrollbar);
    updateScrollbar();

    // No bottom input field - editing is done inline in the list

    // Restore saved state or calculate initial zoom to fit entire map
    if(s_has_saved_state)
    {
        view_x = s_saved_view_x;
        view_y = s_saved_view_y;
        zoom_scale = s_saved_zoom_scale;
        clampViewPosition();
    }
    else if(map_width > 0 && map_height > 0)
    {
        int view_w = map_view_width * scale;
        int view_h = map_view_height * scale;

        float scale_x = (float)view_w / map_width;
        float scale_y = (float)view_h / map_height;
        zoom_scale = (scale_x < scale_y) ? scale_x : scale_y;

        view_x = 0;
        view_y = 0;
    }

    return true;
}

void WorldMapDialog::centerOnPlayer()
{
    if(map_width == 0 || map_height == 0)
        return;

    int scale = get_menu_scale();
    int view_w = map_view_width * scale;
    int view_h = map_view_height * scale;

    int map_scale = map_width / 1024;
    int player_map_x = player_x * map_scale;
    int player_map_y = player_y * map_scale;

    int src_view_width = (int)(view_w / zoom_scale);
    int src_view_height = (int)(view_h / zoom_scale);

    view_x = player_map_x - src_view_width / 2;
    view_y = player_map_y - src_view_height / 2;

    clampViewPosition();
}

void WorldMapDialog::clampViewPosition()
{
    if(map_width == 0 || map_height == 0)
        return;

    int scale = get_menu_scale();
    int view_w = map_view_width * scale;
    int view_h = map_view_height * scale;

    int src_view_width = (int)(view_w / zoom_scale);
    int src_view_height = (int)(view_h / zoom_scale);

    if(src_view_width >= map_width)
        view_x = (map_width - src_view_width) / 2;
    else
    {
        if(view_x < 0) view_x = 0;
        if(view_x + src_view_width > map_width) view_x = map_width - src_view_width;
    }

    if(src_view_height >= map_height)
        view_y = (map_height - src_view_height) / 2;
    else
    {
        if(view_y < 0) view_y = 0;
        if(view_y + src_view_height > map_height) view_y = map_height - src_view_height;
    }
}

void WorldMapDialog::drawMapView()
{
    if(worldmap_surface == NULL || map_width == 0)
        return;

    int scale = get_menu_scale();
    int scaled_view_x = area.x + map_view_x * scale;
    int scaled_view_y = area.y + map_view_y * scale;
    int scaled_view_width = map_view_width * scale;
    int scaled_view_height = map_view_height * scale;

    int src_view_width = (int)(scaled_view_width / zoom_scale);
    int src_view_height = (int)(scaled_view_height / zoom_scale);

    SDL_Rect src_rect;
    src_rect.x = view_x;
    src_rect.y = view_y;
    src_rect.w = src_view_width;
    src_rect.h = src_view_height;

    if(src_rect.x < 0) { src_rect.w += src_rect.x; src_rect.x = 0; }
    if(src_rect.y < 0) { src_rect.h += src_rect.y; src_rect.y = 0; }
    if(src_rect.x + src_rect.w > map_width) src_rect.w = map_width - src_rect.x;
    if(src_rect.y + src_rect.h > map_height) src_rect.h = map_height - src_rect.y;

    SDL_Rect dst_rect;
    dst_rect.x = scaled_view_x;
    dst_rect.y = scaled_view_y;
    dst_rect.w = scaled_view_width;
    dst_rect.h = scaled_view_height;

    SDL_FillRect(surface, &dst_rect, SDL_MapRGB(surface->format, 40, 60, 100));

    if(src_rect.w > 0 && src_rect.h > 0)
    {
        dst_rect.w = (int)(src_rect.w * zoom_scale);
        dst_rect.h = (int)(src_rect.h * zoom_scale);

        if(dst_rect.w < scaled_view_width)
            dst_rect.x = scaled_view_x + (scaled_view_width - dst_rect.w) / 2;
        if(dst_rect.h < scaled_view_height)
            dst_rect.y = scaled_view_y + (scaled_view_height - dst_rect.h) / 2;

        SDL_BlitScaled(worldmap_surface, &src_rect, surface, &dst_rect);

        // Darken map when underground (50% opacity overlay)
        if(is_underground)
        {
            SDL_Rect dark_rect = dst_rect;
            // Create semi-transparent dark overlay effect by drawing dark pixels
            Uint32 dark_color = SDL_MapRGB(surface->format, 0, 0, 30);
            // Use SDL blend mode for darkening
            SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
            for(int y = dark_rect.y; y < dark_rect.y + dark_rect.h; y += 2)
            {
                for(int x = dark_rect.x; x < dark_rect.x + dark_rect.w; x += 2)
                {
                    SDL_Rect pixel = {x, y, 1, 1};
                    SDL_FillRect(surface, &pixel, dark_color);
                }
            }
        }
    }
}

void WorldMapDialog::drawPlayerMarker()
{
    // Don't show player marker when underground
    if(is_underground)
        return;

    if(map_width == 0 || map_height == 0)
        return;

    int scale = get_menu_scale();
    int scaled_view_x = area.x + map_view_x * scale;
    int scaled_view_y = area.y + map_view_y * scale;
    int scaled_view_width = map_view_width * scale;
    int scaled_view_height = map_view_height * scale;

    int map_img_scale = map_width / 1024;
    int player_map_x = player_x * map_img_scale;
    int player_map_y = player_y * map_img_scale;

    int src_view_width = (int)(scaled_view_width / zoom_scale);
    int src_view_height = (int)(scaled_view_height / zoom_scale);

    int src_x = view_x;
    int src_y = view_y;
    int src_w = src_view_width;
    int src_h = src_view_height;

    if(src_x < 0) { src_w += src_x; src_x = 0; }
    if(src_y < 0) { src_h += src_y; src_y = 0; }
    if(src_x + src_w > map_width) src_w = map_width - src_x;
    if(src_y + src_h > map_height) src_h = map_height - src_y;

    if(src_w <= 0 || src_h <= 0)
        return;

    if(player_map_x < src_x || player_map_x >= src_x + src_w ||
       player_map_y < src_y || player_map_y >= src_y + src_h)
        return;

    int dst_w = (int)(src_w * zoom_scale);
    int dst_h = (int)(src_h * zoom_scale);
    int dst_x = scaled_view_x;
    int dst_y = scaled_view_y;

    if(dst_w < scaled_view_width)
        dst_x = scaled_view_x + (scaled_view_width - dst_w) / 2;
    if(dst_h < scaled_view_height)
        dst_y = scaled_view_y + (scaled_view_height - dst_h) / 2;

    int screen_x = dst_x + (int)((player_map_x - src_x) * zoom_scale);
    int screen_y = dst_y + (int)((player_map_y - src_y) * zoom_scale);

    uint32 elapsed = SDL_GetTicks() - anim_start_time;
    float pulse = sinf((float)elapsed * 0.006f);
    float pulse_normalized = (pulse + 1.0f) * 0.5f;

    int base_radius = 5 + scale;
    int radius = base_radius + (int)(3.0f * pulse_normalized);

    uint32 outline_color = SDL_MapRGB(surface->format, 0, 0, 0);
    uint32 marker_color = SDL_MapRGB(surface->format, 255, 255, 0);

    int outer_radius = radius + 2;

    for(int dy = -outer_radius; dy <= outer_radius; dy++)
    {
        int outer_dx = (int)sqrtf((float)(outer_radius * outer_radius - dy * dy));
        SDL_Rect line_rect;
        line_rect.y = screen_y + dy;
        line_rect.h = 1;
        line_rect.x = screen_x - outer_dx;
        line_rect.w = outer_dx * 2 + 1;
        SDL_FillRect(surface, &line_rect, outline_color);
    }

    for(int dy = -radius; dy <= radius; dy++)
    {
        int inner_dx = (int)sqrtf((float)(radius * radius - dy * dy));
        SDL_Rect line_rect;
        line_rect.y = screen_y + dy;
        line_rect.h = 1;
        line_rect.x = screen_x - inner_dx;
        line_rect.w = inner_dx * 2 + 1;
        SDL_FillRect(surface, &line_rect, marker_color);
    }
}

void WorldMapDialog::drawMemoList()
{
    int scale = get_menu_scale();
    int memo_scale = (scale > 1) ? 2 : 1;  // Use 2x scale for memo items (more compact)
    int list_x = area.x + memo_list_x * scale;
    int list_y = area.y + memo_list_y * scale;
    int list_w = memo_list_width * scale - SCROLLBAR_WIDTH;  // Exclude scrollbar area
    int list_h = memo_list_height * scale;

    // Draw list background (excluding scrollbar area)
    SDL_Rect bg_rect = { list_x, list_y, list_w, list_h };
    SDL_FillRect(surface, &bg_rect, SDL_MapRGB(surface->format, 30, 45, 80));

    // Draw border
    SDL_Rect border_rect;
    border_rect.x = list_x - 1;
    border_rect.y = list_y - 1;
    border_rect.w = list_w + 2;
    border_rect.h = 1;
    SDL_FillRect(surface, &border_rect, SDL_MapRGB(surface->format, 80, 100, 140));
    border_rect.y = list_y + list_h;
    SDL_FillRect(surface, &border_rect, SDL_MapRGB(surface->format, 80, 100, 140));
    border_rect.y = list_y;
    border_rect.w = 1;
    border_rect.h = list_h;
    SDL_FillRect(surface, &border_rect, SDL_MapRGB(surface->format, 80, 100, 140));
    border_rect.x = list_x + list_w;
    SDL_FillRect(surface, &border_rect, SDL_MapRGB(surface->format, 80, 100, 140));

    // Draw memo items (using memo_scale for more compact display)
    GUI *gui = GUI::get_gui();
    GUI_Font *font = gui->get_font();
    FontManager *fm = Game::get_game()->get_font_manager();
    KoreanFont *korean_font = (fm && fm->is_korean_enabled()) ? fm->get_korean_font() : NULL;
    Screen *scr = Game::get_game()->get_screen();
    int item_h = memo_item_height * memo_scale;  // Use memo_scale for item height
    int visible_count = list_h / item_h;

    for(int i = 0; i < visible_count && (i + memo_scroll_offset) < (int)markers.size(); i++)
    {
        int idx = i + memo_scroll_offset;
        const MapMarker &m = markers[idx];

        int item_y = list_y + i * item_h;

        // Highlight selected item
        if(idx == selected_marker)
        {
            SDL_Rect sel_rect = { list_x, item_y, list_w, item_h };
            SDL_FillRect(surface, &sel_rect, SDL_MapRGB(surface->format, 60, 80, 120));
        }

        // Draw color indicator (small square) - use memo_scale
        int color_size = 6 * memo_scale;
        SDL_Rect color_rect = { list_x + 2*memo_scale, item_y + (item_h - color_size)/2, color_size, color_size };
        SDL_FillRect(surface, &color_rect, getMarkerColor(m.color_index));

        // Draw number
        char num_str[8];
        snprintf(num_str, sizeof(num_str), "%d.", idx + 1);

        int text_x = list_x + 4*memo_scale + color_size;
        int text_y = item_y + 1*memo_scale;

        // Draw memo text - show editing_text if currently editing this item
        std::string display_text;
        bool is_editing_this = (editing_memo && idx == selected_marker);

        if(is_editing_this)
        {
            display_text = editing_text;  // Will add cursor/composing separately
        }
        else
        {
            display_text = m.text;
            if(display_text.empty())
            {
                char coord_str[32];
                snprintf(coord_str, sizeof(coord_str), "(%d,%d)", m.x, m.y);
                display_text = coord_str;
            }
        }

        // Combine number and display text
        std::string full_text = std::string(num_str) + " " + display_text;

        // Draw text using Korean font if available
        if(korean_font)
        {
            uint16 drawn_width = korean_font->drawStringUTF8(scr, full_text.c_str(), text_x, text_y, 0x0f, 0x0f, 1);

            // If editing this item, draw composing text with underline and cursor
            if(is_editing_this)
            {
                int cursor_x = text_x + drawn_width;

                // Draw composing text (IME preview) with underline
                if(!composing_text.empty())
                {
                    uint16 comp_width = korean_font->drawStringUTF8(scr, composing_text.c_str(), cursor_x, text_y, 0x0f, 0x0f, 1);

                    // Draw underline under composing text
                    SDL_Rect underline;
                    underline.x = cursor_x;
                    underline.y = text_y + korean_font->getCharHeight() - 2;
                    underline.w = comp_width;
                    underline.h = 1;
                    SDL_FillRect(surface, &underline, SDL_MapRGB(surface->format, 255, 100, 100));

                    cursor_x += comp_width;
                }

                // Draw cursor (red vertical bar)
                SDL_Rect cursor_rect;
                cursor_rect.x = cursor_x;
                cursor_rect.y = text_y;
                cursor_rect.w = 2;
                cursor_rect.h = korean_font->getCharHeight();
                SDL_FillRect(surface, &cursor_rect, SDL_MapRGB(surface->format, 255, 0, 0));
            }
        }
        else
        {
            if(scale > 1)
                font->TextOutScaled(surface, text_x, text_y, full_text.c_str(), scale);
            else
                font->TextOut(surface, text_x, text_y, full_text.c_str());

            // Draw cursor for non-Korean mode
            if(is_editing_this)
            {
                int cursor_x = text_x + (int)full_text.length() * font->CharWidth() * scale;
                SDL_Rect cursor_rect;
                cursor_rect.x = cursor_x;
                cursor_rect.y = text_y;
                cursor_rect.w = 2;
                cursor_rect.h = font->CharHeight() * scale;
                SDL_FillRect(surface, &cursor_rect, SDL_MapRGB(surface->format, 255, 0, 0));
            }
        }
    }

    // Scrollbar widget handles scroll indicators
}

void WorldMapDialog::Display(bool full_redraw)
{
    GUI_Dialog::Display(full_redraw);

    drawMapView();
    drawMarkers();
    drawPlayerMarker();
    drawMemoList();

    screen->update(area.x, area.y, area.w, area.h);
}

void WorldMapDialog::zoomIn()
{
    if(zoom_scale < 2.0f)
    {
        int scale = get_menu_scale();
        int view_w = map_view_width * scale;
        int view_h = map_view_height * scale;

        int src_view_width = (int)(view_w / zoom_scale);
        int src_view_height = (int)(view_h / zoom_scale);
        int center_x = view_x + src_view_width / 2;
        int center_y = view_y + src_view_height / 2;

        zoom_scale *= 1.5f;
        if(zoom_scale > 2.0f) zoom_scale = 2.0f;

        src_view_width = (int)(view_w / zoom_scale);
        src_view_height = (int)(view_h / zoom_scale);
        view_x = center_x - src_view_width / 2;
        view_y = center_y - src_view_height / 2;

        clampViewPosition();
    }
}

void WorldMapDialog::zoomOut()
{
    int scale = get_menu_scale();
    int view_w = map_view_width * scale;
    int view_h = map_view_height * scale;

    float min_scale_x = (float)view_w / map_width;
    float min_scale_y = (float)view_h / map_height;
    float min_zoom = (min_scale_x < min_scale_y) ? min_scale_x : min_scale_y;

    if(zoom_scale > min_zoom)
    {
        int src_view_width = (int)(view_w / zoom_scale);
        int src_view_height = (int)(view_h / zoom_scale);
        int center_x = view_x + src_view_width / 2;
        int center_y = view_y + src_view_height / 2;

        zoom_scale /= 1.5f;
        if(zoom_scale < min_zoom) zoom_scale = min_zoom;

        src_view_width = (int)(view_w / zoom_scale);
        src_view_height = (int)(view_h / zoom_scale);
        view_x = center_x - src_view_width / 2;
        view_y = center_y - src_view_height / 2;

        clampViewPosition();
    }
}

void WorldMapDialog::scrollMap(int dx, int dy)
{
    view_x += dx;
    view_y += dy;
    clampViewPosition();
}

void WorldMapDialog::zoomAtPoint(int screen_x, int screen_y, bool zoom_in)
{
    int scale = get_menu_scale();
    int view_w = map_view_width * scale;
    int view_h = map_view_height * scale;
    int scaled_view_x = area.x + map_view_x * scale;
    int scaled_view_y = area.y + map_view_y * scale;

    int src_view_width = (int)(view_w / zoom_scale);
    int src_view_height = (int)(view_h / zoom_scale);

    float rel_x = (float)(screen_x - scaled_view_x) / view_w;
    float rel_y = (float)(screen_y - scaled_view_y) / view_h;

    int map_point_x = view_x + (int)(src_view_width * rel_x);
    int map_point_y = view_y + (int)(src_view_height * rel_y);

    float min_scale_x = (float)view_w / map_width;
    float min_scale_y = (float)view_h / map_height;
    float min_zoom = (min_scale_x < min_scale_y) ? min_scale_x : min_scale_y;
    float max_zoom = 2.0f;

    float old_zoom = zoom_scale;

    if(zoom_in)
    {
        if(zoom_scale < max_zoom)
        {
            zoom_scale *= 1.5f;
            if(zoom_scale > max_zoom) zoom_scale = max_zoom;
        }
    }
    else
    {
        if(zoom_scale > min_zoom)
        {
            zoom_scale /= 1.5f;
            if(zoom_scale < min_zoom) zoom_scale = min_zoom;
        }
    }

    if(zoom_scale == old_zoom)
        return;

    src_view_width = (int)(view_w / zoom_scale);
    src_view_height = (int)(view_h / zoom_scale);

    view_x = map_point_x - (int)(src_view_width * rel_x);
    view_y = map_point_y - (int)(src_view_height * rel_y);

    clampViewPosition();
}

GUI_status WorldMapDialog::close_dialog()
{
    Delete();
    return GUI_YUM;
}

void WorldMapDialog::selectMarker(int index)
{
    // If currently editing a different marker, finish editing first
    if(editing_memo && selected_marker != index)
    {
        finishEditingMemo();
    }

    if(index >= 0 && index < (int)markers.size())
    {
        selected_marker = index;

        // Center map on marker
        int scale = get_menu_scale();
        int view_w = map_view_width * scale;
        int view_h = map_view_height * scale;
        int map_scale = map_width / 1024;

        int marker_map_x = markers[index].x * map_scale;
        int marker_map_y = markers[index].y * map_scale;

        int src_view_width = (int)(view_w / zoom_scale);
        int src_view_height = (int)(view_h / zoom_scale);

        view_x = marker_map_x - src_view_width / 2;
        view_y = marker_map_y - src_view_height / 2;

        clampViewPosition();

        // Ensure selected marker is visible in list
        int visible_count = getMemoVisibleCount();
        if(index < memo_scroll_offset)
            memo_scroll_offset = index;
        else if(index >= memo_scroll_offset + visible_count)
            memo_scroll_offset = index - visible_count + 1;

        updateScrollbar();
    }
    else
    {
        selected_marker = -1;
    }
}

void WorldMapDialog::startEditingMemo()
{
    if(selected_marker >= 0 && selected_marker < (int)markers.size())
    {
        editing_memo = true;
        editing_text = markers[selected_marker].text;
        composing_text.clear();
        SDL_StartTextInput();
    }
}

void WorldMapDialog::finishEditingMemo()
{
    if(editing_memo && selected_marker >= 0 && selected_marker < (int)markers.size())
    {
        // Include any remaining composing text
        if(!composing_text.empty())
        {
            editing_text += composing_text;
            composing_text.clear();
        }
        markers[selected_marker].text = editing_text;
        editing_memo = false;
        SDL_StopTextInput();
    }
}

void WorldMapDialog::cycleMarkerColor()
{
    if(selected_marker >= 0 && selected_marker < (int)markers.size())
    {
        markers[selected_marker].color_index = (markers[selected_marker].color_index + 1) % MARKER_NUM_COLORS;
    }
}

int WorldMapDialog::getMemoVisibleCount()
{
    int scale = get_menu_scale();
    int memo_scale = (scale > 1) ? 2 : 1;  // Memo items use 2x scale for compact display
    int list_h = memo_list_height * scale;
    int item_h = memo_item_height * memo_scale;
    return list_h / item_h;
}

void WorldMapDialog::scrollMemoList(int delta)
{
    int visible_count = getMemoVisibleCount();
    int max_scroll = (int)markers.size() - visible_count;
    if(max_scroll < 0) max_scroll = 0;

    memo_scroll_offset += delta;
    if(memo_scroll_offset < 0) memo_scroll_offset = 0;
    if(memo_scroll_offset > max_scroll) memo_scroll_offset = max_scroll;

    updateScrollbar();
}

void WorldMapDialog::updateScrollbar()
{
    if(!memo_scrollbar)
        return;

    int visible_count = getMemoVisibleCount();
    int total_items = (int)markers.size();

    if(total_items <= visible_count)
    {
        // All items visible, full slider
        memo_scrollbar->set_slider_length(1.0f);
        memo_scrollbar->set_slider_position(0.0f);
    }
    else
    {
        // Calculate slider length as ratio of visible to total (same as GUI_Scroller)
        float slider_len = (float)visible_count / (float)total_items;
        memo_scrollbar->set_slider_length(slider_len);

        // Calculate slider position (same formula as GUI_Scroller: offset / total_items)
        float position = (float)memo_scroll_offset / (float)total_items;
        memo_scrollbar->set_slider_position(position);
    }
}

int WorldMapDialog::getMemoItemAtScreen(int screen_x, int screen_y)
{
    int scale = get_menu_scale();
    int memo_scale = (scale > 1) ? 2 : 1;  // Memo items use 2x scale
    int list_x = area.x + memo_list_x * scale;
    int list_y = area.y + memo_list_y * scale;
    int list_w = memo_list_width * scale - SCROLLBAR_WIDTH;  // Exclude scrollbar area
    int list_h = memo_list_height * scale;

    // Check if click is in memo list area (excluding scrollbar)
    if(screen_x < list_x || screen_x >= list_x + list_w ||
       screen_y < list_y || screen_y >= list_y + list_h)
        return -1;

    int item_h = memo_item_height * memo_scale;
    int item_idx = (screen_y - list_y) / item_h + memo_scroll_offset;

    if(item_idx >= 0 && item_idx < (int)markers.size())
        return item_idx;

    return -1;
}

GUI_status WorldMapDialog::KeyDown(SDL_Keysym key)
{
    // If editing memo, handle editing keys
    if(editing_memo)
    {
        if(key.sym == SDLK_RETURN || key.sym == SDLK_KP_ENTER)
        {
            // If composing text exists, just finalize it (IME will send TextInput)
            // Don't finish editing yet - wait for next Enter
            if(!composing_text.empty())
            {
                // The IME should handle this - just consume the key
                return GUI_YUM;
            }
            finishEditingMemo();
            return GUI_YUM;
        }
        else if(key.sym == SDLK_ESCAPE)
        {
            // Cancel editing - restore original text
            editing_memo = false;
            composing_text.clear();
            SDL_StopTextInput();
            return GUI_YUM;
        }
        else if(key.sym == SDLK_BACKSPACE)
        {
            // If composing, clear composing text first
            if(!composing_text.empty())
            {
                composing_text.clear();
            }
            else if(!editing_text.empty())
            {
                // Delete last UTF-8 character from editing_text
                size_t len = editing_text.length();
                size_t i = len - 1;
                while(i > 0 && (editing_text[i] & 0xC0) == 0x80)
                    i--;
                editing_text = editing_text.substr(0, i);
            }
            return GUI_YUM;
        }
        return GUI_YUM;  // Consume all keys while editing
    }

    int scroll_amount = (int)(32 / zoom_scale);
    if(scroll_amount < 8) scroll_amount = 8;

    switch(key.sym)
    {
        case SDLK_ESCAPE:
        case SDLK_w:
            return close_dialog();

        case SDLK_UP:
            // If marker is selected, move to previous marker
            if(selected_marker > 0)
            {
                selectMarker(selected_marker - 1);
            }
            else if(selected_marker < 0 && !markers.empty())
            {
                selectMarker((int)markers.size() - 1);
            }
            else
            {
                scrollMap(0, -scroll_amount);
            }
            break;
        case SDLK_DOWN:
            // If marker is selected, move to next marker
            if(selected_marker >= 0 && selected_marker < (int)markers.size() - 1)
            {
                selectMarker(selected_marker + 1);
            }
            else if(selected_marker < 0 && !markers.empty())
            {
                selectMarker(0);
            }
            else
            {
                scrollMap(0, scroll_amount);
            }
            break;
        case SDLK_LEFT:
            scrollMap(-scroll_amount, 0);
            break;
        case SDLK_RIGHT:
            scrollMap(scroll_amount, 0);
            break;

        case SDLK_PLUS:
        case SDLK_EQUALS:
        case SDLK_KP_PLUS:
            zoomIn();
            break;
        case SDLK_MINUS:
        case SDLK_KP_MINUS:
            zoomOut();
            break;

        case SDLK_HOME:
        case SDLK_c:
            centerOnPlayer();
            break;

        case SDLK_DELETE:
            if(selected_marker >= 0)
            {
                deleteMarker(selected_marker);
                selected_marker = -1;
            }
            break;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            startEditingMemo();
            break;

        default:
            break;
    }

    return GUI_YUM;
}

GUI_status WorldMapDialog::TextInput(const char *text)
{
    if(editing_memo && text)
    {
        // Clear composing text since we got finalized input
        composing_text.clear();
        editing_text += text;
        return GUI_YUM;
    }
    return GUI_PASS;
}

GUI_status WorldMapDialog::TextEditing(const char *text, int start, int length)
{
    if(!editing_memo)
        return GUI_PASS;

    // Store the composing text for display
    // This is the text being composed by the IME (e.g., Korean "ㄱ" before becoming "그")
    composing_text = (text != NULL) ? text : "";

    return GUI_YUM;
}

GUI_status WorldMapDialog::MouseDown(int x, int y, int button)
{
    int scale = get_menu_scale();

    // Check if click is in memo list
    int memo_idx = getMemoItemAtScreen(x, y);
    if(memo_idx >= 0)
    {
        if(button == SDL_BUTTON_LEFT)
        {
            uint32 current_time = SDL_GetTicks();

            // Check for double-click (same marker within 400ms)
            if(memo_idx == last_click_marker && (current_time - last_click_time) < 400)
            {
                // Double-click: start editing
                selectMarker(memo_idx);
                startEditingMemo();
                last_click_time = 0;
                last_click_marker = -1;
            }
            else
            {
                // Single click: select marker
                selectMarker(memo_idx);
                last_click_time = current_time;
                last_click_marker = memo_idx;
            }
            return GUI_YUM;
        }
    }

    // Check if click is in map view area
    int local_x = x - area.x;
    int local_y = y - area.y;

    int scaled_view_x = map_view_x * scale;
    int scaled_view_y = map_view_y * scale;
    int scaled_view_width = map_view_width * scale;
    int scaled_view_height = map_view_height * scale;

    if(local_x >= scaled_view_x && local_x < scaled_view_x + scaled_view_width &&
       local_y >= scaled_view_y && local_y < scaled_view_y + scaled_view_height)
    {
        if(button == SDL_BUTTON_LEFT)
        {
            // Check if clicking on a marker
            int marker_idx = getMarkerAtScreen(x, y);
            if(marker_idx >= 0)
            {
                selectMarker(marker_idx);
                return GUI_YUM;
            }

            dragging = true;
            drag_start_x = x;
            drag_start_y = y;
            drag_view_start_x = view_x;
            drag_view_start_y = view_y;
            return GUI_YUM;
        }
        else if(button == SDL_BUTTON_RIGHT)
        {
            // Right-click: add marker and start editing
            addMarkerAtScreen(x, y);
            selectMarker((int)markers.size() - 1);
            startEditingMemo();
            return GUI_YUM;
        }
    }

    return GUI_Dialog::MouseDown(x, y, button);
}

GUI_status WorldMapDialog::MouseUp(int x, int y, int button)
{
    if(button == SDL_BUTTON_LEFT && dragging)
    {
        dragging = false;
        return GUI_YUM;
    }

    return GUI_Dialog::MouseUp(x, y, button);
}

GUI_status WorldMapDialog::MouseMotion(int x, int y, Uint8 state)
{
    if(dragging)
    {
        int screen_dx = drag_start_x - x;
        int screen_dy = drag_start_y - y;

        int map_dx = (int)(screen_dx / zoom_scale);
        int map_dy = (int)(screen_dy / zoom_scale);

        view_x = drag_view_start_x + map_dx;
        view_y = drag_view_start_y + map_dy;

        clampViewPosition();
        return GUI_YUM;
    }

    return GUI_Dialog::MouseMotion(x, y, state);
}

GUI_status WorldMapDialog::HandleEvent(const SDL_Event *event)
{
    // Handle mouse wheel BEFORE passing to children (scrollbar)
    // This ensures map/memo list wheel behavior works correctly
    if(event->type == SDL_MOUSEWHEEL)
    {
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        int scale = get_menu_scale();

        // Check if over memo list area (including scrollbar)
        int list_x = area.x + memo_list_x * scale;
        int list_y = area.y + memo_list_y * scale;
        int list_w = memo_list_width * scale;
        int list_h = memo_list_height * scale;

        if(mouse_x >= list_x && mouse_x < list_x + list_w &&
           mouse_y >= list_y && mouse_y < list_y + list_h)
        {
            // Scroll memo list
            if(event->wheel.y > 0)
                scrollMemoList(-1);
            else if(event->wheel.y < 0)
                scrollMemoList(1);
            return GUI_YUM;
        }

        // Check if over map view area
        int scaled_view_x = area.x + map_view_x * scale;
        int scaled_view_y = area.y + map_view_y * scale;
        int scaled_view_width = map_view_width * scale;
        int scaled_view_height = map_view_height * scale;

        if(mouse_x >= scaled_view_x && mouse_x < scaled_view_x + scaled_view_width &&
           mouse_y >= scaled_view_y && mouse_y < scaled_view_y + scaled_view_height)
        {
            // Zoom map at mouse position
            if(event->wheel.y > 0)
                zoomAtPoint(mouse_x, mouse_y, true);
            else if(event->wheel.y < 0)
                zoomAtPoint(mouse_x, mouse_y, false);
            return GUI_YUM;
        }

        // Default: zoom map (not at specific point)
        if(event->wheel.y > 0)
            zoomIn();
        else if(event->wheel.y < 0)
            zoomOut();
        return GUI_YUM;
    }

    // For all other events, use default handling (children first)
    return GUI_Dialog::HandleEvent(event);
}

GUI_status WorldMapDialog::MouseWheel(sint32 x, sint32 y)
{
    // This is now handled in HandleEvent, but keep for compatibility
    return GUI_YUM;
}

GUI_status WorldMapDialog::callback(uint16 msg, GUI_CallBack *caller, void *data)
{
    if(caller == close_button)
    {
        return close_dialog();
    }
    else if(caller == zoom_in_button)
    {
        zoomIn();
    }
    else if(caller == zoom_out_button)
    {
        zoomOut();
    }
    else if(caller == add_memo_button)
    {
        // Add marker at center of current view
        int scale = get_menu_scale();
        int view_w = map_view_width * scale;
        int view_h = map_view_height * scale;

        int src_view_width = (int)(view_w / zoom_scale);
        int src_view_height = (int)(view_h / zoom_scale);

        int center_map_x = view_x + src_view_width / 2;
        int center_map_y = view_y + src_view_height / 2;

        int map_img_scale = map_width / 1024;
        uint16 tile_x = (uint16)(center_map_x / map_img_scale);
        uint16 tile_y = (uint16)(center_map_y / map_img_scale);

        if(tile_x > 1023) tile_x = 1023;
        if(tile_y > 1023) tile_y = 1023;

        MapMarker m;
        m.x = tile_x;
        m.y = tile_y;
        m.text = "";
        m.color_index = markers.size() % MARKER_NUM_COLORS;
        markers.push_back(m);

        selectMarker((int)markers.size() - 1);
        startEditingMemo();
    }
    else if(caller == delete_memo_button)
    {
        if(selected_marker >= 0)
        {
            deleteMarker(selected_marker);
            selected_marker = -1;
        }
    }
    else if(caller == color_button)
    {
        cycleMarkerColor();
    }
    else if(caller == memo_scrollbar)
    {
        // Handle scrollbar messages
        if(msg == SCROLLBAR_CB_UP_BUTTON)
        {
            scrollMemoList(-1);
        }
        else if(msg == SCROLLBAR_CB_DOWN_BUTTON)
        {
            scrollMemoList(1);
        }
        else if(msg == SCROLLBAR_CB_PAGE_UP)
        {
            int visible_count = getMemoVisibleCount();
            scrollMemoList(-visible_count);
        }
        else if(msg == SCROLLBAR_CB_PAGE_DOWN)
        {
            int visible_count = getMemoVisibleCount();
            scrollMemoList(visible_count);
        }
        else if(msg == SCROLLBAR_CB_SLIDER_MOVED)
        {
            // Data contains the percentage (slider_y / track_length)
            // Use same formula as GUI_Scroller::move_percentage()
            float *percentage = (float *)data;
            if(percentage)
            {
                int visible_count = getMemoVisibleCount();
                int total_items = (int)markers.size();
                int max_scroll = total_items - visible_count;
                if(max_scroll > 0)
                {
                    // Same as GUI_Scroller: offset = total_items * percentage
                    memo_scroll_offset = (int)((float)total_items * (*percentage));
                    if(memo_scroll_offset < 0) memo_scroll_offset = 0;
                    if(memo_scroll_offset > max_scroll) memo_scroll_offset = max_scroll;
                }
            }
        }
    }

    return GUI_YUM;
}

bool WorldMapDialog::screenToTile(int screen_x, int screen_y, uint16 &tile_x, uint16 &tile_y)
{
    if(map_width == 0 || map_height == 0)
        return false;

    int scale = get_menu_scale();
    int scaled_view_x = area.x + map_view_x * scale;
    int scaled_view_y = area.y + map_view_y * scale;
    int scaled_view_width = map_view_width * scale;
    int scaled_view_height = map_view_height * scale;

    if(screen_x < scaled_view_x || screen_x >= scaled_view_x + scaled_view_width ||
       screen_y < scaled_view_y || screen_y >= scaled_view_y + scaled_view_height)
        return false;

    int src_view_width = (int)(scaled_view_width / zoom_scale);
    int src_view_height = (int)(scaled_view_height / zoom_scale);

    int src_x = view_x;
    int src_y = view_y;
    int src_w = src_view_width;
    int src_h = src_view_height;

    if(src_x < 0) { src_w += src_x; src_x = 0; }
    if(src_y < 0) { src_h += src_y; src_y = 0; }
    if(src_x + src_w > map_width) src_w = map_width - src_x;
    if(src_y + src_h > map_height) src_h = map_height - src_y;

    if(src_w <= 0 || src_h <= 0)
        return false;

    int dst_w = (int)(src_w * zoom_scale);
    int dst_h = (int)(src_h * zoom_scale);
    int dst_x = scaled_view_x;
    int dst_y = scaled_view_y;

    if(dst_w < scaled_view_width)
        dst_x = scaled_view_x + (scaled_view_width - dst_w) / 2;
    if(dst_h < scaled_view_height)
        dst_y = scaled_view_y + (scaled_view_height - dst_h) / 2;

    if(screen_x < dst_x || screen_x >= dst_x + dst_w ||
       screen_y < dst_y || screen_y >= dst_y + dst_h)
        return false;

    int map_img_x = src_x + (int)((screen_x - dst_x) / zoom_scale);
    int map_img_y = src_y + (int)((screen_y - dst_y) / zoom_scale);

    int map_img_scale = map_width / 1024;
    tile_x = (uint16)(map_img_x / map_img_scale);
    tile_y = (uint16)(map_img_y / map_img_scale);

    if(tile_x > 1023) tile_x = 1023;
    if(tile_y > 1023) tile_y = 1023;

    return true;
}

void WorldMapDialog::drawMarkers()
{
    if(map_width == 0 || map_height == 0)
        return;

    int scale = get_menu_scale();
    int scaled_view_x = area.x + map_view_x * scale;
    int scaled_view_y = area.y + map_view_y * scale;
    int scaled_view_width = map_view_width * scale;
    int scaled_view_height = map_view_height * scale;

    int src_view_width = (int)(scaled_view_width / zoom_scale);
    int src_view_height = (int)(scaled_view_height / zoom_scale);

    int src_x = view_x;
    int src_y = view_y;
    int src_w = src_view_width;
    int src_h = src_view_height;

    if(src_x < 0) { src_w += src_x; src_x = 0; }
    if(src_y < 0) { src_h += src_y; src_y = 0; }
    if(src_x + src_w > map_width) src_w = map_width - src_x;
    if(src_y + src_h > map_height) src_h = map_height - src_y;

    if(src_w <= 0 || src_h <= 0)
        return;

    int dst_w = (int)(src_w * zoom_scale);
    int dst_h = (int)(src_h * zoom_scale);
    int dst_x = scaled_view_x;
    int dst_y = scaled_view_y;

    if(dst_w < scaled_view_width)
        dst_x = scaled_view_x + (scaled_view_width - dst_w) / 2;
    if(dst_h < scaled_view_height)
        dst_y = scaled_view_y + (scaled_view_height - dst_h) / 2;

    int map_img_scale = map_width / 1024;

    GUI *gui = GUI::get_gui();
    GUI_Font *font = gui->get_font();
    FontManager *fm = Game::get_game()->get_font_manager();
    KoreanFont *korean_font = (fm && fm->is_korean_enabled()) ? fm->get_korean_font() : NULL;
    Screen *scr = Game::get_game()->get_screen();

    // Calculate minimum zoom to determine if we should show numbers
    float min_scale_x = (float)scaled_view_width / map_width;
    float min_scale_y = (float)scaled_view_height / map_height;
    float min_zoom = (min_scale_x < min_scale_y) ? min_scale_x : min_scale_y;
    bool show_numbers = (zoom_scale > min_zoom * 1.1f);  // Only show numbers when zoomed in

    for(size_t i = 0; i < markers.size(); i++)
    {
        const MapMarker &m = markers[i];

        int marker_map_x = m.x * map_img_scale;
        int marker_map_y = m.y * map_img_scale;

        if(marker_map_x < src_x || marker_map_x >= src_x + src_w ||
           marker_map_y < src_y || marker_map_y >= src_y + src_h)
            continue;

        int screen_x = dst_x + (int)((marker_map_x - src_x) * zoom_scale);
        int screen_y = dst_y + (int)((marker_map_y - src_y) * zoom_scale);

        // Draw marker (diamond shape)
        int size = 5;
        uint32 color = getMarkerColor(m.color_index);
        uint32 outline_color = SDL_MapRGB(surface->format, 0, 0, 0);

        // Selected marker is larger
        if((int)i == selected_marker)
            size = 7;

        // Black outline
        for(int d = 0; d <= size + 1; d++)
        {
            SDL_Rect r;
            r.x = screen_x - (size + 1 - d);
            r.y = screen_y - d;
            r.w = (size + 1 - d) * 2 + 1;
            r.h = 1;
            SDL_FillRect(surface, &r, outline_color);

            r.y = screen_y + d;
            SDL_FillRect(surface, &r, outline_color);
        }

        // Colored fill
        for(int d = 0; d <= size; d++)
        {
            SDL_Rect r;
            r.x = screen_x - (size - d);
            r.y = screen_y - d;
            r.w = (size - d) * 2 + 1;
            r.h = 1;
            SDL_FillRect(surface, &r, color);

            r.y = screen_y + d;
            SDL_FillRect(surface, &r, color);
        }

        // Draw number next to marker (only when zoomed in)
        if(show_numbers)
        {
            char num_str[8];
            snprintf(num_str, sizeof(num_str), "%d", (int)i + 1);

            if(korean_font)
            {
                korean_font->drawStringUTF8(scr, num_str, screen_x + size + 3, screen_y - 4*scale, 0x0f, 0x0f, 1);  // White color
            }
            else
            {
                if(scale > 1)
                    font->TextOutScaled(surface, screen_x + size + 3, screen_y - 4*scale, num_str, scale);
                else
                    font->TextOut(surface, screen_x + size + 3, screen_y - 4*scale, num_str);
            }
        }
    }
}

int WorldMapDialog::getMarkerAtScreen(int screen_x, int screen_y)
{
    uint16 tile_x, tile_y;
    if(!screenToTile(screen_x, screen_y, tile_x, tile_y))
        return -1;

    for(size_t i = 0; i < markers.size(); i++)
    {
        int dx = (int)markers[i].x - (int)tile_x;
        int dy = (int)markers[i].y - (int)tile_y;
        if(dx * dx + dy * dy <= 25)
            return (int)i;
    }

    return -1;
}

void WorldMapDialog::addMarkerAtScreen(int screen_x, int screen_y)
{
    // Note: Allow adding markers even when underground
    // (markers are just map coordinates, useful for "entrance above here" notes)

    uint16 tile_x, tile_y;
    if(!screenToTile(screen_x, screen_y, tile_x, tile_y))
        return;

    MapMarker m;
    m.x = tile_x;
    m.y = tile_y;
    m.text = "";
    m.color_index = markers.size() % MARKER_NUM_COLORS;

    markers.push_back(m);
    DEBUG(0, LEVEL_INFORMATIONAL, "Added marker at tile (%d, %d)\n", tile_x, tile_y);
}

void WorldMapDialog::deleteMarker(int index)
{
    if(index >= 0 && index < (int)markers.size())
    {
        markers.erase(markers.begin() + index);
    }
}

void WorldMapDialog::loadMarkers()
{
    markers.clear();

    FILE *f = fopen("worldmap_markers.txt", "r");
    if(!f)
        return;

    char line[512];
    while(fgets(line, sizeof(line), f))
    {
        MapMarker m;
        int x, y, color;
        char text[400] = "";

        // Format: x,y,color,text (text is UTF-8)
        if(sscanf(line, "%d,%d,%d,%399[^\n]", &x, &y, &color, text) >= 3)
        {
            m.x = (uint16)x;
            m.y = (uint16)y;
            m.color_index = (uint8)color;
            m.text = text;
            markers.push_back(m);
        }
    }

    fclose(f);
    DEBUG(0, LEVEL_INFORMATIONAL, "Loaded %d markers from worldmap_markers.txt\n", (int)markers.size());
}

void WorldMapDialog::saveMarkers()
{
    FILE *f = fopen("worldmap_markers.txt", "w");
    if(!f)
        return;

    for(size_t i = 0; i < markers.size(); i++)
    {
        const MapMarker &m = markers[i];
        fprintf(f, "%d,%d,%d,%s\n", m.x, m.y, m.color_index, m.text.c_str());
    }

    fclose(f);
    DEBUG(0, LEVEL_INFORMATIONAL, "Saved %d markers to worldmap_markers.txt\n", (int)markers.size());
}
