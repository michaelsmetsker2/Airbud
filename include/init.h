/**
 * @file init.h
 *
 * contains basic initialization functions for SLD3
 * and appstate struct to carry data between sdl functions
 *
 * @author Michael Metsker
 * @version 1.0
*/

#ifndef INIT_H
#define INIT_H

#include "common.h"
#include "../include/frame-queue.h"

/**
 * Initialization for SDL3
 *
 * @param window ** to main window
 * @param renderer ** to main window renderer
 * @return bool true on success, false on failure
*/
bool sdl_init(SDL_Window **window, SDL_Renderer **renderer);

/**
 * @struct Appstate
 *
 * Struct for carrying basic info to all parts of the SDL program
 */
typedef struct {
 FrameQueue *queue;             /*< render queue of buffered frames */
 volatile bool *stopBuffering;  /*< The exit flag for the decoding thread */
 //TODO gamestate??? potentially idk
} Appstate;

#endif //INIT_H
