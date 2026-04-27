#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "touchmap.h"
#include "touchmap_loader.h"

static bool
write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }

    size_t len = strlen(content);
    bool ok = fwrite(content, 1, len, file) == len;
    ok = fclose(file) == 0 && ok;
    return ok;
}

static char *
join_path(const char *dir, const char *name) {
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    char *path = SDL_malloc(dir_len + 1 + name_len + 1);
    assert(path);
    memcpy(path, dir, dir_len);
    path[dir_len] = '/';
    memcpy(path + dir_len + 1, name, name_len + 1);
    return path;
}

static void
remove_if_exists(const char *path) {
    (void) unlink(path);
}

static void
prepare_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        assert(S_ISDIR(st.st_mode));
        return;
    }

    assert(mkdir(path, 0700) == 0);
}

static void
cleanup_dir_files(const char *dir, const char *const names[]) {
    for (size_t i = 0; names[i]; ++i) {
        char *path = join_path(dir, names[i]);
        remove_if_exists(path);
        SDL_free(path);
    }
}

static void
test_touchmap_loader_init_disabled(void) {
    struct sc_touchmap_loader tml;
    bool ok = sc_touchmap_loader_init(&tml, NULL);
    assert(ok);
    assert(!tml.enabled);
    assert(!tml.touchmap_dir);
    assert(!tml.index_entries);
    assert(tml.index_count == 0);
    sc_touchmap_loader_destroy(&tml);
}

static void
test_touchmap_loader_init_enabled(void) {
    const char *dir = "/tmp/scrcpy-touchmap-loader-init-enabled";
    prepare_dir(dir);

    struct sc_touchmap_loader tml;
    bool ok = sc_touchmap_loader_init(&tml, dir);
    assert(ok);
    assert(tml.enabled);
    assert(tml.touchmap_dir);
    assert(!SDL_strcmp(tml.touchmap_dir, dir));
    assert(!tml.index_entries);
    assert(tml.index_count == 0);
    sc_touchmap_loader_destroy(&tml);

    assert(rmdir(dir) == 0);
}

static void
test_touchmap_loader_init_expands_home(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) {
        return;
    }

    struct sc_touchmap_loader tml;
    bool ok = sc_touchmap_loader_init(&tml, "~");
    assert(ok);
    assert(tml.enabled);
    assert(tml.touchmap_dir);
    assert(!SDL_strcmp(tml.touchmap_dir, home));

    sc_touchmap_loader_destroy(&tml);
}

static void
test_touchmap_loader_runtime_state(void) {
    const char *dir = "/tmp/scrcpy-touchmap-loader-runtime";
    prepare_dir(dir);

    struct sc_touchmap_loader tml;
    bool ok = sc_touchmap_loader_init(&tml, dir);
    assert(ok);

    tml.index_entries = SDL_calloc(2, sizeof(*tml.index_entries));
    assert(tml.index_entries);
    tml.index_count = 2;
    tml.index_entries[0].package_name = SDL_strdup("com.example.one");
    tml.index_entries[0].path = join_path(dir, "one.json");
    tml.index_entries[1].package_name = SDL_strdup("com.example.two");
    tml.index_entries[1].path = join_path(dir, "two.json");
    assert(tml.index_entries[0].package_name);
    assert(tml.index_entries[0].path);
    assert(tml.index_entries[1].package_name);
    assert(tml.index_entries[1].path);

    sc_touchmap_loader_destroy(&tml);
    assert(!tml.enabled);
    assert(!tml.touchmap_dir);
    assert(!tml.index_entries);
    assert(tml.index_count == 0);

    assert(rmdir(dir) == 0);
}

static void
test_touchmap_loader_rebuild_index(void) {
    static const char *const names[] = {
        "alpha.json",
        "beta.json",
        "duplicate_a.json",
        "duplicate_b.json",
        "invalid.json",
        "missing.json",
        "notes.txt",
        NULL,
    };

    const char *dir = "/tmp/scrcpy-touchmap-loader-test";
    prepare_dir(dir);
    cleanup_dir_files(dir, names);

    char *path = join_path(dir, "alpha.json");
    assert(write_file(path,
        "{"
        "\"packageName\":\"com.example.alpha\","
        "\"mappings\":{\"button_mappings\":[],\"skill_casting\":[]}"
        "}"));
    SDL_free(path);

    path = join_path(dir, "beta.json");
    assert(write_file(path,
        "{"
        "\"packageName\":\"com.example.beta\","
        "\"mappings\":{\"button_mappings\":[],\"skill_casting\":[]}"
        "}"));
    SDL_free(path);

    path = join_path(dir, "duplicate_a.json");
    assert(write_file(path,
        "{"
        "\"packageName\":\"com.example.dup\","
        "\"mappings\":{\"button_mappings\":[],\"skill_casting\":[]}"
        "}"));
    SDL_free(path);

    path = join_path(dir, "duplicate_b.json");
    assert(write_file(path,
        "{"
        "\"packageName\":\"com.example.dup\","
        "\"mappings\":{\"button_mappings\":[],\"skill_casting\":[]}"
        "}"));
    SDL_free(path);

    path = join_path(dir, "invalid.json");
    assert(write_file(path, "{invalid"));
    SDL_free(path);

    path = join_path(dir, "missing.json");
    assert(write_file(path, "{\"mappings\":{}}"));
    SDL_free(path);

    path = join_path(dir, "notes.txt");
    assert(write_file(path, "not json"));
    SDL_free(path);

    struct sc_touchmap_loader tml;
    bool ok = sc_touchmap_loader_init(&tml, dir);
    assert(ok);

    ok = sc_touchmap_loader_rebuild_index(&tml);
    assert(ok);
    assert(tml.index_count == 3);

    const char *alpha = sc_touchmap_loader_find_path(&tml, "com.example.alpha");
    const char *beta = sc_touchmap_loader_find_path(&tml, "com.example.beta");
    const char *dup = sc_touchmap_loader_find_path(&tml, "com.example.dup");
    assert(alpha);
    assert(beta);
    assert(dup);
    assert(strstr(alpha, "/alpha.json"));
    assert(strstr(beta, "/beta.json"));
    assert(strstr(dup, "/duplicate_a.json"));
    assert(!sc_touchmap_loader_find_path(&tml, "com.example.missing"));

    sc_touchmap_loader_destroy(&tml);
    cleanup_dir_files(dir, names);
    assert(rmdir(dir) == 0);
}

int
main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test_touchmap_loader_init_disabled();
    test_touchmap_loader_init_enabled();
    test_touchmap_loader_init_expands_home();
    test_touchmap_loader_runtime_state();
    test_touchmap_loader_rebuild_index();
    return 0;
}
