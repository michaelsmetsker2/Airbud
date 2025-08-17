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
#include <stdint.h>

#include <frame_queue.h>
#include <render.h>
#include <init.h>

#define TIMEOUT_DELAY_MS 125
#define NUM_CHANNELS 2 // stereo
#define BYTES_PER_SAMPLE 2 // 16 bit

#define SAMPLES_TO_MS  (1000.0 / 48000) //sample rate is 48000 / second
#define PTS_TO_MS      (1000.0 / 90000.0) // time base is 1 / 90000

#define AUDEO_LATENCY_MS 200 //hardware latency, 200ms seems to be good, can be adjusted if needed
#define LAG_TOLERANCE_MS 20.0 // won't drop a frame in only a small amount behind

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
    /* waits for queue mutex */
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

    { // syncing audio and video
        const uint32_t queued_samples = SDL_GetAudioStreamQueued(args->audio_stream) / (NUM_CHANNELS * BYTES_PER_SAMPLE);
        const uint32_t played_audio_samples = SDL_GetAtomicU32(args->total_audio_samples) - queued_samples;

        // timestamps of current audio and video frames in ms
        const double audio_time_ms = played_audio_samples * SAMPLES_TO_MS + AUDEO_LATENCY_MS + offset_ms;
        const double video_time_ms = (double)current_frame->best_effort_timestamp * PTS_TO_MS;

        if (video_time_ms > audio_time_ms ) {
            // delay until audio catches up
            SDL_Delay((uint32_t)(video_time_ms - audio_time_ms));
        } else if (audio_time_ms - video_time_ms > LAG_TOLERANCE_MS) {
            SDL_Log("dropping frame");
            av_frame_free(&current_frame);
            return true;
        }

        //FIXME remove debugging lines
        printf("played: %f" "\n", audio_time_ms);
        printf("pts   : %f" "\n", video_time_ms);
    } // syncing audio and video

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

    //TODO cleanup?

    return true;
}