#include <cassert>
#include <stdlib.h>
#include <string.h>
#include "nuvieDefs.h"
#include "Configuration.h"
#include "Screen.h"
#include "NuvieIO.h"
#include "NuvieIOFile.h"
#include "U6misc.h"
#include "U6Lzw.h"
#include "U6Shape.h"
#include "U6Lib_n.h"
#include "Cursor.h"
#include "Game.h"

using std::string;
using std::vector;


Cursor::Cursor()
{
    cursor_id = 0;
    cur_x = cur_y = -1;
    cleanup = NULL;
    cleanup_area.x = cleanup_area.y = 0;
    cleanup_area.w = cleanup_area.h = 0;
    update_area.x = update_area.y = 0;
    update_area.w = update_area.h = 0;
    hidden = false;
    screen = NULL;
    config = NULL;
    screen_w =screen_h = 0;
}


/* Returns true if mouse pointers file was loaded.
 */
bool Cursor::init(Configuration *c, Screen *s, nuvie_game_t game_type)
{
    std::string file, filename;
    bool enable_cursors;
    
    config = c;
    screen = s;
    
    screen_w = screen->get_width();
    screen_h = screen->get_height();

    config->value("config/general/enable_cursors", enable_cursors, true);
    
    if(!enable_cursors)
    	return false;
    switch(game_type)
    {
		case NUVIE_GAME_U6 : file = "u6mcga.ptr"; break;
		case NUVIE_GAME_SE : file = "secursor.ptr"; break;
		case NUVIE_GAME_MD : file = "mdcursor.ptr"; break;
    }

    config_get_path(config, file, filename);
    
    if(filename != "")
        if(load_all(filename, game_type) > 0)
            return(true);
    return(false);
}


/* Load pointers from `filename'. (lzw -> s_lib_32 -> shapes)
 * Returns the number found in the file.
 */
uint32 Cursor::load_all(std::string filename, nuvie_game_t game_type)
{
    U6Lzw decompressor;
    U6Lib_n pointer_list;
    NuvieIOBuffer iobuf;
    uint32 slib32_len = 0;
    unsigned char *slib32_data;
    if(game_type != NUVIE_GAME_U6)
    {
    	U6Lib_n file;
    	file.open(filename, 4, game_type);
    	slib32_data = file.get_item(0);
    	slib32_len = file.get_item_size(0);
    }
    else
    {
    	slib32_data = decompressor.decompress_file(filename, slib32_len);
    }

    if(slib32_len == 0)
    	return(0);
    // FIXME: u6lib_n assumes u6 libs have no filesize header
    iobuf.open(slib32_data, slib32_len);
    free(slib32_data);

    if(!pointer_list.open(&iobuf, 4, NUVIE_GAME_MD))
    	return(0);


    uint32 num_read = 0, num_total = pointer_list.get_num_items();
    cursors.resize(num_total);
    while(num_read < num_total) // read each into a new MousePointer
    {
        MousePointer *ptr = NULL;
        U6Shape *shape = new U6Shape;
        unsigned char *data = pointer_list.get_item(num_read);
        if(!shape->load(data))
        {
           free(data);
           delete shape;
           break;
        }
        ptr = new MousePointer; // set from shape data
        shape->get_hot_point(&(ptr->point_x), &(ptr->point_y));
        shape->get_size(&(ptr->w), &(ptr->h));
        ptr->shapedat = (unsigned char *)malloc(ptr->w * ptr->h);
        memcpy(ptr->shapedat, shape->get_data(), ptr->w * ptr->h);
        cursors[num_read++] = ptr;

        free(data);
        delete shape;
    }
    pointer_list.close();
    iobuf.close();
    return(num_read);
}


/* Free data.
 */
void Cursor::unload_all()
{
    for(uint32 i = 0; i < cursors.size(); i++)
    {
        if(cursors[i] && cursors[i]->shapedat)
            free(cursors[i]->shapedat);
        delete cursors[i];
    }
    if(cleanup)
        free(cleanup);
}


/* Set active pointer.
 */
bool Cursor::set_pointer(uint8 ptr_num)
{
    if(ptr_num >= cursors.size() || !cursors[ptr_num])
        return(false);

    cursor_id = ptr_num;
    return(true);
}


/* Draw self on screen at px,py, or at mouse location if px or py is -1.
 * Returns false on failure.
 */
