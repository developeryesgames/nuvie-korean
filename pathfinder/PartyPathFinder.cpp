#include <cassert>
#include <vector>
#include "U6misc.h"
#include "Actor.h"
#include "Party.h"
#include "SeekPath.h"
#include "ActorPathFinder.h"
#include "PartyPathFinder.h"
#include "Game.h"
#include "Map.h"

using std::vector;

PartyPathFinder::PartyPathFinder(Party *p)
{
    assert(p);
    party = p;
}

PartyPathFinder::~PartyPathFinder()
{

}

/* True if a member's target and leader are in roughly the same direction. */
bool PartyPathFinder::is_behind_target(uint32 member_num)
{
    if(get_leader() < 0)
        return false;
    uint8 ldir = get_member(get_leader()).actor->get_direction(); // leader direciton
    MapCoord from = party->get_location(member_num);
    MapCoord to = party->get_formation_coords(member_num); // target
    sint8 to_x = to.x-from.x, to_y = to.y-from.y;
    return(  ((ldir == NUVIE_DIR_N && to_y < 0)
           || (ldir == NUVIE_DIR_S && to_y > 0)
           || (ldir == NUVIE_DIR_E && to_x > 0)
           || (ldir == NUVIE_DIR_W && to_x < 0)));
}

bool PartyPathFinder::is_at_target(uint32 p)
{
    MapCoord target_loc = party->get_formation_coords(p);
    MapCoord member_loc = party->get_location(p);
    return(target_loc == member_loc);
}

/* Is anyone in front of `member_num' adjacent to `from'?
 * (is_contiguous(member, member_loc) == "is member adjacent to another member
 * whose following position is lower-numbered?") */
bool PartyPathFinder::is_contiguous(uint32 member_num, MapCoord from)
{
    for(uint32 q = 0; q < member_num; q++) // check lower-numbered members
    {
    	Actor *actor = get_member(q).actor;
        if(actor && actor->is_immobile() == true) continue;

        MapCoord loc = party->get_location(q);
        if(from.distance(loc) <= 1)
            return true;
    }
    return false;
}

/* Is member adjacent to another member whose following position is lower-numbered? */
bool PartyPathFinder::is_contiguous(uint32 member_num)
{
    MapCoord member_loc = party->get_location(member_num);
    return(is_contiguous(member_num, member_loc));
}

/* Returns in rel_x and rel_y the direction a character needs to move to get
 * closer to their target. */
void PartyPathFinder::get_target_dir(uint32 p, sint8 &rel_x, sint8 &rel_y)
{
    MapCoord leader_loc = party->get_leader_location();
    MapCoord target_loc = party->get_formation_coords(p);
    MapCoord member_loc = party->get_location(p);

    rel_x = get_wrapped_rel_dir(target_loc.x,member_loc.x, target_loc.z);
    rel_y = get_wrapped_rel_dir(target_loc.y,member_loc.y, target_loc.z);
}

/* Returns in vec_x and vec_y the last direction the leader moved in. It's
 * derived from his facing direction so it's not as precise as get_last_move(). */
void PartyPathFinder::get_forward_dir(sint8 &vec_x, sint8 &vec_y)
{
//    get_last_move(vec_x, vec_y);
    vec_x=0; vec_y=0;
    uint8 dir = (get_leader()>=0)?get_member(get_leader()).actor->get_direction():NUVIE_DIR_N;
    if(dir == NUVIE_DIR_N)      { vec_x = 0; vec_y = -1; }
    else if(dir == NUVIE_DIR_S) { vec_x = 0; vec_y = 1; }
    else if(dir == NUVIE_DIR_E) { vec_x = 1; vec_y = 0; }
    else if(dir == NUVIE_DIR_W) { vec_x = -1; vec_y = 0; }
}

/* Returns in vec_x and vec_y the last direction the leader moved in. */
void PartyPathFinder::get_last_move(sint8 &vec_x, sint8 &vec_y)
{
    MapCoord leader_loc = party->get_leader_location();

    vec_x = get_wrapped_rel_dir(leader_loc.x,party->prev_leader_x, leader_loc.z);
    vec_y = get_wrapped_rel_dir(leader_loc.y,party->prev_leader_y, leader_loc.z);
}

