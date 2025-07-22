/**
 * @file init.c
 *
 * contains implementations for all initialization functions related to ffmpeg and sld3
 * TODO depreciate file?
 * @author Michael Metsker
 * @version 1.0
*/

#include "../include/init.h"
#include "../include/common.h"

bool sdl_init(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_SetAppMetadata("airbud", "1.0", "com.airbud.renderer");

    // TODO initialize more than just video
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return false;
    }

    if (!SDL_CreateWindowAndRenderer("airbud/renderer", SCREEN_WIDTH, SCREEN_HEIGHT, 0, window, renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return false;
    }

    return true;
}