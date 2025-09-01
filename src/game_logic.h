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

/**
 * @struct game_data
 * @brief simple struct containing all game specific data
 */
struct game_data {
    bool        league;  /** difficuly, false for little league, true for major league */
    uint16_t    seed;    /** TODO */

    uint8_t     strikes;
    uint8_t     outs;
    uint8_t     runs;
};

/**
 * @brief cleanly updates the gamestate
 * makes sure all threads are aware of the change, should only be called from main thread
 *
 * @param appstate basic information struct from main thread
 * @param destination gamestate to change to
 * @return true on success, false otherwise
 */
bool change_game_state(app_state *appstate, STATE_ID destination);

#endif //GAME_LOGIC_H