/* Returns true if the leader moved before the last call to follow(). */
bool PartyPathFinder::leader_moved()
{
    MapCoord leader_loc = party->get_leader_location();
    return(  (leader_loc.x-party->prev_leader_x)
           ||(leader_loc.y-party->prev_leader_y));
}

/* Returns true if the leader moved far enough away from follower to pull him. */
bool PartyPathFinder::leader_moved_away(uint32 p)
{
    MapCoord leader_loc = party->get_leader_location();
    MapCoord target_loc = party->get_formation_coords(p);
    MapCoord member_loc = party->get_location(p);
    return(leader_loc.distance(member_loc) > leader_loc.distance(target_loc));
}

/* Compares leader position with last known position. */
bool PartyPathFinder::leader_moved_diagonally()
{
    MapCoord leader_loc = party->get_leader_location();
    return(party->prev_leader_x != leader_loc.x
           && party->prev_leader_y != leader_loc.y);
}

/* Original U6-style eager-based direction selection for narrow passages.
 * Based on original U6 MoveFollowers() function in seg_1E0F.c
 *
 * Original U6 logic (seg_1E0F.c:560-587):
 * - C_1E0F_000F() checks if move is possible AND sets D_17A9 if tile is damaging
 * - If D_17A9 (danger tile): eager = 128, and skip if already contiguous
 * - If safe tile: eager = 256
 * - Contiguous bonus: +256
 * - If currently contiguous but move breaks it: eager = 0
 * - Subtract manhattan distance to target
 */
