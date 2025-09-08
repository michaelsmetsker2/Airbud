/**
 * @file game_states.h
 *
 * Hardcoded games states and their buttons
 *
 * @author Michael Metsker
 * @version 1.0
 */

#ifndef GAME_STATES_H
#define GAME_STATES_H

#include <stdint.h>
#include <stdbool.h>

#define STATE_COUNT 4 //FIXME, this polutes the namespace?

// forward declaration of game_data, definition is in game_logic.h
struct game_data;

/**
 * @enum STATE_ID
 * @brief names that corrospond to positions in the GAME_STATES array of game_states
 */
typedef enum STATE_ID {
    MAIN_MENU_1,
    MAIN_MENU_2,
    MAIN_MENU_3,
    TUTORIAL
} STATE_ID;

/**
 * @typedef next_state_func
 * @brief Points to a function that determins what game state to go to next and updates game data when necessary
 *
 * @param data game data that can influence the outcome of where to go next
 * @return STATE_ID enum of the next state to go to
 */
typedef STATE_ID (*next_state_func)(struct game_data *data);

/**
 * @typedef button
 * @brief struct containing the dimenstions of a UI button, and what happens when it is clicked
 */
typedef struct button {
    const int32_t x;                /**< the top of the buttons offset relative to the top of the window */
    const int32_t y;                /**< the left of the buttons offset relative to the left of the window */
    const int32_t height;           /**< height of the button in pixels*/
    const int32_t width;            /**< width of the button in pixels */

    const next_state_func on_click; /**< function to be triggered when the button is selected, returns the next state to go to */

} button;

/**
 * @struct game_state
 * @brief all necessary information pertaining to a decodable section of the vob file and its position in the game
 */
struct game_state {
    const uint32_t start_offset_bytes; /** the offset where decoding should start */
    const uint32_t end_offset_bytes;   /** the offset where decoding should end*/
    const bool audio_only;             /** if the corosponding section of the file only has audio information */

    const next_state_func next_state;  /** a function that returns the id of the next gamestate and updates the passed gamedata accordingly */

    const button *buttons;             /** array of buttons that corrospond to the section being decoded*/
    const uint8_t buttons_count;       /** the size of the buttons array */
};

// Array of all game states, index aligns with the STATE_ID enum
extern const struct game_state GAME_STATES[STATE_COUNT];

#endif //GAME_STATES_H
