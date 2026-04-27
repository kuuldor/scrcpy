#include "touchmap_loader.h"

#include <dirent.h>
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "touchmap.h"
#include "util/file.h"
#include "util/log.h"

static void
sc_touchmap_loader_clear_index(struct sc_touchmap_loader *tml) {
    for (size_t i = 0; i < tml->index_count; ++i) {
        SDL_free(tml->index_entries[i].package_name);
        SDL_free(tml->index_entries[i].path);
    }

    SDL_free(tml->index_entries);
    tml->index_entries = NULL;
    tml->index_count = 0;
}

static bool
sc_touchmap_loader_has_json_suffix(const char *filename) {
    size_t len = strlen(filename);
    return len > 5 && !SDL_strcasecmp(filename + len - 5, ".json");
}

static bool
sc_touchmap_loader_join_path(const char *dir, const char *name, char **out) {
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    bool need_sep = dir_len > 0 && dir[dir_len - 1] != SC_PATH_SEPARATOR;
    size_t len = dir_len + (need_sep ? 1 : 0) + name_len + 1;

    char *path = SDL_malloc(len);
    if (!path) {
        LOG_OOM();
        return false;
    }

    memcpy(path, dir, dir_len);
    if (need_sep) {
        path[dir_len++] = SC_PATH_SEPARATOR;
    }
    memcpy(path + dir_len, name, name_len + 1);

    *out = path;
    return true;
}

static bool
sc_touchmap_loader_add_index_entry(struct sc_touchmap_loader *tml,
                                   char *package_name, char *path) {
    struct sc_touchmap_loader_entry *entries = SDL_realloc(
        tml->index_entries, (tml->index_count + 1) * sizeof(*entries));
    if (!entries) {
        LOG_OOM();
        return false;
    }

    tml->index_entries = entries;
    tml->index_entries[tml->index_count].package_name = package_name;
    tml->index_entries[tml->index_count].path = path;
    ++tml->index_count;
    return true;
}

static int
sc_touchmap_loader_compare_entries(const void *a, const void *b) {
    const struct sc_touchmap_loader_entry *ea = a;
    const struct sc_touchmap_loader_entry *eb = b;

    int cmp = SDL_strcmp(ea->package_name, eb->package_name);
    if (cmp) {
        return cmp;
    }

    return SDL_strcmp(ea->path, eb->path);
}

bool
sc_touchmap_loader_init(struct sc_touchmap_loader *tml,
                        const char *touchmap_dir) {
    tml->enabled = false;
    tml->touchmap_dir = NULL;
    tml->index_entries = NULL;
    tml->index_count = 0;

    if (!touchmap_dir || !*touchmap_dir) {
        return true;
    }

    char *absolute_dir = sc_file_get_absolute_path(touchmap_dir);
    if (!absolute_dir) {
        LOGE("Could not resolve touchmap directory: %s", touchmap_dir);
        return false;
    }

    tml->touchmap_dir = SDL_strdup(absolute_dir);
    free(absolute_dir);
    if (!tml->touchmap_dir) {
        LOG_OOM();
        return false;
    }

    tml->enabled = true;
    LOGI("Touchmap auto-load directory: %s", tml->touchmap_dir);
    return true;
}

void
sc_touchmap_loader_destroy(struct sc_touchmap_loader *tml) {
    if (!tml) {
        return;
    }

    sc_touchmap_loader_clear_index(tml);
    SDL_free(tml->touchmap_dir);

    tml->enabled = false;
    tml->touchmap_dir = NULL;
    tml->index_entries = NULL;
    tml->index_count = 0;
}

bool
sc_touchmap_loader_rebuild_index(struct sc_touchmap_loader *tml) {
    sc_touchmap_loader_clear_index(tml);

    if (!tml->enabled || !tml->touchmap_dir) {
        return true;
    }

    DIR *dir = opendir(tml->touchmap_dir);
    if (!dir) {
        LOGW("Could not open touchmap directory: %s", tml->touchmap_dir);
        return false;
    }

    bool ok = true;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (!sc_touchmap_loader_has_json_suffix(entry->d_name)) {
            continue;
        }

        char *path = NULL;
        if (!sc_touchmap_loader_join_path(tml->touchmap_dir, entry->d_name,
                                          &path)) {
            ok = false;
            break;
        }

        char *absolute_path = sc_file_get_absolute_path(path);
        SDL_free(path);
        if (!absolute_path) {
            LOGW("Could not resolve touchmap path: %s/%s",
                 tml->touchmap_dir, entry->d_name);
            continue;
        }

        path = SDL_strdup(absolute_path);
        free(absolute_path);
        if (!path) {
            LOG_OOM();
            ok = false;
            break;
        }

        char *package_name = sc_touchmap_read_package_name(path);
        if (!package_name) {
            LOGI("Ignoring touchmap without packageName or valid JSON: %s",
                 path);
            SDL_free(path);
            continue;
        }

        LOGI("Indexed touchmap package %s -> %s", package_name, path);
        if (!sc_touchmap_loader_add_index_entry(tml, package_name, path)) {
            SDL_free(package_name);
            SDL_free(path);
            ok = false;
            break;
        }
    }

    closedir(dir);

    if (!ok) {
        sc_touchmap_loader_clear_index(tml);
        return false;
    }

    if (tml->index_count > 1) {
        qsort(tml->index_entries, tml->index_count, sizeof(*tml->index_entries),
              sc_touchmap_loader_compare_entries);

        size_t write_index = 0;
        for (size_t i = 1; i < tml->index_count; ++i) {
            struct sc_touchmap_loader_entry *kept =
                &tml->index_entries[write_index];
            struct sc_touchmap_loader_entry *candidate =
                &tml->index_entries[i];

            if (SDL_strcmp(kept->package_name, candidate->package_name)) {
                ++write_index;
                if (write_index != i) {
                    tml->index_entries[write_index] = *candidate;
                }
                continue;
            }

            LOGW("Duplicate touchmap package \"%s\": keeping %s, ignoring %s",
                 kept->package_name, kept->path, candidate->path);
            SDL_free(candidate->package_name);
            SDL_free(candidate->path);
        }

        tml->index_count = write_index + 1;
    }

    LOGI("Touchmap index ready: %zu package(s) from %s",
         tml->index_count, tml->touchmap_dir);
    return true;
}

const char *
sc_touchmap_loader_find_path(const struct sc_touchmap_loader *tml,
                             const char *package_name) {
    if (!package_name) {
        return NULL;
    }

    for (size_t i = 0; i < tml->index_count; ++i) {
        if (!SDL_strcmp(tml->index_entries[i].package_name, package_name)) {
            LOGI("Resolved touchmap package %s -> %s", package_name,
                 tml->index_entries[i].path);
            return tml->index_entries[i].path;
        }
    }

    LOGI("No indexed touchmap for package %s", package_name);
    return NULL;
}
