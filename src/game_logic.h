/**
* @file game_logic.h
 *
 * Logic for gameplay, buttons and switching between game states
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdbool.h>
#include <init.h>
#include <game_states.h>

/**
 * @brief cleanly updates the gamestate
 * makes sure all threads are aware of the change, should only be called from main thread
 * @param appstate basic information struct from main thread
 * @param destination gamestate to change to
 * @return true on success, false otherwise
 */
bool change_game_state(app_state *appstate, STATE_ID destination);

#endif //GAME_LOGIC_H