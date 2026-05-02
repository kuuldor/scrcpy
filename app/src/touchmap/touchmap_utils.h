#ifndef SC_TOUCHMAP_UTILS_H
#define SC_TOUCHMAP_UTILS_H

#include "common.h"

#include <SDL2/SDL.h>

#include "util/log.h"

static inline void
sc_touchmap_start_thread(const char *name, SDL_ThreadFunction fn, void *data) {
    SDL_Thread *thread = SDL_CreateThread(fn, name, data);
    if (!thread) {
        LOGE("Failed to create thread: %s", SDL_GetError());
    } else {
        SDL_DetachThread(thread);
    }
}

#endif
