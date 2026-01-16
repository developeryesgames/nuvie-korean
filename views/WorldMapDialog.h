#ifndef __WorldMapDialog_h__
#define __WorldMapDialog_h__
/*
 *  WorldMapDialog.h
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

#include "GUI_Dialog.h"
#include <vector>
#include <string>

class GUI_Button;
class GUI_TextInput;
class GUI_ScrollBar;
class Player;

#define WORLDMAP_CB_CLOSE 0x1

// Marker colors (8 colors)
#define MARKER_COLOR_RED      0
#define MARKER_COLOR_BLUE     1
#define MARKER_COLOR_GREEN    2
#define MARKER_COLOR_ORANGE   3
#define MARKER_COLOR_PURPLE   4
#define MARKER_COLOR_CYAN     5
#define MARKER_COLOR_YELLOW   6
#define MARKER_COLOR_WHITE    7
#define MARKER_NUM_COLORS     8

// Map marker/memo structure
struct MapMarker {
    uint16 x, y;           // Tile coordinates (0-1023)
    std::string text;      // Memo text (UTF-8)
    uint8 color_index;     // Marker color (0-7)
};

class WorldMapDialog : public GUI_Dialog {
protected:
    GUI_Button *close_button;
    GUI_Button *zoom_in_button;
    GUI_Button *zoom_out_button;

    SDL_Surface *worldmap_surface;  // The loaded world map image

    // View parameters
    int view_x, view_y;      // Top-left corner of view in map coordinates
    float zoom_scale;        // Zoom scale: 1.0=1:1, 0.5=zoom out 2x, 2.0=zoom in 2x
    int map_width, map_height;  // Size of loaded map

    // Dialog dimensions
    int map_view_width;      // Width of map view area
    int map_view_height;     // Height of map view area
    int map_view_x;          // X offset of map view area
    int map_view_y;          // Y offset of map view area

    // Player position
    uint16 player_x, player_y;
    bool is_underground;     // True if player is underground (z != 0)

    // Dragging
    bool dragging;
    int drag_start_x, drag_start_y;
    int drag_view_start_x, drag_view_start_y;

    // Animation
    uint32 anim_start_time;  // For marker animation

    // Markers/Memos
    std::vector<MapMarker> markers;
    int selected_marker;     // Index of selected marker, -1 if none

    // Memo list UI
    int memo_list_x;         // X position of memo list panel (base coords)
    int memo_list_y;         // Y position of memo list panel (base coords)
    int memo_list_width;     // Width of memo list panel (base coords)
    int memo_list_height;    // Height of memo list panel (base coords)
    int memo_scroll_offset;  // Scroll offset for memo list
    int memo_item_height;    // Height of each memo item (base coords)
    int memo_visible_count;  // Number of visible memo items

    // Memo editing
    GUI_Button *add_memo_button;
    GUI_Button *delete_memo_button;
    GUI_Button *color_button;
    bool editing_memo;       // True when editing a memo
    std::string editing_text; // Current text being edited
    std::string composing_text; // IME composition text (Korean input preview)

    // Double-click detection
    uint32 last_click_time;
    int last_click_marker;

    // Scrollbar
    GUI_ScrollBar *memo_scrollbar;

    // Internal methods
    bool loadWorldMap();
    bool generateWorldMap();  // Generate world map from tiles at runtime
    void drawMapView();
    void drawPlayerMarker();
    void drawMarkers();
    void drawMemoList();
    void centerOnPlayer();
    void clampViewPosition();

    // Marker methods
    void loadMarkers();
    void saveMarkers();
    int getMarkerAtScreen(int screen_x, int screen_y);
    void addMarkerAtScreen(int screen_x, int screen_y);
    void deleteMarker(int index);
    bool screenToTile(int screen_x, int screen_y, uint16 &tile_x, uint16 &tile_y);

    // Memo list methods
    int getMemoVisibleCount();  // Calculate visible memo items based on scaling
    int getMemoItemAtScreen(int screen_x, int screen_y);
    void scrollMemoList(int delta);
    void selectMarker(int index);
    void startEditingMemo();
    void finishEditingMemo();
    void cycleMarkerColor();
    uint32 getMarkerColor(int color_index);
    void updateScrollbar();

public:
    WorldMapDialog();
    ~WorldMapDialog();

    bool init();

    void Display(bool full_redraw);
    GUI_status close_dialog();
    GUI_status KeyDown(SDL_Keysym key);
    GUI_status TextInput(const char *text);
    GUI_status TextEditing(const char *text, int start, int length);
    GUI_status MouseDown(int x, int y, int button);
    GUI_status MouseUp(int x, int y, int button);
    GUI_status MouseMotion(int x, int y, Uint8 state);
    GUI_status MouseWheel(sint32 x, sint32 y);
    GUI_status HandleEvent(const SDL_Event *event);

    GUI_status callback(uint16 msg, GUI_CallBack *caller, void *data);

    void zoomIn();
    void zoomOut();
    void zoomAtPoint(int screen_x, int screen_y, bool zoom_in);
    void scrollMap(int dx, int dy);
};

#endif /* __WorldMapDialog_h__ */