bool PartyPathFinder::follow_passA(uint32 p)
{
    bool contiguous = is_contiguous(p);

    // If already at target and contiguous, nothing to do
    if(contiguous && is_at_target(p))
        return true;

    MapCoord member_loc = party->get_location(p);
    MapCoord target_loc = party->get_formation_coords(p);

    // Direction offsets (N, NE, E, SE, S, SW, W, NW) - same as original U6 DirIncrX/Y
    static const sint8 dir_x[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static const sint8 dir_y[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

    sint32 max_eager = 0;  // Original U6 uses 0 as initial max_eager
    sint8 best_dir = -1;
    sint8 best_dx = 0, best_dy = 0;

    Actor *actor = get_member(p).actor;
    if(!actor) return true;

    Game *game = Game::get_game();
    if(!game) return true;
    Map *map = game->get_game_map();
    if(!map) return true;

    for(int dir = 0; dir < 8; dir++)
    {
        sint8 dx = dir_x[dir];
        sint8 dy = dir_y[dir];
        MapCoord try_loc = member_loc.abs_coords(dx, dy);

        // Check if move is possible - use ACTOR_IGNORE_DANGER to allow dangerous tiles
        // Original U6 C_1E0F_000F() allows move but sets D_17A9 flag for danger
        ActorMoveFlags flags = ACTOR_IGNORE_MOVES | ACTOR_IGNORE_DANGER;
        if(!actor->check_move(try_loc.x, try_loc.y, try_loc.z, flags))
            continue;

        // Check if this is a damaging tile (original U6: D_17A9 flag)
        bool is_danger = map->is_damaging(try_loc.x, try_loc.y, try_loc.z);

        sint32 eager;
        if(is_danger)
        {
            // Original U6: if danger tile and already contiguous, skip this direction
            if(contiguous)
                continue;
            eager = 128;  // Lower base score for danger tiles
        }
        else
        {
            eager = 256;  // Normal base score for safe tiles
        }

        // Contiguous bonus: +256 if this move keeps/makes us contiguous
        if(is_contiguous(p, try_loc))
            eager += 256;
        else if(contiguous)
            eager = 0;  // Don't break contiguity in first pass

        // Subtract manhattan distance to target
        eager -= abs((sint32)try_loc.x - (sint32)target_loc.x);
        eager -= abs((sint32)try_loc.y - (sint32)target_loc.y);

        if(eager > max_eager)
        {
            max_eager = eager;
            best_dir = dir;
            best_dx = dx;
            best_dy = dy;
        }
    }

    // Make the best move if found (original U6: if new_dir >= 0)
    if(best_dir >= 0)
    {
        MapCoord dest = member_loc.abs_coords(best_dx, best_dy);
        // Use ACTOR_IGNORE_DANGER for actual move too - let the game handle damage
        if(actor->move(dest.x, dest.y, dest.z, ACTOR_IGNORE_MOVES | ACTOR_IGNORE_DANGER))
        {
            actor->set_direction(best_dx, best_dy);
            return true;
        }
    }

    // Fallback: try moving towards target with original logic
    sint8 rel_x = 0, rel_y = 0;
    get_target_dir(p, rel_x, rel_y);
    if(move_member(p, rel_x, rel_y, !contiguous))
        return true;

    return false;
}

/* Second pass with relaxed conditions - allows non-contiguous moves to catch up.
 * Based on original U6 MoveFollowers() second pass (seg_1E0F.c:523, pass == 1)
 *
 * Original U6 pass 2 conditions (line 552-555):
 * - If aFlag: move if not at target position
 * - If !aFlag: move if target is ahead in facing direction
 *
 * Pass 2 uses same eager calculation but allows movement even when
 * already contiguous, to help catch up to the party.
 */
bool PartyPathFinder::follow_passB(uint32 p)
{
    bool contiguous = is_contiguous(p);

    // If already at target and contiguous, done
    if(contiguous && is_at_target(p))
        return true;

    MapCoord member_loc = party->get_location(p);
    MapCoord target_loc = party->get_formation_coords(p);

    // Direction offsets (N, NE, E, SE, S, SW, W, NW)
    static const sint8 dir_x[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    static const sint8 dir_y[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

    sint32 max_eager = 0;
    sint8 best_dir = -1;
    sint8 best_dx = 0, best_dy = 0;

    Actor *actor = get_member(p).actor;
    if(!actor) return true;

    Game *game = Game::get_game();
    if(!game) return true;
    Map *map = game->get_game_map();
    if(!map) return true;

    // Pass 2: check if we need to move (original U6 pass 2 conditions)
    // Move if not contiguous OR not at target position
    bool need_move = !contiguous || !is_at_target(p);

    if(need_move)
    {
        for(int dir = 0; dir < 8; dir++)
        {
            sint8 dx = dir_x[dir];
            sint8 dy = dir_y[dir];
            MapCoord try_loc = member_loc.abs_coords(dx, dy);

            // Check if move is possible - allow dangerous tiles
            ActorMoveFlags flags = ACTOR_IGNORE_MOVES | ACTOR_IGNORE_DANGER;
            if(!actor->check_move(try_loc.x, try_loc.y, try_loc.z, flags))
                continue;

            // Check if this is a damaging tile
            bool is_danger = map->is_damaging(try_loc.x, try_loc.y, try_loc.z);

            sint32 eager;
            if(is_danger)
            {
                // In pass 2, still skip danger tiles if currently contiguous
                if(contiguous)
                    continue;
                eager = 128;
            }
            else
            {
                eager = 256;
            }

            // Contiguous bonus
            if(is_contiguous(p, try_loc))
                eager += 256;
            else if(contiguous)
                eager = 0;  // Still don't break contiguity unnecessarily

            // Subtract manhattan distance to target
            eager -= abs((sint32)try_loc.x - (sint32)target_loc.x);
            eager -= abs((sint32)try_loc.y - (sint32)target_loc.y);

            if(eager > max_eager)
            {
                max_eager = eager;
                best_dir = dir;
                best_dx = dx;
                best_dy = dy;
            }
        }

        if(best_dir >= 0)
        {
            MapCoord dest = member_loc.abs_coords(best_dx, best_dy);
            if(actor->move(dest.x, dest.y, dest.z, ACTOR_IGNORE_MOVES | ACTOR_IGNORE_DANGER))
            {
                actor->set_direction(best_dx, best_dy);
                return true;
            }
        }
    }

    // Still not contiguous after pass 2 - will trigger seek mode in Party::follow()
    if(!is_contiguous(p))
        return false;

    return true;
}

/* Follower moves up, down, left, or right. (to intercept a target who moved
 * diagonally) Returns true if the character moved. */
bool PartyPathFinder::try_moving_sideways(uint32 p)
{
    // prefer movement towards target direction
    sint8 rel_x, rel_y;
    get_target_dir(p, rel_x, rel_y);
    if(!move_member(p, rel_x, 0)) // try two directions
        if(!move_member(p, 0, rel_y))
            return false;
    return true;
}

/* Follower tries moving towards the leader, and then towards each adjacent
 * direction if necessary. Returns true if the character moved. */
bool PartyPathFinder::try_moving_to_leader(uint32 p, bool ignore_position)
{
    // move towards leader (allow non-contiguous moves)
    sint8 rel_x, rel_y;
    get_target_dir(p, rel_x, rel_y);
    if(move_member(p, rel_x, rel_y, ignore_position, true, false))
        return true;
    DirFinder::get_adjacent_dir(rel_x, rel_y, -1);
    if(move_member(p, rel_x, rel_y, ignore_position, true, false))
        return true;
    DirFinder::get_adjacent_dir(rel_x, rel_y, 2);
    if(move_member(p, rel_x, rel_y, ignore_position, true, false))
        return true;
    return false;
}

/* Try moving in a forward direction. (direction leader moved) */
bool PartyPathFinder::try_moving_forward(uint32 p)
{
    sint8 vec_x=0, vec_y=0;
    get_forward_dir(vec_x, vec_y);
    if(!move_member(p, vec_x, vec_y))
        return false;
    return true;
}

/* Follower moves in the direction of their target, trying both adjacent
 * directions if necessary.
 * Returns true if character moved, or doesn't need to. Returns false if he stil
 * needs to try to move. */
bool PartyPathFinder::try_moving_to_target(uint32 p, bool avoid_damage_tiles)
{
    sint8 rel_x, rel_y;
    get_target_dir(p, rel_x, rel_y);
    if(!move_member(p, rel_x, rel_y, false, false)) // don't ignore position, don't bump other followers
    {
      sint8 leader = get_leader();
      if(leader >= 0)
      {
        // try both adjacent directions, first the one which is
        // perpendicular to the leader's facing direction
        uint8 ldir = get_member(leader).actor->get_direction();
        sint8 dx = (ldir==NUVIE_DIR_W)?-1:(ldir==NUVIE_DIR_E)?1:0;
        sint8 dy = (ldir==NUVIE_DIR_N)?-1:(ldir==NUVIE_DIR_S)?1:0;
        sint8 relx2 = rel_x, rely2 = rel_y; // adjacent directions, counter-clockwise
        sint8 relx3 = rel_x, rely3 = rel_y; // clockwise
        DirFinder::get_adjacent_dir(relx2, rely2, -1);
        DirFinder::get_adjacent_dir(relx3, rely3, 1);
        if(!(abs(relx2) == abs(dy) && abs(rely2) == abs(dx)))
        {
            // first isn't perpendicular; swap directions
            DirFinder::get_adjacent_dir(relx2, rely2, 2); // becomes clockwise
            DirFinder::get_adjacent_dir(relx3, rely3, -2); // counter-clockwise
        }
        if(!move_member(p, relx2, rely2))
            if(!move_member(p, relx3, rely3))
            {
                // this makes Iolo (follower 3) try to move around other party
                // members when leader changes direction and they
                // block him
                return false;
            }
      }
    }
    return true;
}

/* Follower p will try moving in every direction, first towards the leader, and
 * then in a circular order starting with the direction closest to target_loc.
 * Returns true if character moved. */
bool PartyPathFinder::try_all_directions(uint32 p, MapCoord target_loc)
{
    MapCoord leader_loc = party->get_leader_location();
    MapCoord member_loc = party->get_location(p);
    sint8 to_leader_x = get_wrapped_rel_dir(leader_loc.x,member_loc.x, leader_loc.z);
    sint8 to_leader_y = get_wrapped_rel_dir(leader_loc.y,member_loc.y, leader_loc.z);
    // rotate direction, towards target
    sint8 rot = DirFinder::get_turn_towards_dir(to_leader_x,to_leader_y,
                                              sint8(target_loc.x-member_loc.x),
                                              sint8(target_loc.y-member_loc.y));
    if(rot == 0) rot = 1; // default clockwise

    // check all directions, first only those adjacent to the real target
    MapCoord real_target = party->get_formation_coords(p);
    for(uint32 dir=0; dir<8; dir++)
    {
        MapCoord dest = member_loc.abs_coords(to_leader_x, to_leader_y);
        if(dest.distance(real_target) == 1 && move_member(p, to_leader_x, to_leader_y))
            return true;
        DirFinder::get_adjacent_dir(to_leader_x, to_leader_y, rot);
    }
    // this time, don't allow any moves that take us further from the leader
    // than our target position is (unless we're already that far away)
    for(uint32 dir=0; dir<8; dir++)
    {
        MapCoord dest = member_loc.abs_coords(to_leader_x, to_leader_y);
        if((dest.distance(leader_loc) <= real_target.distance(leader_loc)
            || dest.distance(leader_loc) <= member_loc.distance(leader_loc))
           && move_member(p, to_leader_x, to_leader_y))
            return true;
        DirFinder::get_adjacent_dir(to_leader_x, to_leader_y, rot);
    }
    // now try any move possible (don't bother if already contiguous)
    if(!is_contiguous(p))
        for(uint32 dir=0; dir<8; dir++)
        {
            MapCoord dest = member_loc.abs_coords(to_leader_x, to_leader_y);
            if(move_member(p, to_leader_x, to_leader_y))
                return true;
            DirFinder::get_adjacent_dir(to_leader_x, to_leader_y, rot);
        }
    return false;
}

/* Returns a list(vector) of all locations adjacent to 'center', sorted by their
 * distance to 'target'. (near to far)
 */
vector<MapCoord>
PartyPathFinder::get_neighbor_tiles(MapCoord &center, MapCoord &target)
{
    sint8 rel_x = get_wrapped_rel_dir(target.x,center.x,target.z);
    sint8 rel_y = get_wrapped_rel_dir(target.y,center.y,target.z);
    vector<MapCoord> neighbors;
    for(uint32 dir=0; dir<8; dir++)
    {
        MapCoord this_square = center.abs_coords(rel_x, rel_y); // initial square in first iteration
        vector<MapCoord>::iterator i = neighbors.begin();
        uint32 sorted=0;
        for(; sorted<neighbors.size(); sorted++,i++)
        {
            MapCoord check_square = neighbors[sorted];
            if(target.distance(this_square) < target.distance(check_square)
               && !party->is_anyone_at(check_square)) // exclude squares with any other party member from being at the front of the list
            {
                neighbors.insert(i, this_square);
                break;
            }
        }
        if(sorted == neighbors.size()) // place before end of the list
            neighbors.insert(neighbors.end(), this_square);
        DirFinder::get_adjacent_dir(rel_x, rel_y, 1);
    }
    return neighbors;
}

/* This is like Actor::push_actor and exchanges positions with an actor, or
 * moves them to a neighboring location. This method doesn't move the original
 * "bumper" actor. When exchanging positions, the bumped character loses their
 * turn.
 * Characters can only be "bumped" within one move of their ORIGINAL square. It
 * is illegal to bump someone into a CRITICAL or IMPOSSIBLE state.
 * Returns true if the party member moved successfully.
 */
bool PartyPathFinder::bump_member(uint32 bumped_member_num, uint32 member_num)
{
    if(member_num >= party->get_party_size())
        return false;
    Actor *actor = get_member(bumped_member_num).actor;
    if(actor->is_immobile())
    	return false;
    Actor *push_actor = get_member(member_num).actor;
    MapCoord bump_from = party->get_location(bumped_member_num);
    MapCoord bump_target = party->get_formation_coords(bumped_member_num); // initial direction
    MapCoord member_loc = party->get_location(member_num);
    sint8 to_member_x = get_wrapped_rel_dir(member_loc.x,bump_from.x,member_loc.z); // to push_actor
    sint8 to_member_y = get_wrapped_rel_dir(member_loc.y,bump_from.y,member_loc.z);

    // sort neighboring squares by distance to target (closest first)
    vector<MapCoord> neighbors;
    if(bump_target != bump_from)
        neighbors = get_neighbor_tiles(bump_from, bump_target);
    else // sort by distance to leader
    {
        MapCoord leader_loc = party->get_leader_location();
        neighbors = get_neighbor_tiles(bump_from, leader_loc);
    }

    for(uint32 dir=0; dir<8; dir++)
    {
        sint8 rel_x = get_wrapped_rel_dir(neighbors[dir].x,bump_from.x, bump_from.z);
        sint8 rel_y = get_wrapped_rel_dir(neighbors[dir].y,bump_from.y, bump_from.z);
        // Since this direction is blocked, it will only be at the end of the
        // sorted list.
        if(rel_x == to_member_x && rel_y == to_member_y)
        {
            // Use special push() that ignores actors, and reduces moves left.
            actor->push(push_actor, ACTOR_PUSH_HERE);
            return true;
        }
        else if(move_member(bumped_member_num, rel_x, rel_y))
        {
            // Reduce moves left so actor can't move (or get pushed) again.
            actor->set_moves_left(0);
            return true;
        }
    }
    return false;
}

/* "Try a move", only if target is contiguous. */
bool PartyPathFinder::move_member(uint32 member_num, sint16 relx, sint16 rely, bool ignore_position, bool can_bump, bool avoid_danger_tiles)
{
    /**Do not call with relx and rely set to 0.**/
    if(relx == 0 && rely == 0)
        return true;
    MapCoord member_loc = party->get_location(member_num);
    MapCoord target(member_loc);
    target = member_loc.abs_coords(relx, rely);
    Actor *actor = get_member(member_num).actor;
    ActorMoveFlags flags = ACTOR_IGNORE_MOVES;
    if(!avoid_danger_tiles)
    	flags = flags | ACTOR_IGNORE_DANGER;

    if(is_contiguous(member_num, target) || ignore_position)
    {
        if(actor->move(target.x, target.y, target.z, flags))
        {
            actor->set_direction(relx, rely);
            return true;
        }
        if(actor->get_error()->err == ACTOR_BLOCKED_BY_ACTOR)
        {
            Actor *blocking_actor = actor->get_error()->blocking_actor;
            sint8 blocking_member_num = -1;
            if(blocking_actor)
                blocking_member_num = party->get_member_num(blocking_actor);
            if(blocking_member_num < sint32(member_num))
                return false; // blocked by an actor not in the party
            if(bump_member(uint32(blocking_member_num), member_num)
               && actor->move(target.x, target.y, target.z, flags|ACTOR_IGNORE_MOVES))
            {
                actor->set_direction(relx, rely);
                return true;
            }
        }
    }
    return false; // target is not contiguous, or move is blocked
}

/* Use a better pathfinder to search for the leader. */
void PartyPathFinder::seek_leader(uint32 p)
{
    Actor *actor = get_member(p).actor;
    MapCoord leader_loc = party->get_leader_location();
    ActorPathFinder *df = actor->get_pathfinder();
    if(!df)
    {
        df = new ActorPathFinder(actor, leader_loc);
        actor->set_pathfinder(df, new SeekPath);
    }
    else if(leader_moved()) // update target
        df->set_goal(leader_loc);
}

void PartyPathFinder::end_seek(uint32 p)
{
    get_member(p).actor->delete_pathfinder();
}
