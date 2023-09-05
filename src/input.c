#include "input.h"
#include "ui.h"
#include "level.h"
#include "constants.h"
#include <raymath.h>

void clamp_camera (GameState * state) {
    Vector2 map = { state->map.width, state->map.height };
    Vector2 limit_top    = map;
    Vector2 limit_bottom = Vector2Zero();

    Vector2 screen = (Vector2) { GetScreenWidth(), GetScreenHeight() };
    Vector2 pixels = Vector2Divide(screen, (Vector2) { state->camera.zoom, state->camera.zoom });
    Vector2 diff = Vector2Subtract(pixels, map);

    diff.x = diff.x < 0.0f ? 0.0f : diff.x * 0.5f;
    diff.y = diff.y < 0.0f ? 0.0f : diff.y * 0.5f;

    limit_top = Vector2Add(limit_top, diff);
    limit_bottom = Vector2Subtract(limit_bottom, diff);

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
    clamp_camera(state);
}

void state_none (GameState * state) {
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == false) {
        return;
    }
    state->current_input = INPUT_MOVE_MAP;
    state->selected_point = GetMousePosition();

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

    if (path_by_position(&state->map, cursor, &state->selected_path)) {
        return;
    }
    if (region_by_position(&state->map, cursor, &state->selected_region)) {
        return;
    }
    state->current_input = INPUT_CLICKED_PATH;
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
                state->selected_region->active_path = state->selected_region->paths.len;
            }
            else {
                state->selected_region->active_path = p;
            }
            for (usize w = 0; w < state->selected_region->nav_graph.waypoints.len; w++) {
                WayPoint * point = state->selected_region->nav_graph.waypoints.items[w];
                if (point && point->unit && point->unit->player_owned == state->selected_region->player_id) {
                    point->unit->pathfind.len = 0;
                }
            }
            break;
        }
    }
    // Vector2 cursor = GetScreenToWorld2D(GetMousePosition(), state->camera);
    state->selected_path = NULL;
    state->selected_region = NULL;
    state->current_input = INPUT_NONE;
}
void state_building (GameState * state) {
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
            place_building(state->selected_building, type);
            if (IsKeyDown(KEY_LEFT_SHIFT) == false)
                player->resource_gold -= cost;
        }
        #else
        if (player->resource_gold >= cost) {
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
                    upgrade_building(state->selected_building);
                    player->resource_gold -= cost;
                }
                state->current_input = INPUT_NONE;
                state->selected_building = NULL;
            } break;
            case BUILDING_ACTION_DEMOLISH: {
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
