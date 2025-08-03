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
#include <render.h>
#include <init.h>

static const Sint8 TIMEOUT_DELAY_MS = 125;

bool create_render_thread(app_state *appstate) {

    // creates and populates args
    struct render_thread_args *args = malloc(sizeof(struct render_thread_args));
    if (!args) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate args for render thread\n");
        return false;
    }

    args->exit_flag = &appstate->stop_render_thread;
    args->renderer = appstate->renderer;
    args->window = appstate->window;
    args->texture = appstate->base_texture;
    args->queue = appstate->render_queue;
    args->audio_playback_time = &appstate->audio_playback_time;

    //starts decoder thread
    appstate->decoder_thread = SDL_CreateThread(render_frames, "decoder", args);
    if (!appstate->decoder_thread) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate render thread\n");
        return false;
    }

    return true;
}

/**
 * @brief main render loop
 * @param args all nesesary information in a render_thread_args struct
 * @return true on success, false otherwise
 */
bool render_loop(const struct render_thread_args *args) {
    /* waits for mutex */
    SDL_LockMutex(args->queue->mutex);

    if (args->queue->size == 0) {
        if (!SDL_WaitConditionTimeout(args->queue->not_empty, args->queue->mutex, TIMEOUT_DELAY_MS)) {
            //timeout, it is not abnormal for no frames to be in the queue, does not set exit thread
            SDL_UnlockMutex(args->queue->mutex);
            return false;
        }
    }

    AVFrame *current_frame = dequeue_frame(args->queue);
    SDL_UnlockMutex(args->queue->mutex);

    if (!current_frame) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "error dequeueing frame");
        av_frame_free(&current_frame);
        return false;
    }

    //uint32_t audio_time_ms = SDL_GetAtomicU32(&state->audio_playback_time) * 1000 / 48000;

    const uint64_t frame_time = current_frame->best_effort_timestamp;
    const uint64_t audio_time = SDL_GetAtomicU32(args->audio_playback_time);

    SDL_Log("frame time: %" PRIu64, frame_time);
    SDL_Log("audio time: %" PRIu64, audio_time);

    /*
    if (frame_time < audio_time) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "dropping slow frame");
        return false;
    }

    uint64_t wait_time = frame_time - audio_time;
    SDL_Delay(wait_time * 1000 / 48000);
    */

    SDL_Delay(32);

    SDL_UpdateYUVTexture(args->texture, NULL,
        current_frame->data[0], current_frame->linesize[0],   // Y plane
        current_frame->data[1], current_frame->linesize[1],   // U plane
        current_frame->data[2], current_frame->linesize[2]);  // V plane

    SDL_RenderClear(args->renderer);
    SDL_RenderTexture(args->renderer, args->texture, NULL, NULL);  // whole texture to window
    SDL_RenderPresent(args->renderer);

    SDL_FlushRenderer(args->renderer);


    av_frame_unref(current_frame);
    av_frame_free(&current_frame);
    return true;

}

int render_frames(void *data) {
    const struct render_thread_args *args = (struct render_thread_args *) data;

    while (!SDL_GetAtomicInt(args->exit_flag)) {

        render_loop(args);
    }

    SDL_Log("test");
    // TODO cleanup
    return true;
}


