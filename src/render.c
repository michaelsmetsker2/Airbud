/**
 * @file decode.h
 * Functions relating to displaying video and playing audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <libavutil/frame.h>

#include <frame_queue.h>
#include <init.h>

bool render_frame(app_state *state) {

    /* waits for mutex */
    SDL_LockMutex(state->render_queue->mutex);

    //FIXME make own thread
    /* FIXME doing it like this every frame instead of when the queue is updated may be the problem with timing
       FIXME and if it isnt then the onEmpty may be gotten rid of */
    /* queue empty */
    if (state->render_queue->size == 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "queue empty");
        SDL_UnlockMutex(state->render_queue->mutex);
        return false;
    }

    AVFrame *current_frame = dequeue_frame(state->render_queue);
    SDL_UnlockMutex(state->render_queue->mutex);

    if (!current_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error dequeueing frame");
        return false;
    }

    //uint32_t audio_time_ms = SDL_GetAtomicU32(&state->audio_playback_time) * 1000 / 48000;

    uint64_t frame_time = current_frame->best_effort_timestamp;
    uint64_t audio_time = SDL_GetAtomicU32(&state->audio_playback_time);

    //SDL_Log("frame time: %" PRIu64, frame_time);
    //SDL_Log("audio time: %" PRIu64, audio_time);

    /*
    if (frame_time < audio_time) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "dropping slow frame");
        return false;
    }

    uint64_t wait_time = frame_time - audio_time;
    SDL_Delay(wait_time * 1000 / 48000);
    */

    SDL_Delay(32);

    SDL_UpdateYUVTexture(state->base_texture, NULL,
        current_frame->data[0], current_frame->linesize[0],   // Y plane
        current_frame->data[1], current_frame->linesize[1],   // U plane
        current_frame->data[2], current_frame->linesize[2]);  // V plane

    SDL_RenderClear(state->renderer);
    SDL_RenderTexture(state->renderer, state->base_texture, NULL, NULL);  // whole texture to window
    SDL_RenderPresent(state->renderer);

    SDL_FlushRenderer(state->renderer);
    av_frame_unref(current_frame);


    av_frame_free(&current_frame);

    return true;
}


