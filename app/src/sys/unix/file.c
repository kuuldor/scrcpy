#include "util/file.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef __APPLE__
# include <mach-o/dyld.h> // for _NSGetExecutablePath()
#endif

#include "util/log.h"

static char *
sc_file_expand_home_path(const char *path) {
    if (!path || path[0] != '~') {
        return strdup(path);
    }

    if (path[1] != '\0' && path[1] != '/') {
        LOGE("Unsupported home path syntax: %s", path);
        return NULL;
    }

    const char *home = getenv("HOME");
    if (!home || !*home) {
        LOGE("HOME is not set");
        return NULL;
    }

    size_t home_len = strlen(home);
    size_t suffix_len = strlen(path + 1);
    char *expanded = malloc(home_len + suffix_len + 1);
    if (!expanded) {
        LOG_OOM();
        return NULL;
    }

    memcpy(expanded, home, home_len);
    memcpy(expanded + home_len, path + 1, suffix_len + 1);
    return expanded;
}

bool
sc_file_executable_exists(const char *file) {
    char *path = getenv("PATH");
    if (!path)
        return false;
    path = strdup(path);
    if (!path)
        return false;

    bool ret = false;
    size_t file_len = strlen(file);
    char *saveptr;
    for (char *dir = strtok_r(path, ":", &saveptr); dir;
            dir = strtok_r(NULL, ":", &saveptr)) {
        size_t dir_len = strlen(dir);
        char *fullpath = malloc(dir_len + file_len + 2);
        if (!fullpath)
        {
            LOG_OOM();
            continue;
        }
        memcpy(fullpath, dir, dir_len);
        fullpath[dir_len] = '/';
        memcpy(fullpath + dir_len + 1, file, file_len + 1);

        struct stat sb;
        bool fullpath_executable = stat(fullpath, &sb) == 0 &&
            sb.st_mode & S_IXUSR;
        free(fullpath);
        if (fullpath_executable) {
            ret = true;
            break;
        }
    }

    free(path);
    return ret;
}

char *
sc_file_get_executable_path(void) {
// <https://stackoverflow.com/a/1024937/1987178>
#ifdef __linux__
    char buf[PATH_MAX + 1]; // +1 for the null byte
    ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX);
    if (len == -1) {
        perror("readlink");
        return NULL;
    }
    buf[len] = '\0';
    return strdup(buf);
#elif defined(__APPLE__)
    char buf[PATH_MAX];
    uint32_t bufsize = PATH_MAX;
    if (_NSGetExecutablePath(buf, &bufsize) != 0) {
        LOGE("Executable path buffer too small; need %u bytes", bufsize);
        return NULL;
    }
    return realpath(buf, NULL);
#else
    // "_" is often used to store the full path of the command being executed
    char *path = getenv("_");
    if (!path) {
        LOGE("Could not determine executable path");
        return NULL;
    }
    return strdup(path);
#endif
}

char *
sc_file_get_absolute_path(const char *path) {
    char *expanded_path = sc_file_expand_home_path(path);
    if (!expanded_path) {
        return NULL;
    }

    char *result = realpath(expanded_path, NULL);
    free(expanded_path);
    if (!result) {
        perror("realpath");
    }
    return result;
}

bool
sc_file_is_regular(const char *path) {
    struct stat path_stat;

    if (stat(path, &path_stat)) {
        perror("stat");
        return false;
    }
    return S_ISREG(path_stat.st_mode);
}
