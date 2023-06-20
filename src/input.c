#include "input.h"
#include "ui.h"
#include "level.h"

void state_none(Map *const map, GameState * state) {
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetMousePosition();
    Building * b = get_building_by_position(map, cursor);
    if (b) {
        state->current_input = INPUT_CLICKED_BUILDING;
        state->selected_building = b;
        state->selected_region = b->region;
        return;
    }
    Movement dir;
    Path * p = path_on_point(map, cursor, &dir);
    if (p) {
        state->selected_path = p;
        if (dir == MOVEMENT_DIR_FORWARD)
            state->selected_region = p->region_a;
        else
            state->selected_region = p->region_b;
        state->current_input = INPUT_CLICKED_PATH;
        return;
    }
}

void state_clicked_building (Map *const map, GameState * state) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetMousePosition();
    Building * b = get_building_by_position(map, cursor);
    if (b && b == state->selected_building) {
        state->current_input = INPUT_OPEN_BUILDING;
        return;
    }

    Path * path = path_on_point(map, cursor, NULL);
    if (path) {
        for (usize i = 0; i < state->selected_building->spawn_paths.len; i++) {
            Bridge bridge = state->selected_building->spawn_paths.items[i];
            if (bridge.end->next == path->bridge.end || bridge.end->next == path->bridge.start) {
                state->selected_building->active_spawn = i;
                break;
            }
        }
    }

    state->selected_region = NULL;
    state->selected_building = NULL;
    state->current_input = INPUT_NONE;
}

void state_clicked_path (Map *const map, GameState * state) {
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetMousePosition();
    Movement dir;
    Path * path = path_on_point(map, cursor, &dir);
    if (path == NULL) {
        goto clear;
    }

    for (usize f = 0; f < state->selected_region->paths.len; f++) {
        PathEntry * entry = &state->selected_region->paths.items[f];

        if (entry->path != state->selected_path) {
            continue;
        }

        for (usize s = 0; s < entry->redirects.len; s++) {
            if (entry->redirects.items[s].to == path) {
                entry->active_redirect = s;
                Node * node = path_start_node(entry->path, state->selected_region, &dir);
                if (dir == MOVEMENT_DIR_FORWARD) {
                    node->previous = entry->redirects.items[s].bridge->start;
                }
                else {
                    node->next = entry->redirects.items[s].bridge->start;
                }
                goto clear;
            }
        }
        break;
    }

    clear:
    state->selected_path = NULL;
    state->selected_region = NULL;
    state->current_input = INPUT_NONE;
}

void state_building(Map *const map, GameState * state) {
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
        place_building(state->selected_building, type);
        state->current_input = INPUT_NONE;
        state->selected_building = NULL;
    }
    else {
        BuildingAction action;
        if (ui_building_action_click(state, cursor, &action)) {
            Building * b = get_building_by_position(map, cursor);
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
                upgrade_building(state->selected_building);
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

void update_input_state(Map *const map, GameState * state) {
    switch (state->current_input) {
        case INPUT_NONE:             return state_none             (map, state);
        case INPUT_CLICKED_BUILDING: return state_clicked_building (map, state);
        case INPUT_CLICKED_PATH:     return state_clicked_path     (map, state);
        case INPUT_OPEN_BUILDING:    return state_building         (map, state);
    }

    TraceLog(LOG_ERROR, "Encountered unknown game state while handling input");
}
