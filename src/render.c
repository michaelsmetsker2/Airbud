/**
 * @file decode.h
 *
 * functions for rendering thread and syncing rendering to the audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#include <SDL3/SDL.h>
#include <libavutil/frame.h>

#include <frame_queue.h>
#include <render.h>
#include <init.h>

#define TIMEOUT_DELAY_MS 125
#define SAMPLE_RATE 48000.0
#define NUM_CHANNELS 2 // stereo
#define BYTES_PER_SAMPLE 2 // 16 bit

#define AUDEO_LATENCY_MS 200 //bit confused on this one, seems to line up good though
#define LAG_TOLERANCE_MS 20.0 // one small amount of time behind shouldn't drop the frame

//Time base of the media format context, hardcoded as only this dvd is needed
static const AVRational time_base = {1, 90000};

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
    args->total_audio_samples = &appstate->total_audio_samples;
    args->audio_stream = appstate->audio_stream;

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
static bool render_loop(const struct render_thread_args *args) {
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
        return false;
    }

    // sync playback time to the audio

    const uint32_t queued_samples = SDL_GetAudioStreamQueued(args->audio_stream) / (NUM_CHANNELS * BYTES_PER_SAMPLE);
    const uint32_t played_audio_samples = SDL_GetAtomicU32(args->total_audio_samples) - queued_samples;

    // Seconds to MS devided by sample rate plus the hardware delay
    const double played_ms = played_audio_samples * 1000.0 / SAMPLE_RATE + AUDEO_LATENCY_MS;

    const double pts_ms = (double)current_frame->best_effort_timestamp * av_q2d(time_base) * 1000.0;

    if (pts_ms > played_ms) {
        const double delay_ms = pts_ms - played_ms;
        SDL_Delay((uint32_t)delay_ms);
    } else if (played_ms - pts_ms > LAG_TOLERANCE_MS) {
        SDL_Log("dropping frame");
        av_frame_free(&current_frame);
        return true;
    }

    // render the frame
    SDL_UpdateYUVTexture(args->texture, NULL,
        current_frame->data[0], current_frame->linesize[0],   // Y plane
        current_frame->data[1], current_frame->linesize[1],   // U plane
        current_frame->data[2], current_frame->linesize[2]);  // V plane

    SDL_RenderClear(args->renderer);
    SDL_RenderTexture(args->renderer, args->texture, NULL, NULL);  // whole texture to window
    SDL_RenderPresent(args->renderer);

    SDL_FlushRenderer(args->renderer);
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


