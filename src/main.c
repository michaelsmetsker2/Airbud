/**
* @file init.c
 *
 * contains implementations for all initialization functions related to ffmpeg and sld3
 *
 * @author Michael Metsker
 * @version 1.0
 */

#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL_main.h>

#include "../include/common.h"
#include "../include/init.h"
#include "../include/playback.h"
#include "../include/frame-queue.h"

/* runs on startup */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

    *appstate = initialize();
    if (*appstate == NULL) {
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE; /* carry on with the program!*/
}

//TODO THIS SHOULD NOT BE GLOBAL fix immedietly when done debugging
struct frame *current_frame; //TODO make last between calls so current frame can be repeated if no new frame

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
    app_state *state = (app_state *) appstate;

    //TODO this should really be a render function in the playback file

    SDL_LockMutex(state->queue->mutex);
    if (state->queue->size > 0) {

        // frame in the queue

        struct frame *frameBuffer = dequeue_frame(state->queue);
        if (!frameBuffer) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error receiving frame\n");
        } else {
            current_frame = frameBuffer;
        }
    }
    SDL_UnlockMutex(state->queue->mutex);


    //int64_t pts = pFrame->best_effort_timestamp;
    //printf("Decoded frame: %dx%d, pts=%lld\n", frame->width, frame->height, pts);

    if (current_frame == NULL) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "frame is null\n");
        return SDL_APP_CONTINUE;
    }

    SDL_Texture *texture = SDL_CreateTexture(state->renderer,
    SDL_PIXELFORMAT_IYUV,   // Equivalent to YUV420 planar
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH,
    SCREEN_HEIGHT);

    SDL_UpdateYUVTexture(texture,
        NULL,               // entire texture
        current_frame->video_frame->data[0], current_frame->video_frame->linesize[0],   // Y plane
        current_frame->video_frame->data[1], current_frame->video_frame->linesize[1],   // U plane
        current_frame->video_frame->data[2], current_frame->video_frame->linesize[2]);  // V plane

    SDL_RenderClear(state->renderer);
    SDL_RenderTexture(state->renderer, texture, NULL, NULL);  // whole texture to window
    SDL_RenderPresent(state->renderer);

    //SDL_Delay(16);
    SDL_DestroyTexture(texture);


    //TODO menu layer/buttons

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc.) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}


/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL will clean up the window/renderer for us. */
    //TODO clean up framequeue
}
