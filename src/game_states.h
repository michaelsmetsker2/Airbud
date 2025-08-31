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

/**
 * TODO
 *
 *
 */
typedef struct button {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    void (*on_click)();

} button;

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
 * @brief Function pointer determining the next gamestate to go to after the decoder has reached the end of playback.
 *
 * @param //TODO
 * @return STATE_ID enum of the next state to go to
 */
typedef STATE_ID (*next_state_func)(void *args); //FIXME fix args

/**
 * @struct game_state
 * @brief all necessary information pertaining to a decodable section of the vob file
 */
struct game_state {
    const uint32_t start_offset_bytes; /** the offset where decoding should start */
    const uint32_t end_offset_bytes;   /** the offset where decoding should end*/
    const bool audio_only;             /** if the corosponding section of the file only has audio information */

    const next_state_func next_state; /** a function that returns the id of the next gamestate */ //TODO this will bneed to be passed some data

    const button *buttons;             /** TODO*/
    const uint8_t buttons_count;       /** TODO*/
};

// Array of all game states, index aligns with the STATE_ID enum
extern const struct game_state GAME_STATES[STATE_COUNT];

#endif //GAME_STATES_H
