#include "input.h"
#include "ui.h"
#include "level.h"
#include "constants.h"
#include "audio.h"
#include <raymath.h>

Vector2 mouse_position_inworld (Camera2D view) {
    #if defined(ANDROID)
    Vector2 point = (Vector2) { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    #else
    Vector2 point = GetMousePosition();
    #endif
    return GetScreenToWorld2D(point, view);
}
Vector2 mouse_position_pointer () {
    #if defined(ANDROID)
    Vector2 point = (Vector2) { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };
    #else
    Vector2 point = GetMousePosition();
    #endif
    return point;
}

void clamp_camera (GameState * state) {
    Vector2 limit_top    = { state->map.width, state->map.height };
    Vector2 limit_bottom = Vector2Zero();

    state->camera.offset = (Vector2) { GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f };

    if (state->camera.target.x > limit_top.x)
        state->camera.target.x = limit_top.x;
    if (state->camera.target.y > limit_top.y)
        state->camera.target.y = limit_top.y;

    if (state->camera.target.x < limit_bottom.x)
        state->camera.target.x = limit_bottom.x;
    if (state->camera.target.y < limit_bottom.y)
        state->camera.target.y = limit_bottom.y;
}
void camera_zoom (GameState * state) {
    float wheel = GetMouseWheelMove();
    state->camera.zoom += wheel * 0.1f;

    Vector2 map_size;
    map_size.x = (GetScreenWidth()  - 20.0f) / (float)state->map.width;
    map_size.y = (GetScreenHeight() - 20.0f - state->settings->theme.info_bar_height) / (float)state->map.height;
    float minz = (map_size.x < map_size.y) ? map_size.x : map_size.y;
    if (state->camera.zoom < minz) state->camera.zoom = minz;

    float maxz = 10.0f;
    if (state->camera.zoom > maxz) state->camera.zoom = maxz;

    clamp_camera(state);
}
Test camera_control_android (GameState * state) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        CameraJoystick joy = android_camera_control(&state->settings->theme);
        Vector2 point = GetMousePosition();
        if (CheckCollisionPointRec(point, joy.zoom_in)) {
            Range zoom_range = map_zoom_range(state);
            float in = GetFrameTime() + state->camera.zoom;
            state->camera.zoom = (in > zoom_range.max) ? zoom_range.max : in;
            return YES;
        }
        if (CheckCollisionPointRec(point, joy.zoom_out)) {
            Range zoom_range = map_zoom_range(state);
            float in = state->camera.zoom - GetFrameTime();
            state->camera.zoom = (in < zoom_range.min) ? zoom_range.min : in;
            return YES;
        }
        if (CheckCollisionPointRec(point, joy.joystick)) {
            Vector2 center = { joy.joystick.x + joy.joystick.width * 0.5f,
                               joy.joystick.y + joy.joystick.height * 0.5f };
            Vector2 drag = Vector2Subtract(point, center);
            drag = Vector2Scale(drag, GetFrameTime() * 4.0f);
            state->camera.target = Vector2Add(state->camera.target, drag);
            clamp_camera(state);
            return YES;
        }
    }
    return NO;
}

