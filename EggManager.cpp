/*
 *  EggManager.cpp
 *  Nuvie
 *
 *  Created by Eric Fry on Thu Mar 20 2003.
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

#include <list>
#include <cassert>
#include "nuvieDefs.h"
#include "Configuration.h"

#include "Actor.h"
#include "TileManager.h"
#include "ActorManager.h"
#include "U6misc.h"
#include "U6LList.h"
#include "EggManager.h"
#include "NuvieIOFile.h"
#include "GameClock.h"
#include "Game.h"
#include "Party.h"
#include "U6WorkTypes.h"
#include "U6objects.h" //needed for silver serpent exception
#include "MsgScroll.h"
#include "FontManager.h"
#include "Player.h"
#include "KoreanTranslation.h"

// Shamino warning flag for off-screen enemy spawning (original U6 behavior)
static bool shamino_spawn_warned = false;

/* ALWAYS means the time is unset, is unknown, or day and night are both set */
typedef enum
{
    EGG_HATCH_ALWAYS, EGG_HATCH_DAY, EGG_HATCH_NIGHT
} egg_hatch_time;
#define EGG_DAY_HOUR   06 /* first hour of the day */
#define EGG_NIGHT_HOUR 19 /* first hour of night */
#define EGG_HATCH_TIME(EQ) (EQ < 10) ? EGG_HATCH_ALWAYS \
                            : (EQ < 20) ? EGG_HATCH_DAY \
                            : (EQ < 30) ? EGG_HATCH_NIGHT : EGG_HATCH_ALWAYS;


EggManager::EggManager(Configuration *cfg, nuvie_game_t type, Map *m)
{
 config = cfg;
 gametype = type;
 map = m;
 actor_manager = NULL;
 obj_manager = NULL;
 not_spawning_actors = false;
}

EggManager::~EggManager()
{

}

void EggManager::clean(bool keep_obj)
{
 Egg *egg;
 std::list<Egg *>::iterator egg_iter;

 for(egg_iter = egg_list.begin(); egg_iter != egg_list.end();)
   {
    egg = *egg_iter;

   // eggs are always on the map now.
   // if(keep_obj == false)
   //   delete_obj(egg->obj);

    delete *egg_iter;
    egg_iter = egg_list.erase(egg_iter);
   }

}

void EggManager::add_egg(Obj *egg_obj)
{
 Egg *egg;

 if(egg_obj == NULL)
  return;

 egg = new Egg();
 egg->obj = egg_obj;

 egg_list.push_back(egg);

 return;
}


void EggManager::remove_egg(Obj *egg_obj, bool keep_obj)
{
 std::list<Egg *>::iterator egg_iter;

 for(egg_iter = egg_list.begin(); egg_iter != egg_list.end(); egg_iter++)
   {
    if((*egg_iter)->obj == egg_obj)
       {
        //if(keep_obj == false) eggs always on map now.

        //obj_manager->unlink_from_engine((*egg_iter)->obj);
        //delete_obj((*egg_iter)->obj);

        delete *egg_iter;
        egg_list.erase(egg_iter);

        break;
       }
   }

 return;
}

void EggManager::set_egg_visibility(bool show_eggs)
{
 std::list<Egg *>::iterator egg_iter;

 for(egg_iter = egg_list.begin(); egg_iter != egg_list.end(); egg_iter++)
    (*egg_iter)->obj->set_invisible(!show_eggs);
}

void EggManager::spawn_eggs(uint16 x, uint16 y, uint8 z, bool teleport)
{
 std::list<Egg *>::iterator egg;
 sint16 dist_x, dist_y;
 uint8 hatch_probability;

 for(egg = egg_list.begin(); egg != egg_list.end();)
   {
    uint8 quality = (*egg)->obj->quality;
    dist_x = abs((sint16)(*egg)->obj->x - x);
    dist_y = abs((sint16)(*egg)->obj->y - y);

    //Deactivate eggs that are more than 20 tiles from player.
    if(((*egg)->obj->status & OBJ_STATUS_EGG_ACTIVE) && ( (*egg)->obj->z != z || (dist_x >= 20 || dist_y >= 20) ) )
    {
    	(*egg)->obj->status &= (0xff ^ OBJ_STATUS_EGG_ACTIVE);
    	DEBUG(0,LEVEL_DEBUGGING, "Reactivate egg at (%x,%x,%d)\n", (*egg)->obj->x, (*egg)->obj->y, (*egg)->obj->z);
    }

    if(dist_x < 20 && dist_y < 20 && (*egg)->obj->z == z
       && (dist_x > 8 || dist_y > 8 || !Game::get_game()->is_orig_style() || teleport))
      {

       if(((*egg)->obj->status & OBJ_STATUS_EGG_ACTIVE) == 0)
         {
          (*egg)->obj->status |= OBJ_STATUS_EGG_ACTIVE;

          hatch_probability = NUVIE_RAND()%100;
          DEBUG(0,LEVEL_DEBUGGING,"Checking Egg (%x,%x,%x). Rand: %d Probability: %d%%",(*egg)->obj->x, (*egg)->obj->y, (*egg)->obj->z,hatch_probability,(*egg)->obj->qty);

          DEBUG(1,LEVEL_DEBUGGING," Align: %s", get_actor_alignment_str(quality % 10));

          if(quality < 10)      DEBUG(1,LEVEL_DEBUGGING," (always)");    // 0-9
          else if(quality < 20) DEBUG(1,LEVEL_DEBUGGING," (day)");       // 10-19
          else if(quality < 30) DEBUG(1,LEVEL_DEBUGGING," (night)");     // 20-29
          else if(quality < 40) DEBUG(1,LEVEL_DEBUGGING," (day+night)"); // 30-39
          DEBUG(1,LEVEL_DEBUGGING,"\n");
          spawn_egg((*egg)->obj, hatch_probability);
         }
      }

    egg++;
   }

 return;
}


