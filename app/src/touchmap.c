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
    if (strcmp(button_name, "RT") == 0 || strcmp(button_name, "R2") == 0) return SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    return SDL_CONTROLLER_BUTTON_INVALID; // Return invalid if the name is unrecognized
}

static const char *
button_value_to_name(uint8_t button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return "A";
        case SDL_CONTROLLER_BUTTON_B: return "B";
        case SDL_CONTROLLER_BUTTON_X: return "X";
        case SDL_CONTROLLER_BUTTON_Y: return "Y";
        case SDL_CONTROLLER_BUTTON_BACK: return "BACK";
        case SDL_CONTROLLER_BUTTON_GUIDE: return "GUIDE";
        case SDL_CONTROLLER_BUTTON_START: return "START";
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "LTHUMB";
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "RTHUMB";
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "LB";
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "RB";
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return "UP";
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "DOWN";
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "LEFT";
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "RIGHT";
        case SDL_CONTROLLER_BUTTON_MISC1: return "MISC";
        case SDL_CONTROLLER_BUTTON_PADDLE1: return "PADDLE1";
        case SDL_CONTROLLER_BUTTON_PADDLE2: return "PADDLE2";
        case SDL_CONTROLLER_BUTTON_PADDLE3: return "PADDLE3";
        case SDL_CONTROLLER_BUTTON_PADDLE4: return "PADDLE4";
        case SDL_CONTROLLER_BUTTON_TOUCHPAD: return "TOUCHPAD";
        default:
            if (button == SDL_CONTROLLER_BUTTON_MAX
                    + SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                return "LT";
            }
            if (button == SDL_CONTROLLER_BUTTON_MAX
                    + SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                return "RT";
            }
            return "UNKNOWN";
    }
}



int sc_gptm_compare_btn(const void *a, const void *b) {
    struct sc_gptm_touch_button * p1 = (struct sc_gptm_touch_button*)a;
    struct sc_gptm_touch_button * p2 = (struct sc_gptm_touch_button*)b;

    return p1->button - p2->button;
}

void
sc_gptm_gamepad_touchmap_destroy(struct sc_gptm_gamepad_touchmap *map) {
    if (!map) {
        return;
    }

    if (map->json_root) {
        cJSON_Delete(map->json_root);
    }
    free(map);
}

static bool
replace_or_add(cJSON *object, const char *name, cJSON *item) {
    if (!object || !name || !item) {
        cJSON_Delete(item);
        return false;
    }

    if (cJSON_GetObjectItemCaseSensitive(object, name)) {
        if (!cJSON_ReplaceItemInObjectCaseSensitive(object, name, item)) {
            cJSON_Delete(item);
            return false;
        }
    } else if (!cJSON_AddItemToObject(object, name, item)) {
        cJSON_Delete(item);
        return false;
    }

    return true;
}

static cJSON *
get_or_add_object(cJSON *object, const char *name) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (cJSON_IsObject(item)) {
        return item;
    }

    cJSON *replacement = cJSON_CreateObject();
    if (!replacement) {
        return NULL;
    }

    if (!replace_or_add(object, name, replacement)) {
        return NULL;
    }

    return replacement;
}

static bool
set_number_item(cJSON *object, const char *name, int32_t value) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (cJSON_IsNumber(item)) {
        cJSON_SetNumberValue(item, value);
        return true;
    }

    return replace_or_add(object, name, cJSON_CreateNumber(value));
}

static bool
set_string_item(cJSON *object, const char *name, const char *value) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(object, name);
    if (cJSON_IsString(item)) {
        return cJSON_SetValuestring(item, value) != NULL;
    }

    return replace_or_add(object, name, cJSON_CreateString(value));
}

static bool
update_point_object(cJSON *object, const char *name,
                    const struct sc_point *point) {
    cJSON *point_object = get_or_add_object(object, name);
    if (!point_object) {
        return false;
    }

    return set_number_item(point_object, "x", point->x)
        && set_number_item(point_object, "y", point->y);
}

static bool
update_walk_json(cJSON *mappings,
                 const struct sc_gptm_walk_control *walk,
                 cJSON **json_entry) {
    cJSON *entry = get_or_add_object(mappings, "walk_control");
    if (!entry) {
        return false;
    }

    bool ok = update_point_object(entry, "center", &walk->center)
           && set_number_item(entry, "radius", walk->radius);
    if (ok && json_entry) {
        *json_entry = entry;
    }
    return ok;
}

