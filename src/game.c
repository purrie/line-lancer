#include "game.h"

void spawn_unit(GameState * state, Building * building) {
    Unit * unit = unit_from_building(building);
    if (unit == NULL) {
        TraceLog(LOG_ERROR, "Couldn't create unit from building");
        return;
    }
    listUnitAppend(&state->units, unit);
    TraceLog(LOG_INFO, "Unit spawned for player %zu", unit->player_owned);
}

void spawn_units(GameState * state, float delta_time) {
    for (usize r = 0; r < state->current_map->regions.len; r++) {
        ListBuilding * buildings = &state->current_map->regions.items[r].buildings;

        for (usize b = 0; b < buildings->len; b++) {
            Building * building = &buildings->items[b];
            if (building->type == BUILDING_EMPTY)
                continue;

            building->spawn_timer += delta_time;

            if (building->spawn_timer >= 6.0f) {
                building->spawn_timer = 0.0f;
                spawn_unit(state, building);
            }
        }
    }
}

void update_unit_state(GameState * state) {
    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        switch (unit->state) {
            case UNIT_STATE_APPROACHING_ROAD: {
                float dist = Vector2DistanceSqr(unit->position, path_start_point(unit->path, unit->region));
                if (dist < 1.0f) {
                    unit->state = UNIT_STATE_FOLLOWING_ROAD;
                }
                // TODO check if enemies are in the region, if there are, switch to defending
            } break;
            case UNIT_STATE_FOLLOWING_ROAD: {
                float length = path_length(unit->path);
                if (length <= unit->progress) {
                    Region * r = path_end_region(unit->path, unit->region);
                    Path * p = region_redirect_path(r, unit->path);
                    unit->path = p;
                    unit->region = r;
                    unit->progress = 0.0f;
                    if (r->player_owned.has_value == false || unit->player_owned != r->player_owned.value) {
                        unit->state = UNIT_STATE_ATTACKING_REGION;
                        TraceLog(LOG_INFO, "Switching to attack");
                    }
                    else {
                        unit->state = UNIT_STATE_APPROACHING_ROAD;
                        TraceLog(LOG_INFO, "Switching to another road");
                    }
                }
            } break;
            case UNIT_STATE_ATTACKING: {
                // fighting enemies on path
            } break;
            case UNIT_STATE_HURT: {
                // getting hit animation
            } break;
            case UNIT_STATE_DEFENDING_REGION: {
                // attacking enemies attacking the region
            } break;
            case UNIT_STATE_ATTACKING_REGION: {
                // attacking region guardian
            } break;
        }
    }
}

void move_units(GameState * state, float delta_time) {
    float unit_speed = 10.0f; // TODO make unit speed into proper value
    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        switch (unit->state) {
            case UNIT_STATE_APPROACHING_ROAD: {
                Vector2 target = path_start_point(unit->path, unit->region);
                unit->position = Vector2MoveTowards(unit->position, target, unit_speed * delta_time);
            } break;
            case UNIT_STATE_FOLLOWING_ROAD: {
                unit->progress += unit_speed * delta_time;
                OptionalVector2 move = path_follow(unit->path, unit->region, unit->progress);
                if (move.has_value) {
                    unit->position = move.value;
                }
                else {
                    TraceLog(LOG_WARNING, "No more path");
                }
            } break;
            case UNIT_STATE_DEFENDING_REGION: {
            } break;
            case UNIT_STATE_ATTACKING_REGION: {
            } break;
            default: break;
        }
    }
}

void units_fight(GameState * state, float delta_time) {

}

void simulate_units(GameState * state) {
    float dt = GetFrameTime();
    spawn_units(state, dt);
    move_units(state, dt);
    units_fight(state, dt);
    update_unit_state(state);
}
