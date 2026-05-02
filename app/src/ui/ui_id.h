#ifndef SC_UI_ID_H
#define SC_UI_ID_H

#include "common.h"

#include <stdint.h>

#include "ui_types.h"

static inline sc_ui_id
sc_ui_id_from_u32(const void *scope, uint32_t key) {
    uintptr_t value = (uintptr_t) scope;
    uint64_t mixed = ((uint64_t) value << 32) ^ key ^ UINT64_C(0x9E3779B97F4A7C15);
    mixed ^= mixed >> 30;
    mixed *= UINT64_C(0xBF58476D1CE4E5B9);
    mixed ^= mixed >> 27;
    mixed *= UINT64_C(0x94D049BB133111EB);
    mixed ^= mixed >> 31;

    if (mixed == SC_UI_ID_INVALID) {
        return UINT64_C(1);
    }

    return mixed;
}

#endif
