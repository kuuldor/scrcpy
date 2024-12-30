#include "common.h"

#include "third_party/cjson/cJSON.h"

#include "touchmap.h"

#include "util/log.h"

#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>

static SDL_GameControllerButton button_name_to_value(const char *button_name) {
    if (strcmp(button_name, "A") == 0) return SDL_CONTROLLER_BUTTON_A;
    if (strcmp(button_name, "B") == 0) return SDL_CONTROLLER_BUTTON_B;
    if (strcmp(button_name, "X") == 0) return SDL_CONTROLLER_BUTTON_X;
    if (strcmp(button_name, "Y") == 0) return SDL_CONTROLLER_BUTTON_Y;
    if (strcmp(button_name, "BACK") == 0 || strcmp(button_name, "SELECT") == 0) return SDL_CONTROLLER_BUTTON_BACK;
    if (strcmp(button_name, "GUIDE") == 0 || strcmp(button_name, "HOME") == 0) return SDL_CONTROLLER_BUTTON_GUIDE;
    if (strcmp(button_name, "START") == 0) return SDL_CONTROLLER_BUTTON_START;
    if (strcmp(button_name, "LTHUMB") == 0 || strcmp(button_name, "L3") == 0) return SDL_CONTROLLER_BUTTON_LEFTSTICK;
    if (strcmp(button_name, "RTHUMB") == 0 || strcmp(button_name, "R3") == 0) return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    if (strcmp(button_name, "LB") == 0 || strcmp(button_name, "L1") == 0) return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    if (strcmp(button_name, "RB") == 0 || strcmp(button_name, "R1") == 0) return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    if (strcmp(button_name, "UP") == 0) return SDL_CONTROLLER_BUTTON_DPAD_UP;
    if (strcmp(button_name, "DOWN") == 0) return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    if (strcmp(button_name, "LEFT") == 0) return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    if (strcmp(button_name, "RIGHT") == 0) return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    if (strcmp(button_name, "MISC") == 0) return SDL_CONTROLLER_BUTTON_MISC1;
    if (strcmp(button_name, "PADDLE1") == 0) return SDL_CONTROLLER_BUTTON_PADDLE1;
    if (strcmp(button_name, "PADDLE2") == 0) return SDL_CONTROLLER_BUTTON_PADDLE2;
    if (strcmp(button_name, "PADDLE3") == 0) return SDL_CONTROLLER_BUTTON_PADDLE3;
    if (strcmp(button_name, "PADDLE4") == 0) return SDL_CONTROLLER_BUTTON_PADDLE4;
    if (strcmp(button_name, "TOUCHPAD") == 0) return SDL_CONTROLLER_BUTTON_TOUCHPAD;
    if (strcmp(button_name, "LT") == 0 || strcmp(button_name, "L2") == 0) return SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    if (strcmp(button_name, "RT") == 0 || strcmp(button_name, "L2") == 0) return SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    return SDL_CONTROLLER_BUTTON_INVALID; // Return invalid if the name is unrecognized
}



int sc_gptm_compare_btn(const void *a, const void *b) {
    struct sc_gptm_touch_button * p1 = (struct sc_gptm_touch_button*)a;
    struct sc_gptm_touch_button * p2 = (struct sc_gptm_touch_button*)b;

    return p1->button - p2->button;
}

struct sc_gptm_gamepad_touchmap * parse_touchmap_config(const char *filename) {
    if (filename == NULL) {
        LOGE("No touchmap file defined");
        return NULL;
    }

    // Open the configuration file
    FILE *file = fopen(filename, "r");
    if (!file) {
        LOGE("Failed to open configuration file");
        return NULL;
    }

    // Determine the file size
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    // Allocate memory to read the file content
    char *json_string = (char *)malloc(filesize + 1);
    if (!json_string) {
        LOGE("Failed to allocate memory");
        fclose(file);
        return NULL;
    }

    // Read the file content
    fread(json_string, 1, filesize, file);
    json_string[filesize] = '\0'; // Null-terminate the string
    fclose(file);

    // Parse the JSON content
    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        LOGE("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        free(json_string);
        return NULL;
    }


