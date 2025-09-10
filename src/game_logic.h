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

// forward declaration of STATE_ID, definition is in game_states.h
typedef enum STATE_ID STATE_ID;

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

// PRE COMMANDS



// POST COMMANDS

/**
 * @brief The following are all end of decoding functions, they make up the internal game
 * logic that will run at the end of a decoded chunk and determine where the program goes next
 * @param data game_data struct containing all gameplay vars
 * @return the ID of the next state to go to
 */
STATE_ID next_MAIN_MENU_1(struct game_data *data);
STATE_ID next_MAIN_MENU_2(struct game_data *data);
STATE_ID next_MAIN_MENU_3(struct game_data *data);


#endif //GAME_LOGIC_H