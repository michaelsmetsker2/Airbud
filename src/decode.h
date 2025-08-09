/**
 * @file decode.h
 * Decoding functions for audio and video
 *
 * @author Michael Metkser
 * @version 1.0
 */

#ifndef DECODE_H
#define DECODE_H

#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>

#include <frame_queue.h>

/**
 * Decodes an audio packet and queues and queues the resulting frames if any.
 *
 * @param dec_ctx deCodec to decode packet
 * @param packet Incoming packet data to be parsed
 * @param frame Reusable AVFrame, can be half filled if one packet isn't enough
 * @param resampler resampler context for changing audio to an SDL3 playable format
 * @param stream audio stream to push packet data to
 * @param total_audio_samples total amount of samples pushed to the audio queue, used to sync with renderer
 * @return true on success false on error
 */
bool decode_audio(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, SwrContext *resampler,
                  SDL_AudioStream *stream, SDL_AtomicU32 *total_audio_samples);

/**
 * Decodes a video packet and queues and queues the resulting frames if any.
 *
 * @param dec_ctx deCodec to decode packet
 * @param packet Incoming packet data to be parsed
 * @param frame Reusable AVFrame, can be half filled if one packet isn't enough
 * @param queue Queue to add frames to
 * @return true on success false on error
 */
bool decode_video(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, frame_queue *queue);

#endif //DECODE_H
