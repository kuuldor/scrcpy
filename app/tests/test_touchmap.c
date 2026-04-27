#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "options.h"
#include "third_party/cjson/cJSON.h"
#include "touchmap.h"
#include "touchmap_editor.h"
#include "touchmap_overlay.h"

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
read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }

    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    assert(size >= 0);
    rewind(file);

    char *content = malloc((size_t) size + 1);
    assert(content);

    assert(fread(content, 1, (size_t) size, file) == (size_t) size);
    content[size] = '\0';
    fclose(file);
    return content;
}

static cJSON *
get_path(cJSON *root, const char *first, const char *second) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(root, first);
    return cJSON_GetObjectItemCaseSensitive(item, second);
}

static cJSON *
find_mapping(cJSON *array, const char *button) {
    assert(cJSON_IsArray(array));

    cJSON *item;
    cJSON_ArrayForEach(item, array) {
        cJSON *name = cJSON_GetObjectItemCaseSensitive(item, "button");
        if (cJSON_IsString(name) && !strcmp(name->valuestring, button)) {
            return item;
        }
    }

    return NULL;
}

static struct sc_gptm_touch_button *
find_button(struct sc_gptm_gamepad_touchmap *map, uint8_t button) {
    for (int i = 0; i < map->button_cnt; ++i) {
        if (map->buttons[i].button == button) {
            return &map->buttons[i];
        }
    }

    return NULL;
}

static void
assert_point(cJSON *object, const char *name, int x, int y) {
    cJSON *point = cJSON_GetObjectItemCaseSensitive(object, name);
    assert(cJSON_GetObjectItemCaseSensitive(point, "x")->valueint == x);
    assert(cJSON_GetObjectItemCaseSensitive(point, "y")->valueint == y);
}