bool EggManager::spawn_egg(Obj *egg, uint8 hatch_probability)
{
 U6Link *link;
 uint16 i;
 Obj *obj, *spawned_obj;
 uint16 qty;
 uint8 hour = Game::get_game()->get_clock()->get_hour();
 uint8 alignment = egg->quality % 10;
 bool spawned_hostile = false; // Track if hostile actor spawned

    // check time that the egg will hach
    egg_hatch_time period = EGG_HATCH_TIME(egg->quality);
    if( period==EGG_HATCH_ALWAYS
        || (period==EGG_HATCH_DAY && hour>=EGG_DAY_HOUR && hour<EGG_NIGHT_HOUR)
        || (period==EGG_HATCH_NIGHT && !(hour>=EGG_DAY_HOUR && hour<EGG_NIGHT_HOUR)) )
    {
      if(egg->container == NULL)
      {
        DEBUG(1,LEVEL_WARNING," egg at (%x,%x,%x) does not contain any embryos!", egg->x, egg->y, egg->z);
      }
          // check random probability that the egg will hatch
          if((egg->qty == 100 || hatch_probability <= egg->qty) && egg->container)  // Hatch the egg.
            {
             assert(egg->container);
             for(link = egg->container->start(); link != NULL; link = link->next)
               {
                obj = (Obj *)link->data;
                qty = obj->qty;

                if(gametype == NUVIE_GAME_U6 && obj->obj_n == OBJ_U6_SILVER_SERPENT) //U6 silver serpents only hatch once per egg.
                  qty = 1;

                for(i = 0; i < qty; i++)
                 {
				  if((gametype == NUVIE_GAME_U6 && (obj->obj_n >= OBJ_U6_GIANT_RAT || obj->obj_n == OBJ_U6_CHEST))
						  || obj->quality != 0) /* spawn temp actor we know it's an actor if it has a non-zero worktype. */
				  {
					if((not_spawning_actors && Game::get_game()->are_cheats_enabled())
					   || Game::get_game()->is_armageddon())
						break;
				  	// group new actors randomly if egg space already occupied
				  	Actor *prev_actor = actor_manager->get_actor(egg->x, egg->y, egg->z);
				  	Actor *new_actor = NULL;
				  	MapCoord actor_loc = MapCoord(egg->x, egg->y, egg->z);
				  	if(prev_actor)
				  	{
				  		if(prev_actor->get_obj_n() != obj->obj_n
				  				|| !actor_manager->toss_actor_get_location(egg->x, egg->y, egg->z, 3, 2, &actor_loc)
				  				|| !actor_manager->toss_actor_get_location(egg->x, egg->y, egg->z, 2, 3, &actor_loc))
				  									actor_manager->toss_actor_get_location(egg->x, egg->y, egg->z, 4, 4, &actor_loc);
				  	}
				  	uint8 worktype = get_worktype(obj);
				  	if(actor_manager->create_temp_actor(obj->obj_n,obj->status,actor_loc.x,actor_loc.y,actor_loc.z,alignment,worktype,&new_actor) && prev_actor)
					{
				  		/*
						// try to group actors of the same type first (FIXME: maybe this should use alignment/quality)
						if(prev_actor->get_obj_n() != new_actor->get_obj_n() || !actor_manager->toss_actor(new_actor, 3, 2) || !actor_manager->toss_actor(new_actor, 2, 3))
							actor_manager->toss_actor(new_actor, 4, 4);
						*/
						hatch_probability = NUVIE_RAND()%100;
						if(hatch_probability > egg->qty)
							break; // chance to stop spawning actors
					}
					// Track hostile actor spawn (EVIL=2, CHAOTIC=4)
					if(alignment == ACTOR_ALIGNMENT_EVIL || alignment == ACTOR_ALIGNMENT_CHAOTIC)
						spawned_hostile = true;
				  }
				  else
					{ /* spawn temp object */
					 spawned_obj = new Obj();
					 spawned_obj->obj_n = obj->obj_n;
					 //spawned_obj->x = egg->x+i; // line objects up in a row
					 spawned_obj->x = egg->x; // regeants all grow at the same location
					 spawned_obj->y = egg->y;
					 spawned_obj->z = egg->z;
					 spawned_obj->qty = 1; // (it already spawns qty objects with the loop)
					 spawned_obj->status |= OBJ_STATUS_TEMPORARY | OBJ_STATUS_OK_TO_TAKE;

					 obj_manager->add_obj(spawned_obj, true); // addOnTop
					}
                 }
               }

             // Original U6: Shamino warns when hostile actors spawn off-screen
             if(gametype == NUVIE_GAME_U6 && spawned_hostile && !shamino_spawn_warned)
             {
               Party *party = Game::get_game()->get_party();
               Actor *player = Game::get_game()->get_player()->get_actor();
               if(party && player && party->contains_actor(3)) // Shamino is NPC #3
               {
                 Actor *shamino = actor_manager->get_actor(3);
                 if(shamino && shamino->is_alive())
                 {
                   sint16 shamino_dx = abs((sint16)shamino->get_x() - (sint16)player->get_x());
                   sint16 shamino_dy = abs((sint16)shamino->get_y() - (sint16)player->get_y());
                   if(shamino_dx + shamino_dy < 6) // Shamino close enough to Avatar (Manhattan distance)
                   {
                     MsgScroll *scroll = Game::get_game()->get_scroll();
                     // Original: 75% chance (OSI_rand(0,3) returns non-zero)
                     if(scroll && NUVIE_RAND() % 4 != 0)
                     {
                       sint16 rel_x = (sint16)egg->x - (sint16)player->get_x();
                       sint16 rel_y = (sint16)egg->y - (sint16)player->get_y();
                       FontManager *fm = Game::get_game()->get_font_manager();
                       bool korean = fm && fm->is_korean_enabled();

                       if(korean) {
                         const char *korean_dir;
                         if(rel_x == 0 && rel_y < 0) korean_dir = "북";
                         else if(rel_x > 0 && rel_y < 0) korean_dir = "북동";
                         else if(rel_x > 0 && rel_y == 0) korean_dir = "동";
                         else if(rel_x > 0 && rel_y > 0) korean_dir = "남동";
                         else if(rel_x == 0 && rel_y > 0) korean_dir = "남";
                         else if(rel_x < 0 && rel_y > 0) korean_dir = "남서";
                         else if(rel_x < 0 && rel_y == 0) korean_dir = "서";
                         else if(rel_x < 0 && rel_y < 0) korean_dir = "북서";
                         else korean_dir = "";
                         scroll->display_string(std::string("\n샤미노 : \"") + korean_dir + "쪽에서 뭔가 들리오!\"\n");
                       } else {
                         const char *dir_names[] = {"North", "Northeast", "East", "Southeast", "South", "Southwest", "West", "Northwest"};
                         int dir_idx = 0;
                         if(rel_x == 0 && rel_y < 0) dir_idx = 0;
                         else if(rel_x > 0 && rel_y < 0) dir_idx = 1;
                         else if(rel_x > 0 && rel_y == 0) dir_idx = 2;
                         else if(rel_x > 0 && rel_y > 0) dir_idx = 3;
                         else if(rel_x == 0 && rel_y > 0) dir_idx = 4;
                         else if(rel_x < 0 && rel_y > 0) dir_idx = 5;
                         else if(rel_x < 0 && rel_y == 0) dir_idx = 6;
                         else if(rel_x < 0 && rel_y < 0) dir_idx = 7;
                         scroll->display_string(std::string("\nShamino says, \"I hear something to the ") + dir_names[dir_idx] + "!\"\n");
                       }
                     }
                     shamino_spawn_warned = true; // Set flag - won't warn again until reset
                   }
                 }
               }
             }
             return true;
            }
    }
 return false;
}

// Reset Shamino spawn warning flag (called when no enemies remain)
void EggManager::reset_shamino_warning()
{
  shamino_spawn_warned = false;
}

uint8 EggManager::get_worktype(Obj *embryo)
{
	if(gametype == NUVIE_GAME_U6
			&& (embryo->obj_n == OBJ_U6_WINGED_GARGOYLE || embryo->obj_n == OBJ_U6_GARGOYLE)
			&& (Game::get_game()->get_party()->has_obj(OBJ_U6_AMULET_OF_SUBMISSION, 0, false)
			    || Game::get_game()->get_party()->contains_actor(164))) // Beh lem
	{
		return WORKTYPE_U6_ANIMAL_WANDER;
	}

	return embryo->quality;
}
