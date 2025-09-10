/**
* @file game_logic.c
 *
 * Logic for gameplay, buttons and switching between game states
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <game_logic.h>
#include <game_states.h>

// PRE COMMANDS


// POST COMMANDS

STATE_ID next_MAIN_MENU_1(struct game_data *data) { return MAIN_MENU_2; }
STATE_ID next_MAIN_MENU_2(struct game_data *data) { return MAIN_MENU_3; }
STATE_ID next_MAIN_MENU_3(struct game_data *data) { return MAIN_MENU_1; }