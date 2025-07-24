/**
 * @file decode.h
 * Functions relating to displaying video and playing audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#include <render.h>



bool render_frame(const app_state *state) {

    /* waits for mutex */
    SDL_LockMutex(state->queue->mutex);

    /* queue empty */
    if (state->queue->size == 0) {
        SDL_UnlockMutex(state->queue->mutex);
        return false;
    }

    struct frame *current_frame = dequeue_frame(state->queue);
    SDL_UnlockMutex(state->queue->mutex);

    if (!current_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error receiving frame");
        destroy_frame(current_frame);
        return false;
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


    return true;
}


