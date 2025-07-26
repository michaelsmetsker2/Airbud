/**
 * @file decode.h
 * Functions relating to displaying video and playing audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <libavutil/frame.h>

#include <frame-queue.h>
#include <init.h>

bool render_frame(const app_state *state) {

    /* waits for mutex */
    SDL_LockMutex(state->queue->mutex);

    /* queue empty */
    if (state->queue->size == 0) {
        SDL_UnlockMutex(state->queue->mutex);
        return false;
    }

    AVFrame *current_frame = dequeue_frame(state->queue);
    SDL_UnlockMutex(state->queue->mutex);

    if (!current_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error dequeueing frame");
        return false;
    }

    SDL_UpdateYUVTexture(state->base_texture, NULL,
        current_frame->data[0], current_frame->linesize[0],   // Y plane
        current_frame->data[1], current_frame->linesize[1],   // U plane
        current_frame->data[2], current_frame->linesize[2]);  // V plane

    SDL_RenderClear(state->renderer);
    SDL_RenderTexture(state->renderer, state->base_texture, NULL, NULL);  // whole texture to window
    SDL_RenderPresent(state->renderer);

    SDL_Delay(1); //TODO temporary debug, lets gpu finish rendering the texture befure freeing AVFrame
    av_frame_free(&current_frame);

    return true;
}


