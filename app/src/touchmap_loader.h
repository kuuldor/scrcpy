#ifndef SC_TOUCHMAP_LOADER_H
#define SC_TOUCHMAP_LOADER_H

#include "common.h"

#include <stdbool.h>
#include <stddef.h>

struct sc_touchmap_loader_entry {
    char *package_name;
    char *path;
};

struct sc_touchmap_loader {
    bool enabled;
    char *touchmap_dir;
    struct sc_touchmap_loader_entry *index_entries;
    size_t index_count;
};

bool
sc_touchmap_loader_init(struct sc_touchmap_loader *tml,
                        const char *touchmap_dir);

void
sc_touchmap_loader_destroy(struct sc_touchmap_loader *tml);

bool
sc_touchmap_loader_rebuild_index(struct sc_touchmap_loader *tml);

const char *
sc_touchmap_loader_find_path(const struct sc_touchmap_loader *tml,
                             const char *package_name);

#endif