bool Cursor::display(sint32 px, sint32 py)
{
    if(cursors.empty() || !cursors[cursor_id])
        return(false);
    if(hidden)
        return(true);
    if(px == -1 || py == -1)
    {
        screen->get_mouse_location(&px, &py);
//        DEBUG(0,LEVEL_DEBUGGING,"mouse pos: %d,%d", px, py);
    }
    MousePointer *ptr = cursors[cursor_id];

    // Check for Korean 4x mode
    uint8 cursor_scale = 1;
    Game *game = Game::get_game();
    if(game) {
        uint16 ui_scale = game->get_game_width() / 320;
        if(ui_scale >= 4) {
            cursor_scale = 4;
        }
    }

    fix_position(ptr, px, py, cursor_scale); // modifies px, py

    uint16 scaled_w = ptr->w * cursor_scale;
    uint16 scaled_h = ptr->h * cursor_scale;
    save_backing((uint32)px, (uint32)py, (uint32)scaled_w, (uint32)scaled_h);

    if(cursor_scale >= 4) {
        screen->blit4x((uint16)px, (uint16)py, ptr->shapedat, 8, ptr->w, ptr->h, ptr->w, true, NULL);
    } else if(cursor_scale == 3) {
        screen->blit3x((uint16)px, (uint16)py, ptr->shapedat, 8, ptr->w, ptr->h, ptr->w, true, NULL);
    } else if(cursor_scale >= 2) {
        screen->blit2x((uint16)px, (uint16)py, ptr->shapedat, 8, ptr->w, ptr->h, ptr->w, true, NULL);
    } else {
        screen->blit((uint16)px, (uint16)py, ptr->shapedat, 8, ptr->w, ptr->h, ptr->w, true);
    }

//    screen->update(px, py, ptr->w, ptr->h);
    add_update(px, py, scaled_w, scaled_h);
    update();
    return(true);
}


/* Restore backing behind cursor (hide until next display). Must call update()
 * sometime after to remove from screen.
 */
void Cursor::clear()
{
    if(cleanup)
    {
        screen->restore_area(cleanup, &cleanup_area);
        cleanup = NULL;
//        screen->update(cleanup_area.x, cleanup_area.y, cleanup_area.w, cleanup_area.h);
        add_update(cleanup_area.x, cleanup_area.y, cleanup_area.w, cleanup_area.h);
    }
}


/* Offset requested position px,py by pointer hotspot, and screen boundary.
 */
inline void Cursor::fix_position(MousePointer *ptr, sint32 &px, sint32 &py, uint8 scale)
{
    sint32 hotspot_x = ptr->point_x * scale;
    sint32 hotspot_y = ptr->point_y * scale;
    sint32 scaled_w = ptr->w * scale;
    sint32 scaled_h = ptr->h * scale;

    if((px - hotspot_x) < 0) // offset by hotspot
        px = 0;
    else
        px -= hotspot_x;
    if((py - hotspot_y) < 0)
        py = 0;
    else
        py -= hotspot_y;
    if((px + scaled_w) >= screen_w) // don't draw offscreen
        px = screen_w - scaled_w - 1;
    if((py + scaled_h) >= screen_h)
        py = screen_h - scaled_h - 1;
}


/* Copy cleanup area (cursor backingstore) from screen.
 */
void Cursor::save_backing(uint32 px, uint32 py, uint32 w, uint32 h)
{
    if(cleanup)
    {
        free(cleanup);
        cleanup = NULL;
    }

    cleanup_area.x = px; // cursor must be drawn LAST for this to work
    cleanup_area.y = py;
    cleanup_area.w = w;
    cleanup_area.h = h;
    cleanup = screen->copy_area(&cleanup_area);
}


/* Mark update_area (cleared/displayed) as updated on the screen.
 */
void Cursor::update()
{
    screen->update(update_area.x, update_area.y, update_area.w, update_area.h);
    update_area.x = update_area.y = 0;
    update_area.w = update_area.h = 0;
}


/* Add to update_area.
 */
void Cursor::add_update(uint16 x, uint16 y, uint16 w, uint16 h)
{
    if(update_area.w == 0 || update_area.h == 0)
    {
        update_area.x = x; update_area.y = y;
        update_area.w = w; update_area.h = h;
    }
    else
    {
        uint16 x2 = x + w, y2 = y + h,
               update_x2 = update_area.x + update_area.w, update_y2 = update_area.y + update_area.h;
        if(x <= update_area.x) update_area.x = x;
        if(y <= update_area.y) update_area.y = y;
        if(x2 >= update_x2) update_x2 = x2;
        if(y2 >= update_y2) update_y2 = y2;
        update_area.w = update_x2 - update_area.x;
        update_area.h = update_y2 - update_area.y;
    }
}