void state_none (GameState * state) {
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == false) {
        return;
    }
    state->selected_point = GetMousePosition();
    if (state->selected_point.y < state->settings->theme.info_bar_height) {
        return;
    }
    state->current_input = INPUT_MOVE_MAP;

    usize player;
    if (get_local_player_index(state, &player)) {
        #ifdef DEBUG
        if (IsKeyDown(KEY_LEFT_SHIFT))
            player = 1;
        else
            return;
        #else
        return;
        #endif
    }

    Vector2 cursor = GetScreenToWorld2D(state->selected_point, state->camera);
    Region * region = state->selected_region;
    if (region_by_position(&state->map, cursor, &region)) {
        return;
    }
    if (region->player_id != player) {
        #ifdef DEBUG
        if (IsKeyUp(KEY_LEFT_SHIFT)) {
            return;
        }
        #else
        return;
        #endif
    }

    for (usize i = 0; i < region->buildings.len; i++) {
        Building * b = &region->buildings.items[i];
        if (CheckCollisionPointCircle(cursor, b->position, PLAYER_SELECTION_RADIUS)) {
            state->current_input = INPUT_CLICKED_BUILDING;
            state->selected_building = b;
            state->selected_region = region;
            return;
        }
    }

    for (usize i = 0; i < region->paths.len; i++) {
        Path * path = region->paths.items[i];
        Vector2 point;
        if (path->region_a == region) {
            point = path->lines.items[0].a;
        }
        else {
            point = path->lines.items[path->lines.len - 1].b;
        }
        if (CheckCollisionPointCircle(cursor, point, PLAYER_SELECTION_RADIUS)) {
            state->selected_region = region;
            state->selected_path = path;
            state->current_input = INPUT_CLICKED_PATH;
            return;
        }
    }
}
void state_clicked_building (GameState * state) {
    Vector2 mouse = GetMousePosition();
    float distance = Vector2Distance(mouse, state->selected_point);
    if (distance > 1) {
        state->current_input = INPUT_MOVE_MAP;
        return;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetScreenToWorld2D(GetMousePosition(), state->camera);
    Building * b = get_building_by_position(&state->map, cursor, PLAYER_SELECTION_RADIUS);
    if (b && b == state->selected_building) {
        state->current_input = INPUT_OPEN_BUILDING;
        return;
    }

    state->selected_region = NULL;
    state->selected_building = NULL;
    state->current_input = INPUT_NONE;
}
void state_clicked_path (GameState * state) {
    Vector2 mouse = GetMousePosition();
    float distance = Vector2Distance(mouse, state->selected_point);
    if (distance > 1) {
        state->current_input = INPUT_MOVE_MAP;
        return;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    for (usize p = 0; p < state->selected_region->paths.len; p++) {
        if (state->selected_path == state->selected_region->paths.items[p]) {
            if (state->selected_region->active_path == p) {
                play_sound(state->resources, SOUND_FLAG_DOWN);
                state->selected_region->active_path = state->selected_region->paths.len;
            }
            else {
                play_sound(state->resources, SOUND_FLAG_UP);
                state->selected_region->active_path = p;
            }
            region_reset_unit_pathfinding(state->selected_region);
            break;
        }
    }
    // Vector2 cursor = GetScreenToWorld2D(GetMousePosition(), state->camera);
    state->selected_path = NULL;
    state->selected_region = NULL;
    state->current_input = INPUT_NONE;
}
void state_building (GameState * state) {
    usize player_id;
    if (get_local_player_index(state, &player_id)) {
        #ifdef DEBUG
        player_id = 1;
        #else
        state->selected_building = NULL;
        state->current_input = INPUT_NONE;
        return;
        #endif
    }
    if (player_id != state->selected_building->region->player_id) {
        #ifdef DEBUG
        if (IsKeyDown(KEY_LEFT_SHIFT) == 0) {
            state->selected_building = NULL;
            state->current_input = INPUT_NONE;
            return;
        }
        #else
        state->selected_building = NULL;
        state->current_input = INPUT_NONE;
        return;
        #endif
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetMousePosition();
    Vector2 ui_box = GetWorldToScreen2D(state->selected_building->position, state->camera);
    if (state->selected_building->type == BUILDING_EMPTY) {
        BuildingType type;
        EmptyDialog dialog = empty_dialog(ui_box, &state->settings->theme);
        if (ui_building_buy_click(dialog, cursor, &type)) {
            state->current_input = INPUT_NONE;
            state->selected_building = NULL;
            return;
        }
        usize cost = building_buy_cost(type);
        PlayerData * player = &state->players.items[state->selected_building->region->player_id];
        #ifdef DEBUG
        if (player->resource_gold >= cost || IsKeyDown(KEY_LEFT_SHIFT)) {
            play_sound(state->resources, SOUND_BUILDING_BUILD);
            place_building(state->selected_building, type);
            if (IsKeyDown(KEY_LEFT_SHIFT) == false)
                player->resource_gold -= cost;
        }
        #else
        if (player->resource_gold >= cost) {
            play_sound(state->resources, SOUND_BUILDING_BUILD);
            place_building(state->selected_building, type);
            player->resource_gold -= cost;
        }
        #endif
        state->current_input = INPUT_NONE;
        state->selected_building = NULL;
    }
    else {
        BuildingAction action;
        BuildingDialog dialog = building_dialog(ui_box, &state->settings->theme);
        if (ui_building_action_click(dialog, cursor, &action)) {
            // click outside the ui, test if we have another building clicked
            Building * b = get_building_by_position(&state->map, cursor, PLAYER_SELECTION_RADIUS);
            if (b) {
                state->selected_building = b;
                state->selected_region = b->region;
            }
            else {
                state->current_input = INPUT_NONE;
                state->selected_building = NULL;
            }
            return;
        }

        switch (action) {
            case BUILDING_ACTION_NONE: {
            } break;
            case BUILDING_ACTION_UPGRADE: {
                usize cost = building_upgrade_cost(state->selected_building);
                PlayerData * player = &state->players.items[state->selected_building->region->player_id];
                if (player->resource_gold >= cost) {
                    play_sound(state->resources, SOUND_BUILDING_UPGRADE);
                    upgrade_building(state->selected_building);
                    player->resource_gold -= cost;
                }
                state->current_input = INPUT_NONE;
                state->selected_building = NULL;
            } break;
            case BUILDING_ACTION_DEMOLISH: {
                play_sound(state->resources, SOUND_BUILDING_DEMOLISH);
                demolish_building(state->selected_building);
                state->current_input = INPUT_NONE;
                state->selected_building = NULL;
            } break;
        }
    }
}
void state_map_movement (GameState * state) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        state->current_input = INPUT_NONE;
        return;
    }

    Vector2 cursor = GetMousePosition();
    Vector2 move = Vector2Subtract(cursor, state->selected_point);
    move = Vector2Scale(move, 1.0f / state->camera.zoom);
    state->camera.target = Vector2Subtract(state->camera.target, move);
    state->selected_point = cursor;

    clamp_camera(state);
}

void update_input_state_pc (GameState * state) {
    camera_zoom(state);
    switch (state->current_input) {
        case INPUT_NONE:             return state_none             (state);
        case INPUT_CLICKED_BUILDING: return state_clicked_building (state);
        case INPUT_CLICKED_PATH:     return state_clicked_path     (state);
        case INPUT_OPEN_BUILDING:    return state_building         (state);
        case INPUT_MOVE_MAP:         return state_map_movement     (state);
    }

    TraceLog(LOG_ERROR, "Encountered unknown game state while handling input");
}
void find_pointed_thing (GameState * state, usize player) {
    Vector2 cursor = mouse_position_inworld(state->camera);
    state->current_input = INPUT_NONE;

    Region * region;
    if (region_by_position(&state->map, cursor, &region)) {
        clear_state:
        state->selected_region = NULL;
        state->selected_path = NULL;
        state->selected_building = NULL;
        return;
    }
    if (region->player_id != player) goto clear_state;

    state->selected_region = region;

    for (usize i = 0; i < region->buildings.len; i++) {
        Building * building = &region->buildings.items[i];
        if (CheckCollisionPointCircle(cursor, building->position, PATH_THICKNESS)) {
            state->selected_building = building;
            state->current_input = INPUT_CLICKED_BUILDING;
            return;
        }
    }

    for (usize i = 0; i < region->paths.len; i++) {
        Path * path = region->paths.items[i];
        Vector2 point;
        if (path->region_a == region) {
            point = path->lines.items[0].a;
        }
        else {
            point = path->lines.items[path->lines.len - 1].b;
        }
        if (CheckCollisionPointCircle(cursor, point, PATH_THICKNESS)) {
            state->selected_path = path;
            state->current_input = INPUT_CLICKED_PATH;
            return;
        }
    }

}
void update_input_state_android (GameState * state) {
    usize player = 0;
    if (get_local_player_index(state, &player)) {
        if (state->current_input == INPUT_MOVE_MAP) {
            state_map_movement(state);
            return;
        }
        state->selected_point = GetMousePosition();

        if (camera_control_android(state)) {
            return;
        }

        if (state->selected_point.y < state->settings->theme.info_bar_height) {
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            state->current_input = INPUT_MOVE_MAP;

        return;
    }

    if (state->current_input != INPUT_MOVE_MAP) {
        if (camera_control_android(state)) {
            find_pointed_thing(state, player);
            return;
        }
    }

    // handle current state logic
    switch (state->current_input) {
        case INPUT_MOVE_MAP: {
            state_map_movement(state);
            if (state->current_input != INPUT_MOVE_MAP) {
                state->selected_point = GetMousePosition();
                find_pointed_thing(state, player);
            }
        } return;
        case INPUT_CLICKED_BUILDING: if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 cursor = GetMousePosition();
            if (state->selected_building->type == BUILDING_EMPTY) {
                BuildingType type;
                EmptyDialog dialog = empty_dialog(Vector2Zero(), &state->settings->theme);
                if (ui_building_buy_click(dialog, cursor, &type)) {
                    state->selected_point = GetMousePosition();
                    state->current_input = INPUT_MOVE_MAP;
                    return;
                }
                usize cost = building_buy_cost(type);
                PlayerData * player_data = &state->players.items[player];
                if (player_data->resource_gold >= cost) {
                    play_sound(state->resources, SOUND_BUILDING_BUILD);
                    place_building(state->selected_building, type);
                    player_data->resource_gold -= cost;
                }
            }
            else { // not empty
                BuildingAction action = BUILDING_ACTION_NONE;
                BuildingDialog dialog = building_dialog(Vector2Zero(), &state->settings->theme);
                if (ui_building_action_click(dialog, cursor, &action)) {
                    state->selected_point = GetMousePosition();
                    state->current_input = INPUT_MOVE_MAP;
                    return;
                }

                switch (action) {
                    case BUILDING_ACTION_NONE: {} break;
                    case BUILDING_ACTION_UPGRADE: {
                        usize cost = building_upgrade_cost(state->selected_building);
                        PlayerData * player_data = &state->players.items[player];
                        if (player_data->resource_gold >= cost) {
                            play_sound(state->resources, SOUND_BUILDING_UPGRADE);
                            upgrade_building(state->selected_building);
                            player_data->resource_gold -= cost;
                        }
                    } break;
                    case BUILDING_ACTION_DEMOLISH: {
                        play_sound(state->resources, SOUND_BUILDING_DEMOLISH);
                        demolish_building(state->selected_building);
                    } break;
                }
            }
        } break;
        case INPUT_CLICKED_PATH: if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 cursor = GetMousePosition();
            Rectangle button = flag_button_position();
            Vector2 center = { button.x + button.width * 0.5f, button.y + button.height * 0.5f };
            float larger = (button.height > button.width) ? button.height : button.width;
            if (CheckCollisionPointCircle(cursor, center, larger)) {
                for (usize p = 0; p < state->selected_region->paths.len; p++) {
                    if (state->selected_path == state->selected_region->paths.items[p]) {
                        if (state->selected_region->active_path == p) {
                            play_sound(state->resources, SOUND_FLAG_DOWN);
                            state->selected_region->active_path = state->selected_region->paths.len;
                        }
                        else {
                            play_sound(state->resources, SOUND_FLAG_UP);
                            state->selected_region->active_path = p;
                        }
                        region_reset_unit_pathfinding(state->selected_region);
                        break;
                    }
                }
            }
            else {
                state->selected_point = cursor;
                state->current_input = INPUT_MOVE_MAP;
            }
        } break;
        default: if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            state->selected_point = GetMousePosition();
            state->current_input = INPUT_MOVE_MAP;
        } break;
    }

}
void update_input_state (GameState * state) {
    #if defined(ANDROID)
    update_input_state_android(state);
    #else
    update_input_state_pc(state);
    #endif
}
