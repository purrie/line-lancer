#include "input.h"
#include "ui.h"
#include "level.h"
#include "constants.h"
#include "audio.h"
#include <raymath.h>

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
    Building * b = get_building_by_position(&state->map, cursor);
    if (b) {
        #ifdef DEBUG
        if (b->region->player_id != player && IsKeyUp(KEY_LEFT_SHIFT))
            return;
        #else
        if (b->region->player_id != player)
            return;
        #endif
        state->current_input = INPUT_CLICKED_BUILDING;
        state->selected_building = b;
        state->selected_region = b->region;
        return;
    }

    Region * region = state->selected_region;
    if (region_by_position(&state->map, cursor, &region)) {
        return;
    }
    if (region->player_id != player) return;
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
            state->selected_region = region;
            state->selected_path = path;
            state->current_input = INPUT_CLICKED_PATH;
            return;
        }
    }
}
void state_clicked_building (GameState * state) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetScreenToWorld2D(GetMousePosition(), state->camera);
    Building * b = get_building_by_position(&state->map, cursor);
    if (b && b == state->selected_building) {
        state->current_input = INPUT_OPEN_BUILDING;
        return;
    }

    state->selected_region = NULL;
    state->selected_building = NULL;
    state->current_input = INPUT_NONE;
}
void state_clicked_path (GameState * state) {
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
    if (state->selected_building->type == BUILDING_EMPTY) {
        BuildingType type;
        if (ui_building_buy_click(state, cursor, &type)) {
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
        if (ui_building_action_click(state, cursor, &action)) {
            Building * b = get_building_by_position(&state->map, cursor);
            if (b) {
                state->selected_building = b;
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

void update_input_state (GameState * state) {
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
