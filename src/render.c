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
    SDL_LockMutex(state->video_queue->mutex);

    //FIXME make own thread
    /* FIXME doing it like this every frame instead of when the queue is updated may be the problem with timing
       FIXME and if it isnt then the onEmpty may be gotten rid of */
    /* queue empty */
    if (state->video_queue->size == 0) {
        //SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "queue empty");
        SDL_UnlockMutex(state->video_queue->mutex);
        return false;
    }

    AVFrame *current_frame = dequeue_frame(state->video_queue);
    SDL_UnlockMutex(state->video_queue->mutex);

    if (!current_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error dequeueing frame");
        return false;
    }

    uint32_t audio_time = SDL_GetAtomicU32(&state->audio_playback_time);
    AVRational time_base = {1, 90000};
    double frame_time_ms = current_frame->pts * (av_q2d(time_base) * 1000);

    if (frame_time_ms < audio_time) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "dropping slow frame");
        return false;
    }

    uint32_t wait_time = frame_time_ms - audio_time;
    SDL_Delay(wait_time);




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


