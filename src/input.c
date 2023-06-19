#include "input.h"
#include "ui.h"
#include "level.h"

void place_building(Building * building, BuildingType type) {
    building->type = type;
    // TODO make proper building modification
    // update texture and whatnot
}

void upgrade_building(Building * building) {
    if (building->upgrades >= 2) return;

    building->upgrades ++;
    // TODO make proper building modification
}

void demolish_building (Building * building) {
    building->type = BUILDING_EMPTY;
    building->upgrades = 0;
}

void state_none(Map *const map, GameState * state) {
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) == false) {
        return;
    }

    Vector2 cursor = GetMousePosition();
    Building * b = get_building_by_position(map, cursor);
    if (b) {
        state->current_input = INPUT_OPEN_BUILDING;
        state->selected_building = b;
    }
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

void state_start_path(Map *const map, GameState * state) {
}

void state_end_path(Map *const map, GameState * state) {
}

void update_input_state(Map *const map, GameState * state) {
    switch (state->current_input) {
        case INPUT_NONE:           return state_none       (map, state);
        case INPUT_OPEN_BUILDING:  return state_building   (map, state);
        case INPUT_START_SET_PATH: return state_start_path (map, state);
        case INPUT_SET_PATH:       return state_end_path   (map, state);
    }

    TraceLog(LOG_ERROR, "Encountered unknown game state while handling input");
}
