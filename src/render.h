/**
 * @file render.h
 *
 * Functions relating to displaying video and syncing it to audio
 *
 * @author Michael Metkser
 * @version 1.0
 */

#ifndef RENDER_H
#define RENDER_H

#include <init.h>
#include <stdbool.h>

/**
 * @brief creates render thread and starts it off with the correct parameters
 * this populates the passed appstates, render_thread, and stop_render_thread members
 * this does not have ownership over any variables and therefore needs no cleanup
 * @return true on success, false otherwise
 */
bool create_render_thread(app_state *appstate);

/**
 * @brief thread that manages rendering
 * //TODO update for render layers
 *
 * @param data pointer to a render_thread_args struct
 * @return 0 on clean shutdown
 */
 int render_frames(void *data);

#endif //RENDER_H
