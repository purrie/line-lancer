#include "game.h"
#include "units.h"
#include "level.h"
#include <raymath.h>

Unit * get_enemy_in_range(ListUnit *const units, Unit *const unit) {
    float range;
    switch (unit->type) {
        case UNIT_FIGHTER: {
            range = 15.0f;
        } break;
        case UNIT_ARCHER: {
            range = 30.0f;
        } break;
        case UNIT_SUPPORT: {
            range = 20.0f;
        } break;
        case UNIT_SPECIAL: {
            range = 15.0f;
        } break;
        default: {
            range = 15.0f;
            TraceLog(LOG_WARNING, "Unhandled unit type in enemy check %d", unit->type);
        } break;
    }
    range *= range;
    for (usize i = 0; i < units->len; i++) {
        if (units->items[i] == unit) continue;
        if (unit->player_owned != units->items[i]->player_owned && Vector2DistanceSqr(units->items[i]->position, unit->position) < range) {
            return units->items[i];
        }
    }
    return NULL;
}

bool collision_free(ListUnit *const units, Vector2 target, usize index) {
    float collision_size = 6.0f;
    for (usize c = 0; c < units->len; c++) {
        if (c == index) continue;
        if (CheckCollisionCircles(target, collision_size, units->items[c]->position, collision_size)) {
            return false;
        }
    }
    return true;
}

void spawn_unit(GameState * state, Building * building) {
    // TODO check if there's a space for the building to spawn the unit at
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

usize find_unit(ListUnit * units, Unit * unit) {
    for (usize a = 0; a < units->len; a++) {
        if (units->items[a] == unit) {
            return a;
        }
    }
    return units->len;
}

void update_unit_state(GameState * state) {
    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        switch (unit->state) {
            case UNIT_STATE_APPROACHING_ROAD: {
                Unit * enemy = get_enemy_in_range(&state->units, unit);
                if (enemy) {
                    unit->state = UNIT_STATE_ATTACKING;
                    unit->target = enemy;
                    continue;
                }
                float dist = Vector2DistanceSqr(unit->position, path_start_point(unit->path, unit->region));
                if (dist < 1.0f) {
                    unit->state = UNIT_STATE_FOLLOWING_ROAD;
                }
                // TODO check if enemies are in the region, if there are, switch to defending
            } break;
            case UNIT_STATE_FOLLOWING_ROAD: {
                Unit * enemy = get_enemy_in_range(&state->units, unit);
                if (enemy) {
                    unit->state = UNIT_STATE_ATTACKING;
                    unit->target = enemy;
                    continue;
                }
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
                Unit * enemy = get_enemy_in_range(&state->units, unit);
                if (enemy == NULL) {
                    unit->state = UNIT_STATE_FOLLOWING_ROAD;
                    unit->target = enemy;
                    continue;
                }
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
    float unit_speed = 20.0f; // TODO make unit speed into proper value
    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        switch (unit->state) {
            case UNIT_STATE_APPROACHING_ROAD: {
                // TODO move the units in an arch because they block themselves at the entrance to the path if the angle from the building is too steep
                Vector2 target = path_start_point(unit->path, unit->region);
                target = Vector2MoveTowards(unit->position, target, unit_speed * delta_time);

                if (collision_free(&state->units, target, u))
                    unit->position = target;
            } break;
            case UNIT_STATE_FOLLOWING_ROAD: {
                float progress = unit->progress + unit_speed * delta_time;
                OptionalVector2 move = path_follow(unit->path, unit->region, progress);
                if (move.has_value) {
                    if (collision_free(&state->units, move.value, u)) {
                        unit->position = move.value;
                        unit->progress = progress;
                    }
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
    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];
        if (unit->target) {
            usize target_index = find_unit(&state->units, unit->target);
            if (target_index >= state->units.len) {
                continue;
            }
            unit->target->health -= get_unit_attack(unit) * delta_time;
            if (unit->target->health < 0.0f) {
                listUnitRemove(&state->units, target_index);
                if (target_index < i) i--;
                MemFree(unit->target);
                unit->target = NULL;
            }
        }
    }
}

void simulate_units(GameState * state) {
    float dt = GetFrameTime();

    spawn_units       (state, dt);
    move_units        (state, dt);
    units_fight       (state, dt);
    update_unit_state (state);
}