static void
test_parse_save_preserves_metadata(void) {
    const char *input_path = "test_touchmap_input.json";
    const char *output_path = "test_touchmap_output.json";

    const char *json =
        "{"
        "  \"type\": \"scrcpy-touchmap\","
        "  \"packageName\": \"example.game\","
        "  \"titles\": [\"Main\"],"
        "  \"mappings\": {"
        "    \"profile\": \"default\","
        "    \"walk_control\": {"
        "      \"label\": \"walk\","
        "      \"center\": {\"x\": 310, \"y\": 845},"
        "      \"radius\": 150"
        "    },"
        "    \"button_mappings\": ["
        "      {\"button\": \"B\", \"touch\": {\"x\": 1700, \"y\": 910}, \"label\": \"cancel\"},"
        "      {\"button\": \"A\", \"touch\": {\"x\": 1765, \"y\": 920}, \"label\": \"confirm\"}"
        "    ],"
        "    \"skill_casting\": ["
        "      {\"button\": \"RB\", \"center\": {\"x\": 1427, \"y\": 944}, \"radius\": 180, \"mode\": \"aim\"}"
        "    ]"
        "  }"
        "}";

    assert(write_file(input_path, json));

    struct sc_gptm_gamepad_touchmap *map = parse_touchmap_config(input_path);
    assert(map);
    assert(map->has_walk);
    assert(map->walk.center.x == 310);
    assert(map->walk.center.y == 845);
    assert(map->walk.radius == 150);
    assert(map->button_cnt == 3);

    struct sc_gptm_touch_button *button_a =
        find_button(map, SDL_CONTROLLER_BUTTON_A);
    struct sc_gptm_touch_button *button_b =
        find_button(map, SDL_CONTROLLER_BUTTON_B);
    struct sc_gptm_touch_button *button_rb =
        find_button(map, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    assert(button_a);
    assert(button_b);
    assert(button_rb);

    map->walk.center = (struct sc_point) {320, 850};
    map->walk.radius = 160;
    button_a->center = (struct sc_point) {1800, 930};
    button_rb->radius = 210;

    assert(save_touchmap_config(output_path, map));

    button_b->center = (struct sc_point) {1710, 915};
    assert(save_touchmap_config(output_path, map));

    char *saved = read_file(output_path);
    assert(saved);
    cJSON *root = cJSON_Parse(saved);
    assert(root);

    cJSON *package_name = cJSON_GetObjectItemCaseSensitive(root, "packageName");
    assert(cJSON_IsString(package_name));
    assert(!strcmp(package_name->valuestring, "example.game"));

    cJSON *titles = cJSON_GetObjectItemCaseSensitive(root, "titles");
    assert(cJSON_IsArray(titles));

    cJSON *mappings = cJSON_GetObjectItemCaseSensitive(root, "mappings");
    cJSON *profile = cJSON_GetObjectItemCaseSensitive(mappings, "profile");
    assert(cJSON_IsString(profile));
    assert(!strcmp(profile->valuestring, "default"));

    cJSON *walk = cJSON_GetObjectItemCaseSensitive(mappings, "walk_control");
    cJSON *walk_label = cJSON_GetObjectItemCaseSensitive(walk, "label");
    assert(cJSON_IsString(walk_label));
    assert(!strcmp(walk_label->valuestring, "walk"));
    assert_point(walk, "center", 320, 850);
    assert(cJSON_GetObjectItemCaseSensitive(walk, "radius")->valueint == 160);

    cJSON *buttons = get_path(root, "mappings", "button_mappings");
    cJSON *saved_a = find_mapping(buttons, "A");
    cJSON *saved_b = find_mapping(buttons, "B");
    assert(saved_a);
    assert(saved_b);
    assert_point(saved_a, "touch", 1800, 930);
    assert_point(saved_b, "touch", 1710, 915);
    cJSON *label_a = cJSON_GetObjectItemCaseSensitive(saved_a, "label");
    assert(cJSON_IsString(label_a));
    assert(!strcmp(label_a->valuestring, "confirm"));

    cJSON *skills = get_path(root, "mappings", "skill_casting");
    cJSON *saved_rb = find_mapping(skills, "RB");
    assert(saved_rb);
    assert_point(saved_rb, "center", 1427, 944);
    assert(cJSON_GetObjectItemCaseSensitive(saved_rb, "radius")->valueint
           == 210);
    cJSON *mode = cJSON_GetObjectItemCaseSensitive(saved_rb, "mode");
    assert(cJSON_IsString(mode));
    assert(!strcmp(mode->valuestring, "aim"));

    cJSON_Delete(root);
    free(saved);
    sc_gptm_gamepad_touchmap_destroy(map);
    remove(input_path);
    remove(output_path);
}

static void
test_empty_touchmap_parse_save(void) {
    const char *input_path = "test_touchmap_empty_input.json";
    const char *output_path = "test_touchmap_empty_output.json";

    const char *json =
        "{"
        "  \"packageName\": \"empty.game\","
        "  \"mappings\": {"
        "    \"button_mappings\": [],"
        "    \"skill_casting\": []"
        "  }"
        "}";

    assert(write_file(input_path, json));

    struct sc_gptm_gamepad_touchmap *map = parse_touchmap_config(input_path);
    assert(map);
    assert(!map->has_walk);
    assert(map->button_cnt == 0);

    assert(save_touchmap_config(output_path, map));

    char *saved = read_file(output_path);
    assert(saved);
    cJSON *root = cJSON_Parse(saved);
    assert(root);

    cJSON *mappings = cJSON_GetObjectItemCaseSensitive(root, "mappings");
    assert(cJSON_IsObject(mappings));
    assert(!cJSON_GetObjectItemCaseSensitive(mappings, "walk_control"));
    assert(cJSON_IsArray(
        cJSON_GetObjectItemCaseSensitive(mappings, "button_mappings")));
    assert(cJSON_IsArray(
        cJSON_GetObjectItemCaseSensitive(mappings, "skill_casting")));

    cJSON_Delete(root);
    free(saved);
    sc_gptm_gamepad_touchmap_destroy(map);
    remove(input_path);
    remove(output_path);
}

static void
test_touchmap_package_metadata_helpers(void) {
    const char *input_path = "test_touchmap_package_input.json";
    const char *missing_path = "test_touchmap_package_missing.json";
    const char *output_path = "test_touchmap_package_output.json";

    const char *json =
        "{"
        "  \"packageName\": \"example.initial\","
        "  \"mappings\": {"
        "    \"button_mappings\": [],"
        "    \"skill_casting\": []"
        "  }"
        "}";
    assert(write_file(input_path, json));

    char *package_name = sc_touchmap_read_package_name(input_path);
    assert(package_name);
    assert(!strcmp(package_name, "example.initial"));
    SDL_free(package_name);

    assert(write_file(missing_path, "{\"mappings\": {}}"));
    assert(!sc_touchmap_read_package_name(missing_path));

    struct sc_gptm_gamepad_touchmap *map = parse_touchmap_config(input_path);
    assert(map);

    assert(sc_gptm_gamepad_touchmap_set_package_name(map, "example.changed"));
    assert(save_touchmap_config(output_path, map));

    package_name = sc_touchmap_read_package_name(output_path);
    assert(package_name);
    assert(!strcmp(package_name, "example.changed"));
    SDL_free(package_name);

    assert(sc_gptm_gamepad_touchmap_set_package_name(map, NULL));
    assert(save_touchmap_config(output_path, map));
    assert(!sc_touchmap_read_package_name(output_path));

    assert(sc_gptm_gamepad_touchmap_set_package_name(map, "example.final"));
    assert(save_touchmap_config(output_path, map));
    package_name = sc_touchmap_read_package_name(output_path);
    assert(package_name);
    assert(!strcmp(package_name, "example.final"));
    SDL_free(package_name);

    char *filename = sc_touchmap_build_default_filename("example.final");
    assert(filename);
    assert(!strcmp(filename, "example.final.json"));
    SDL_free(filename);

    filename = sc_touchmap_build_default_filename("bad/package\\name");
    assert(filename);
    assert(!strcmp(filename, "bad_package_name.json"));
    SDL_free(filename);

    assert(!sc_touchmap_build_default_filename(NULL));
    assert(!sc_touchmap_build_default_filename(""));

    sc_gptm_gamepad_touchmap_destroy(map);
    remove(input_path);
    remove(missing_path);
    remove(output_path);
}

static void
test_empty_touchmap_create_save(void) {
    const char *output_path = "test_touchmap_new_empty_output.json";

    struct sc_gptm_gamepad_touchmap *map =
        sc_gptm_gamepad_touchmap_new_empty();
    assert(map);
    assert(sc_gptm_gamepad_touchmap_set_package_name(map, "mutation.game"));
    assert(!map->has_walk);
    assert(map->button_cnt == 0);

    assert(save_touchmap_config(output_path, map));

    char *saved = read_file(output_path);
    assert(saved);
    cJSON *root = cJSON_Parse(saved);
    assert(root);

    cJSON *mappings = cJSON_GetObjectItemCaseSensitive(root, "mappings");
    assert(cJSON_IsObject(mappings));
    assert(!cJSON_GetObjectItemCaseSensitive(mappings, "walk_control"));
    assert(cJSON_IsArray(
        cJSON_GetObjectItemCaseSensitive(mappings, "button_mappings")));
    assert(cJSON_IsArray(
        cJSON_GetObjectItemCaseSensitive(mappings, "skill_casting")));

    cJSON_Delete(root);
    free(saved);
    sc_gptm_gamepad_touchmap_destroy(map);
    remove(output_path);
}

static void
test_touchmap_mutation_helpers(void) {
    const char *output_path = "test_touchmap_mutation_output.json";
    remove(output_path);

    struct sc_gptm_gamepad_touchmap *map =
        sc_gptm_gamepad_touchmap_new_empty();
    assert(map);
    assert(sc_gptm_gamepad_touchmap_set_package_name(map, "mutation.game"));

    assert(sc_gptm_gamepad_touchmap_set_walk(
        map, (struct sc_point) {100, 200}, 10));
    assert(map->has_walk);
    assert(map->walk.center.x == 100);
    assert(map->walk.center.y == 200);
    assert(map->walk.radius == SC_TOUCHMAP_MIN_RADIUS);
    assert(map->walk.finger_id == SC_GPTM_BASE_FINGER_ID);

    assert(sc_gptm_gamepad_touchmap_remove_walk(map));
    assert(!map->has_walk);
    assert(!sc_gptm_gamepad_touchmap_remove_walk(map));

    int index = -1;
    struct sc_gptm_touch_button button_b = {
        .center = {300, 400},
        .button = SDL_CONTROLLER_BUTTON_B,
        .is_skill = false,
    };
    map = sc_gptm_gamepad_touchmap_add_button(map, &button_b, &index);
    assert(map);
    assert(index == 0);
    assert(map->button_cnt == 1);
    assert(map->buttons[0].button == SDL_CONTROLLER_BUTTON_B);
    assert(map->buttons[0].center.x == 300);
    assert(map->buttons[0].current_pos.x == 300);
    assert(map->buttons[0].finger_id == SC_GPTM_BASE_FINGER_ID);
    assert(sc_gptm_touch_button_is_bound(&map->buttons[0]));
    assert(sc_gptm_gamepad_touchmap_find_button(
        map, SDL_CONTROLLER_BUTTON_B) == &map->buttons[0]);
    assert(!sc_gptm_gamepad_touchmap_find_button(
        map, SC_GPTM_BUTTON_UNBOUND));

    struct sc_gptm_touch_button button_a = {
        .center = {100, 200},
        .button = SDL_CONTROLLER_BUTTON_A,
        .is_skill = false,
    };
    map = sc_gptm_gamepad_touchmap_add_button(map, &button_a, &index);
    assert(map);
    assert(index == 0);
    assert(map->button_cnt == 2);
    assert(map->buttons[0].button == SDL_CONTROLLER_BUTTON_A);
    assert(map->buttons[1].button == SDL_CONTROLLER_BUTTON_B);
    assert(map->buttons[1].finger_id == SC_GPTM_BASE_FINGER_ID);

    struct sc_gptm_touch_button skill_unbound = {
        .center = {500, 600},
        .radius = 75,
        .button = SC_GPTM_BUTTON_UNBOUND,
        .is_skill = true,
    };
    map = sc_gptm_gamepad_touchmap_add_button(map, &skill_unbound, &index);
    assert(map);
    assert(index == 2);
    assert(map->button_cnt == 3);
    assert(map->buttons[2].button == SC_GPTM_BUTTON_UNBOUND);
    assert(map->buttons[2].radius == 75);
    assert(!sc_gptm_touch_button_is_bound(&map->buttons[2]));
    assert(!sc_gptm_gamepad_touchmap_find_button(
        map, SC_GPTM_BUTTON_UNBOUND));

    int bound_index = -1;
    assert(sc_gptm_gamepad_touchmap_bind_button(
        map, 2, SDL_CONTROLLER_BUTTON_B, &bound_index));
    assert(bound_index == 1);
    assert(map->buttons[0].button == SDL_CONTROLLER_BUTTON_A);
    assert(map->buttons[1].button == SDL_CONTROLLER_BUTTON_B);
    assert(map->buttons[1].is_skill);
    assert(map->buttons[2].button == SC_GPTM_BUTTON_UNBOUND);
    assert(!map->buttons[2].is_skill);

    assert(!save_touchmap_config(output_path, map));
    assert(!read_file(output_path));

    int next_index = -2;
    map = sc_gptm_gamepad_touchmap_remove_button(map, 2, &next_index);
    assert(map);
    assert(map->button_cnt == 2);
    assert(next_index == 1);
    assert(map->buttons[0].button == SDL_CONTROLLER_BUTTON_A);
    assert(map->buttons[1].button == SDL_CONTROLLER_BUTTON_B);

    assert(sc_gptm_gamepad_touchmap_bind_button(
        map, 1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, &bound_index));
    assert(bound_index == 1);
    assert(save_touchmap_config(output_path, map));

    char *saved = read_file(output_path);
    assert(saved);
    cJSON *root = cJSON_Parse(saved);
    assert(root);

    cJSON *package_name = cJSON_GetObjectItemCaseSensitive(root,
                                                           "packageName");
    assert(cJSON_IsString(package_name));
    assert(!strcmp(package_name->valuestring, "mutation.game"));

    cJSON *buttons = get_path(root, "mappings", "button_mappings");
    assert(cJSON_GetArraySize(buttons) == 1);
    assert(find_mapping(buttons, "A"));

    cJSON *skills = get_path(root, "mappings", "skill_casting");
    assert(cJSON_GetArraySize(skills) == 1);
    assert(find_mapping(skills, "RB"));

    cJSON_Delete(root);
    free(saved);
    sc_gptm_gamepad_touchmap_destroy(map);
    remove(output_path);
}

static void
assert_transformed_point(enum sc_orientation orientation, int x, int y) {
    const struct sc_size frame_size = {100, 200};
    const SDL_Rect unrotated = {10, 20, 1000, 2000};
    const SDL_Rect rotated = {10, 20, 2000, 1000};
    const SDL_Rect *rect = sc_orientation_is_swap(orientation)
                         ? &rotated : &unrotated;
    const struct sc_point point = {10, 30};

    struct sc_point result = sc_touchmap_overlay_transform_point(
        &point, &frame_size, rect, orientation);
    assert(result.x == x);
    assert(result.y == y);
}

static void
test_overlay_coordinate_transforms(void) {
    assert_transformed_point(SC_ORIENTATION_0, 110, 320);
    assert_transformed_point(SC_ORIENTATION_90, 1710, 120);
    assert_transformed_point(SC_ORIENTATION_180, 910, 1720);
    assert_transformed_point(SC_ORIENTATION_270, 310, 920);
    assert_transformed_point(SC_ORIENTATION_FLIP_0, 910, 320);
    assert_transformed_point(SC_ORIENTATION_FLIP_90, 1710, 920);
    assert_transformed_point(SC_ORIENTATION_FLIP_180, 110, 1720);
    assert_transformed_point(SC_ORIENTATION_FLIP_270, 310, 120);

    const struct sc_size frame_size = {100, 200};
    const SDL_Rect unrotated = {0, 0, 1000, 2000};
    const SDL_Rect rotated = {0, 0, 2000, 1000};

    assert(sc_touchmap_overlay_transform_radius(
        7, &frame_size, &unrotated, SC_ORIENTATION_0) == 70);
    assert(sc_touchmap_overlay_transform_radius(
        7, &frame_size, &rotated, SC_ORIENTATION_90) == 70);
}

static void
test_editor_keyboard_nudging(void) {
    struct sc_gptm_gamepad_touchmap *map =
        calloc(1, sizeof(*map) + 2 * sizeof(*map->buttons));
    assert(map);

    map->has_walk = true;
    map->walk.center = (struct sc_point) {100, 200};
    map->walk.current_pos = map->walk.center;
    map->walk.radius = 40;
    map->button_cnt = 2;
    map->buttons[0].center = (struct sc_point) {300, 400};
    map->buttons[0].current_pos = map->buttons[0].center;
    map->buttons[0].radius = 0;
    map->buttons[0].is_skill = false;
    map->buttons[1].center = (struct sc_point) {500, 600};
    map->buttons[1].current_pos = map->buttons[1].center;
    map->buttons[1].radius = 50;
    map->buttons[1].is_skill = true;

    struct sc_touchmap_editor editor;
    sc_touchmap_editor_init(&editor);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_SELECT);

    sc_touchmap_editor_set_mode(&editor,
                                SC_TOUCHMAP_EDITOR_MODE_PLACE_BUTTON);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_PLACE_BUTTON);
    assert(!sc_touchmap_editor_try_start_drag(
        &editor, map, (struct sc_point) {300, 400}));
    sc_touchmap_editor_set_mode(&editor, SC_TOUCHMAP_EDITOR_MODE_ADD_MENU);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_ADD_MENU);
    sc_touchmap_editor_set_mode(&editor, SC_TOUCHMAP_EDITOR_MODE_PLACE_SKILL);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_PLACE_SKILL);
    sc_touchmap_editor_set_mode(&editor, SC_TOUCHMAP_EDITOR_MODE_PLACE_WALK);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_PLACE_WALK);
    sc_touchmap_editor_set_mode(&editor, SC_TOUCHMAP_EDITOR_MODE_SELECT);

    assert(!sc_touchmap_editor_nudge_selection(&editor, map, 1, 0, 1));

    editor.selection.target = SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER;
    editor.selection.button_index = -1;
    assert(sc_touchmap_editor_nudge_selection(&editor, map, -1, 0, -1));
    assert(map->walk.center.x == 99);
    assert(map->walk.center.y == 200);
    assert(map->walk.current_pos.x == 99);
    assert(map->walk.current_pos.y == 200);
    assert(map->walk.radius == 40);

    editor.selection.target = SC_TOUCHMAP_EDITOR_TARGET_WALK_RADIUS;
    assert(sc_touchmap_editor_nudge_selection(&editor, map, 0, -1, 10));
    assert(map->walk.radius == 50);
    assert(sc_touchmap_editor_nudge_selection(&editor, map, 0, 1, -100));
    assert(map->walk.radius == SC_TOUCHMAP_MIN_RADIUS);
    assert(!sc_touchmap_editor_nudge_selection(&editor, map, 0, 1, -1));

    editor.selection.target = SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER;
    editor.selection.button_index = 0;
    assert(sc_touchmap_editor_nudge_selection(&editor, map, 10, -10, 10));
    assert(map->buttons[0].center.x == 310);
    assert(map->buttons[0].center.y == 390);
    assert(map->buttons[0].current_pos.x == 310);
    assert(map->buttons[0].current_pos.y == 390);
    assert(map->buttons[0].radius == 0);

    editor.selection.target = SC_TOUCHMAP_EDITOR_TARGET_BUTTON_RADIUS;
    assert(!sc_touchmap_editor_nudge_selection(&editor, map, 0, 0, 10));
    assert(map->buttons[0].radius == 0);

    editor.selection.button_index = 1;
    assert(sc_touchmap_editor_nudge_selection(&editor, map, 0, 0, 10));
    assert(map->buttons[1].radius == 60);

    sc_touchmap_editor_select_button(&editor, 0);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_SELECT);
    assert(editor.selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER);
    assert(editor.selection.button_index == 0);

    sc_touchmap_editor_select_walk(&editor);
    assert(sc_touchmap_editor_get_mode(&editor)
           == SC_TOUCHMAP_EDITOR_MODE_SELECT);
    assert(editor.selection.target == SC_TOUCHMAP_EDITOR_TARGET_WALK_CENTER);
    assert(editor.selection.button_index == -1);

    sc_touchmap_editor_select_after_button_remove(&editor, map, 1);
    assert(editor.selection.target == SC_TOUCHMAP_EDITOR_TARGET_BUTTON_CENTER);
    assert(editor.selection.button_index == 1);

    sc_touchmap_editor_select_after_button_remove(&editor, map, -1);
    assert(editor.selection.target == SC_TOUCHMAP_EDITOR_TARGET_NONE);
    assert(editor.selection.button_index == -1);

    sc_gptm_gamepad_touchmap_destroy(map);
}

int
main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    test_parse_save_preserves_metadata();
    test_empty_touchmap_parse_save();
    test_touchmap_package_metadata_helpers();
    test_empty_touchmap_create_save();
    test_touchmap_mutation_helpers();
    test_overlay_coordinate_transforms();
    test_editor_keyboard_nudging();
    return 0;
}
