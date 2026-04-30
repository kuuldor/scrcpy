#ifndef SC_TOUCHMAP_RUNTIME_H
#define SC_TOUCHMAP_RUNTIME_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#include <SDL2/SDL_events.h>

struct sc_input_manager;

bool
sc_touchmap_runtime_handle_event(struct sc_input_manager *im,
                                 const SDL_Event *event);

#endif
