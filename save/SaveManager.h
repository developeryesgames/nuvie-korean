#ifndef __SaveManager_h__
#define __SaveManager_h__

/*
 *  SaveManager.h
 *  Nuvie
 *
 *  Created by Eric Fry on Wed Apr 28 2004.
 *  Copyright (c) 2004. All rights reserved.
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

#include <string>
#include <list>

#include <SDL.h>
#include "GUI_CallBack.h"

#define QUICK_LOAD true
#define QUICK_SAVE false

// Autosave interval in milliseconds (1 minute = 60000ms)
#define AUTOSAVE_INTERVAL_MS 60000

class Configuration;


class SaveDialog;
class SaveGame;
class SaveSlot;

class SaveManager : public GUI_CallBack
{
 Configuration *config;
 ActorManager *actor_manager;
 ObjManager *obj_manager;

 int game_type;

 SaveGame *savegame;

 // Autosave
 bool autosave_enabled;
 uint32 last_autosave_time;
 bool autosave_pending;  // Set to true when level change triggers autosave

 std::string savedir;
 std::string search_prefix; //eg. nuvieU6, nuvieMD or nuvieSE
 // gui widgets;

 SaveDialog *dialog;

 public:

 SaveManager(Configuration *cfg);
 virtual ~SaveManager();

 bool init();

 bool load_save();
 bool load_latest_save();

 void create_dialog();
 SaveDialog *get_dialog() { return dialog; }

 bool load(SaveSlot *save_slot);
 bool save(SaveSlot *save_slot);
 bool quick_save(int save_num, bool load);

 // Autosave functions
 bool autosave();
 void check_autosave();  // Called from game loop
 void trigger_autosave_on_map_change();  // Called on level change
 bool is_autosave_enabled() { return autosave_enabled; }
 void set_autosave_enabled(bool enabled) { autosave_enabled = enabled; if(enabled) last_autosave_time = SDL_GetTicks(); }

 std::string get_new_savefilename();
 std::string get_savegame_directory() { return savedir; }

 GUI_status callback(uint16 msg, GUI_CallBack *caller, void *data);

 protected:



};

#endif /* __SaveManager_h__ */
