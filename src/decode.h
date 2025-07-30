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
#include <stdbool.h>

/**
 * Decodes a video packet and queues and queues the resulting frames if any.
 *
 * @param dec_ctx deCodec to decode packet
 * @param packet Incoming packet data to be parsed
 * @param frame Reusable AVFrame, can be half filled if one packet isn't enough
 * @param queue Queue to add frames to
 * @param exit_flag Exit flag for early exit
 */
void decode_video(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, frame_queue *queue,
                  volatile bool *exit_flag);

/**
 * Decodes an audio packet and queues and queues the resulting frames if any.
 *
 * @param dec_ctx deCodec to decode packet
 * @param packet Incoming packet data to be parsed
 * @param frame Reusable AVFrame, can be half filled if one packet isn't enough
 * @param resampler resampler context for changing audio to an SDL3 playable format
 * @param queue Queue to add frames to
 * @param exit_flag Exit flag for early exit
 */
void decode_audio(AVCodecContext *dec_ctx, const AVPacket *packet, AVFrame *frame, SwrContext *resampler, frame_queue *queue,
                  volatile bool *exit_flag);

#endif //DECODE_H
