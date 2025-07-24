/**
 * @file decode.h
 *
 * Functions relating to displaying video and playing audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#ifndef RENDER_H
#define RENDER_H

#include <common.h>
#include <init.h>

/**
 * @brief
 *
 * @param state app_state state that contains window, renderer and queue for rendering
 * @return true on success false otherwise
 */
bool render_frame(const app_state *state);




#endif //RENDER_H