    struct sc_gptm_gamepad_touchmap * map = NULL;


    // Example: Access mappings -> walk_control -> center -> x and y
    cJSON *mappings = cJSON_GetObjectItem(root, "mappings");
    if (mappings) {
        int btn_cnt = 0;
        int skill_cnt = 0;
        uint64_t finger_id = SC_GPTM_BASE_FINGER_ID;

        cJSON * buttons = cJSON_GetObjectItem(mappings, "button_mappings");
        if (buttons && cJSON_IsArray(buttons)) {
            btn_cnt = cJSON_GetArraySize(buttons);
        }
        cJSON * skills = cJSON_GetObjectItem(mappings, "skill_casting");
        if (skills && cJSON_IsArray(skills)) {
            skill_cnt = cJSON_GetArraySize(skills);
        }

        LOGI("Buttons# %d , Skill# %d", btn_cnt, skill_cnt);

        int alloc_size = sizeof(*map) + sizeof(struct sc_gptm_touch_button) * (skill_cnt+btn_cnt);
        map =(struct sc_gptm_gamepad_touchmap *) malloc(alloc_size);
        if (map == NULL) {
            LOGE("Failed to allocate memory");
            return NULL;
        }
        memset(map, 0, alloc_size);
        map->button_cnt = skill_cnt+btn_cnt;

        cJSON *walk_control = cJSON_GetObjectItem(mappings, "walk_control");
        if (walk_control) {
            cJSON *center = cJSON_GetObjectItem(walk_control, "center");
            cJSON *radius = cJSON_GetObjectItem(walk_control, "radius");
            if (center && radius) {
                int x = cJSON_GetObjectItem(center, "x")->valueint;
                int y = cJSON_GetObjectItem(center, "y")->valueint;
                map->walk.center.x = x;
                map->walk.center.y = y;
                map->walk.radius = radius->valueint;
                LOGI("Walk Control Center: (%d, %d) radius: %d", x, y, map->walk.radius);
                map->walk.finger_id = finger_id++;
            }
        }

        struct sc_gptm_touch_button * touch_btn = &map->buttons[0];
        for (int i=0; i < btn_cnt; i++) {
            cJSON * button = cJSON_GetArrayItem(buttons, i);
            cJSON *center = cJSON_GetObjectItem(button, "touch");
            cJSON *trigger = cJSON_GetObjectItem(button, "button");
            if (center && trigger) {
                int x = cJSON_GetObjectItem(center, "x")->valueint;
                int y = cJSON_GetObjectItem(center, "y")->valueint;
                const char * btn_name = trigger->valuestring;

                LOGI("Touch Button <%s> Center: (%d, %d)", btn_name, x, y);
                touch_btn->center.x = x;
                touch_btn->center.y = y;
                touch_btn->radius = 0;
                touch_btn->button = button_name_to_value(btn_name);
                touch_btn->is_skill = false;
                touch_btn->finger_id = finger_id++;

                touch_btn++;
            }
        }

        for (int i=0; i < skill_cnt; i++) {
            cJSON * skill = cJSON_GetArrayItem(skills, i);
            cJSON *center = cJSON_GetObjectItem(skill, "center");
            cJSON *radius = cJSON_GetObjectItem(skill, "radius");
            cJSON *trigger = cJSON_GetObjectItem(skill, "button");
            if (center && radius && trigger) {
                int x = cJSON_GetObjectItem(center, "x")->valueint;
                int y = cJSON_GetObjectItem(center, "y")->valueint;
                const char * btn_name = trigger->valuestring;

                LOGI("Skill Casting <%s> Center: (%d, %d)", btn_name, x, y);
                touch_btn->center.x = x;
                touch_btn->center.y = y;
                touch_btn->radius = radius->valueint;
                touch_btn->button = button_name_to_value(btn_name);
                touch_btn->is_skill = true;
                touch_btn->finger_id = finger_id++;
                touch_btn++;
            }
        }

        qsort(map->buttons, map->button_cnt, sizeof(struct sc_gptm_touch_button), sc_gptm_compare_btn);
    }

    // Cleanup
    cJSON_Delete(root);
    free(json_string);

    return map;
}

