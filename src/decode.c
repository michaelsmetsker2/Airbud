/**
 * @file decode.c
 * Decodes video and audio packets
 *
 * @author Michael Metsker
 * @version 1.0
 */

#include <SDL3/SDL.h>

#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libswresample/swresample.h>

#include <decode.h>
#include <frame_queue.h>


static const Sint32 TIMEOUT_DELAY_MS = 200;

static const int SAMPLE_RATE = 48000;

void decode_audio(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, SwrContext *resampler,
                  SDL_AtomicInt *exit_flag, SDL_AudioStream *stream)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        //SDL_SetAtomicInt(exit_flag, 1); TODO some files start half way through packet??
    }

    //a full frame is ready
    while (!SDL_GetAtomicInt(exit_flag) && avcodec_receive_frame(dec_ctx, frame) == 0) {

        //create resampled frame
        AVFrame *frame_resampled = av_frame_alloc();
        if (!frame_resampled) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't alloc resampled frame");
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }
        //fill new frames values
        av_channel_layout_default(&frame_resampled->ch_layout, 2);
        frame_resampled->format = AV_SAMPLE_FMT_S16;
        frame_resampled->sample_rate = SAMPLE_RATE;
        frame_resampled->nb_samples = frame->nb_samples;
        if (av_frame_get_buffer(frame_resampled, 0) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't allocate audio frame copy buffer");
            av_frame_free(&frame_resampled);
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }

        // convert the frame
        if (swr_convert_frame(resampler, frame_resampled, frame) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't resample audio frame");
            av_frame_free(&frame_resampled);
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }
        // add data to queue
        const int data_size = av_samples_get_buffer_size(NULL, 2, frame_resampled->nb_samples, AV_SAMPLE_FMT_S16, 0);
        if (!SDL_PutAudioStreamData(stream, frame_resampled->data[0], data_size)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't push frame data to audio stream %s", SDL_GetError());
            SDL_SetAtomicInt(exit_flag, 1);
            break;
        }

        /*
        // increment playback_time
        const uint32_t old_val = SDL_GetAtomicU32(args->playback_time);
        const uint32_t new_val = frame->nb_samples * 1000 / SAMPLE_RATE + old_val;
        SDL_Log("audio time: %" PRIu64, new_val);
        SDL_SetAtomicU32(args->playback_time, new_val);
         */

        av_frame_unref(frame);
        av_frame_free(&frame_resampled);
    }
}

void decode_video(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, frame_queue *queue,
                  SDL_AtomicInt *exit_flag)
{
    //decodes packet
    if (avcodec_send_packet(dec_ctx, packet) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "couldn't decode video packet");
        SDL_SetAtomicInt(exit_flag, 1);
    }

    //a full frame is ready
    while (!SDL_GetAtomicInt(exit_flag) && avcodec_receive_frame(dec_ctx, frame) == 0) {

        SDL_LockMutex(queue->mutex); //waits until mutex is unlocked

        //queue is at capacity
        if (queue->size == queue->capacity) {
            //wait for free space
            if (!SDL_WaitConditionTimeout(queue->not_full, queue->mutex, TIMEOUT_DELAY_MS)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "waiting for video queue to empty timed out\n");
                SDL_SetAtomicInt(exit_flag, 1);
                break;
            }
        }

        if (!enqueue_frame(queue, frame)) {
            SDL_SetAtomicInt(exit_flag, 1);
            SDL_UnlockMutex(queue->mutex);
            break;
        }

        av_frame_unref(frame);
        SDL_UnlockMutex(queue->mutex);
    }
}