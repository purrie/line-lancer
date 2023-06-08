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

    Rectangle dialog = ui_empty_building_dialog_rect(state->selected_building->position);
    Vector2 cursor = GetMousePosition();
    if (CheckCollisionPointRec(cursor, dialog)) {
        BuildingType build = ui_get_empty_building_dialog_click(state->selected_building->position);

        if (build != BUILDING_EMPTY) {
            place_building(state->selected_building, build);
            state->current_input = INPUT_NONE;
            state->selected_building = NULL;
        }
    }
    else {
        Building * b = get_building_by_position(map, cursor);
        if (b) {
            state->selected_building = b;
        } else {
            state->current_input = INPUT_NONE;
            state->selected_building = NULL;
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