static bool
update_button_json(cJSON *entry, const struct sc_gptm_touch_button *btn) {
    const char *btn_name = button_value_to_name(btn->button);
    if (!set_string_item(entry, "button", btn_name)) {
        return false;
    }

    if (btn->is_skill) {
        return update_point_object(entry, "center", &btn->center)
            && set_number_item(entry, "radius", btn->radius);
    }

    return update_point_object(entry, "touch", &btn->center);
}

static cJSON *
create_button_json_entry(const struct sc_gptm_touch_button *btn) {
    cJSON *entry = cJSON_IsObject(btn->json_entry)
                 ? cJSON_Duplicate(btn->json_entry, true)
                 : cJSON_CreateObject();
    if (!entry) {
        return NULL;
    }

    if (!update_button_json(entry, btn)) {
        cJSON_Delete(entry);
        return NULL;
    }

    return entry;
}

static bool
rebuild_button_arrays(cJSON *mappings,
                      const struct sc_gptm_gamepad_touchmap *map,
                      cJSON **json_entries) {
    cJSON *buttons = cJSON_CreateArray();
    cJSON *skills = cJSON_CreateArray();
    if (!buttons || !skills) {
        cJSON_Delete(buttons);
        cJSON_Delete(skills);
        return false;
    }

    for (int i = 0; i < map->button_cnt; ++i) {
        const struct sc_gptm_touch_button *btn = &map->buttons[i];
        cJSON *entry = create_button_json_entry(btn);
        if (!entry) {
            cJSON_Delete(buttons);
            cJSON_Delete(skills);
            return false;
        }

        cJSON *target = btn->is_skill ? skills : buttons;
        if (!cJSON_AddItemToArray(target, entry)) {
            cJSON_Delete(entry);
            cJSON_Delete(buttons);
            cJSON_Delete(skills);
            return false;
        }

        if (json_entries) {
            json_entries[i] = entry;
        }
    }

    if (!replace_or_add(mappings, "button_mappings", buttons)) {
        cJSON_Delete(skills);
        return false;
    }

    if (!replace_or_add(mappings, "skill_casting", skills)) {
        return false;
    }

    return true;
}

static bool
update_touchmap_json_root(struct sc_gptm_gamepad_touchmap *map) {
    cJSON **button_json_entries = NULL;
    if (map->button_cnt > 0) {
        button_json_entries = calloc(map->button_cnt,
                                     sizeof(*button_json_entries));
        if (!button_json_entries) {
            LOG_OOM();
            return false;
        }
    }

    cJSON *walk_json_entry = NULL;
    cJSON *root = map->json_root ? cJSON_Duplicate(map->json_root, true)
                                 : cJSON_CreateObject();
    if (!root) {
        free(button_json_entries);
        return false;
    }

    cJSON *mappings = get_or_add_object(root, "mappings");
    if (!mappings || !update_walk_json(mappings, &map->walk,
                                       &walk_json_entry)
            || !rebuild_button_arrays(mappings, map,
                                      button_json_entries)) {
        free(button_json_entries);
        cJSON_Delete(root);
        return false;
    }

    if (map->json_root) {
        cJSON_Delete(map->json_root);
    }

    map->json_root = root;
    map->walk.json_entry = walk_json_entry;
    for (int i = 0; i < map->button_cnt; ++i) {
        map->buttons[i].json_entry = button_json_entries[i];
    }

    free(button_json_entries);
    return true;
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
            cJSON_Delete(root);
            free(json_string);
            return NULL;
        }
        memset(map, 0, alloc_size);
        map->button_cnt = skill_cnt+btn_cnt;
        map->json_root = root;

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
                map->walk.json_entry = walk_control;
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
                touch_btn->json_entry = button;

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
                touch_btn->json_entry = skill;
                touch_btn++;
            }
        }

        qsort(map->buttons, map->button_cnt, sizeof(struct sc_gptm_touch_button), sc_gptm_compare_btn);
    }

    if (!map) {
        cJSON_Delete(root);
    }
    free(json_string);

    return map;
}

bool
save_touchmap_config(const char *filename,
                     struct sc_gptm_gamepad_touchmap *map) {
    if (!filename || !map) {
        LOGE("No touchmap to save");
        return false;
    }

    if (!update_touchmap_json_root(map)) {
        LOGE("Failed to create touchmap JSON");
        return false;
    }

    char *json_string = cJSON_Print(map->json_root);
    if (!json_string) {
        LOGE("Failed to serialize touchmap");
        return false;
    }

    FILE *file = fopen(filename, "w");
    if (!file) {
        LOGE("Failed to open file for writing: %s", filename);
        cJSON_free(json_string);
        return false;
    }

    fwrite(json_string, 1, strlen(json_string), file);
    fclose(file);

    cJSON_free(json_string);
    return true;
}
