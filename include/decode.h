/**
 * Decoding functions for audio and video
 *
 * @author Michael Metkser
 * @version 1.0
 */

#ifndef DECODE_H
#define DECODE_H

#include <libavcodec/packet.h>
#include <libavutil/frame.h>

#include "../include/common.h"



/**
 * Decodes a video packet and queues and queues the resulting frames if any.
 *
 * @param frame
 */
void decode_video(AVPacket *packet, volatile bool* exit_flag);



#endif //DECODE_H
