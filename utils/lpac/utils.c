#include "utils.h"

#include <cjson-ext/cJSON_ex.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(_WIN32)
static char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new = (char *)malloc(len + 1);

    if (new == NULL)
        return NULL;

    new[len] = '\0';
    return (char *)memcpy(new, s, len);
}
#endif

const char *getenv_str_or_default(const char *name, const char *default_value) {
    const char *value = getenv(name);
    if (value == NULL || strlen(value) == 0)
        return default_value;
    return value;
}

bool getenv_bool_or_default(const char *name, const bool default_value) {
    const char *value = getenv(name);
    if (value == NULL || strlen(value) == 0)
        return default_value;
    const int b = str_to_bool(value);
    if (b != -1)
        return b;
    fprintf(stderr, "WARNING: Invalid value '%s' for environment variable '%s', falling back to default (%s)\n", value,
            name, default_value ? "true" : "false");
    return default_value;
}

int getenv_int_or_default(const char *name, const int default_value) {
    return (int)getenv_long_or_default(name, default_value);
}

long getenv_long_or_default(const char *name, const long default_value) {
    const char *value = getenv(name);
    if (value == NULL || strlen(value) == 0)
        return default_value;
    return strtol(value, NULL, 10);
}

void set_deprecated_env_name(const char *name, const char *deprecated_name) {
    const char *value = getenv(name);
    if (value != NULL)
        return; // new env var already set
    value = getenv(deprecated_name);
    if (value == NULL)
        return; // deprecated env var not set
    fprintf(stderr, "WARNING: Please use '%s' instead of '%s'\n", name, deprecated_name);
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

int str_to_bool(const char *value) {
    if (strcasecmp(value, "1") == 0 || strcasecmp(value, "y") == 0 || strcasecmp(value, "on") == 0
        || strcasecmp(value, "yes") == 0 || strcasecmp(value, "true") == 0)
        return true;
    if (strcasecmp(value, "0") == 0 || strcasecmp(value, "n") == 0 || strcasecmp(value, "off") == 0
        || strcasecmp(value, "no") == 0 || strcasecmp(value, "false") == 0)
        return false;
    return -1;
}

bool json_print(char *type, cJSON *jpayload) {
    _cleanup_cjson_ cJSON *jroot = NULL;
    _cleanup_cjson_free_ char *jstr = NULL;

    if (jpayload == NULL) {
        goto err;
    }

    jroot = cJSON_CreateObject();
    if (jroot == NULL) {
        goto err;
    }

    if (cJSON_AddStringOrNullToObject(jroot, "type", type) == NULL) {
        goto err;
    }

    if (cJSON_AddItemReferenceToObject(jroot, "payload", jpayload) == 0) {
        goto err;
    }

    jstr = cJSON_PrintUnformatted(jroot);

    if (jstr == NULL) {
        goto err;
    }

    fprintf(stdout, "%s\n", jstr);
    fflush(stdout);

    return true;

err:
    return false;
}

bool ends_with(const char *restrict str, const char *restrict suffix) {
    if (str == NULL || suffix == NULL) {
        return false;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    return (str_len >= suffix_len) && (!memcmp(str + str_len - suffix_len, suffix, suffix_len));
}

char *remove_suffix(char *restrict str, const char *restrict suffix) {
    if (str == NULL || suffix == NULL) {
        return NULL;
    }
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) {
        return NULL;
    }
    size_t pos = str_len - suffix_len;
    if (strcmp(str + pos, suffix) == 0) {
        return strndup(str, pos);
    } else {
        return NULL;
    }
}

char **merge_array_of_str(char *left[], char *right[]) {
    size_t left_len = 0;
    size_t right_len = 0;
    while (left[left_len] != NULL)
        left_len++;
    while (right[right_len] != NULL)
        right_len++;
    char **merged = calloc(left_len + right_len + 1, sizeof(char *));
    if (merged == NULL)
        return NULL;
    memcpy(merged, left, left_len * sizeof(char *));
    memcpy(merged + left_len, right, right_len * sizeof(char *));
    return merged;
}

char *path_concat(const char *restrict a, const char *restrict b) {
    if (a == NULL || b == NULL) {
        return NULL;
    }
    size_t fullpath_len = strlen(a) + 1 /* SEP */ + strlen(b) + 1 /* NUL */;
    char *fullpath = calloc(fullpath_len, sizeof(char));
    if (fullpath == NULL) {
        return NULL;
    }
    snprintf(fullpath, fullpath_len, "%s/%s", a, b);
    return fullpath;
}

inline struct timespec get_current_clock(clockid_t clock_id) {
    struct timespec current_time;
    clock_gettime(clock_id, &current_time);
    return current_time;
}

inline struct timespec get_duration(struct timespec t0, struct timespec t1) {
    bool borrow = (t1.tv_nsec < t0.tv_nsec);
    struct timespec dur = {.tv_sec = (borrow ? 1e9 : 0) + t1.tv_sec - t0.tv_sec,
                           .tv_nsec = t1.tv_nsec - t0.tv_nsec - (borrow ? 1 : 0)};
    return dur;
}

inline struct timespec get_wall_time(struct timespec wall) {
    return get_duration(wall, get_current_clock(CLOCK_MONOTONIC));
}
