#include "game.h"
#include "units.h"
#include "level.h"
#include "constants.h"
#include <raymath.h>


void spawn_unit(GameState * state, Building * building) {
    Unit * unit = unit_from_building(building);
    if (unit == NULL) {
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

            if (building->spawn_timer >= BUILDING_SPAWN_INTERVAL) {
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
            case UNIT_STATE_MOVING: {
                if (can_unit_progress(unit)) {
                    if (get_enemy_in_range(unit)) {
                        unit->state = UNIT_STATE_FIGHTING;
                    }
                    else {
                        if(move_unit_forward(unit)) {
                            // TODO This fixes units getting stuck at friendly guardian but makes them bounce back instead of passing friendies
                            // Leaving as is for now, if it causes issues, can change later
                            unit->move_direction = ! unit->move_direction;
                        }
                    }
                }
            } break;
            case UNIT_STATE_GUARDING: {
                // do nothing, guardians are always in this state, regular units never enter this state
            } break;
            case UNIT_STATE_FIGHTING: {
                if (get_enemy_in_range(unit) == NULL) {
                    unit->state = UNIT_STATE_MOVING;
                }
            } break;
        }
    }
}

void move_units(GameState * state, float delta_time) {
    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        switch (unit->state) {
            case UNIT_STATE_MOVING: {
                unit->position = Vector2MoveTowards(unit->position, unit->location->position, UNIT_SPEED * delta_time);
            } break;
            default: break;
        }
    }
}

void units_fight(GameState * state, float delta_time) {
    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];
        if (unit->state != UNIT_STATE_FIGHTING)
            continue;
        Unit * target = get_enemy_in_range(unit);
        if (target) {
            target->health -= get_unit_attack(unit) * delta_time;
            if (target->health > 0.0f) {
                continue;
            }
            if (target->state == UNIT_STATE_GUARDING) {
                Region * region = region_by_guardian(&state->current_map->regions, target);
                region_change_ownership(region, unit->player_owned);
            }
            else {
                usize target_index = destroy_unit(&state->units, target);
                if (target_index < i) i--;
            }
        }
    }
}

void guardian_fight(GameState * state, float delta_time) {
    for (usize i = 0; i < state->current_map->regions.len; i++) {
        Region * region = &state->current_map->regions.items[i];

        for (usize p = 0; p < region->paths.len; p++) {
            PathEntry * entry = &region->paths.items[p];
            Node * node = entry->castle_path.end;

            while (node != entry->castle_path.start) {
                if (node->unit && node->unit->player_owned != region->player_id) {
                    node->unit->health -= get_unit_attack(&region->castle.guardian) * delta_time;
                    if (node->unit->health <= 0.0f) {
                        destroy_unit(&state->units, node->unit);
                    }
                }
                node = node->previous;
            }
        }
    }
}

void simulate_units(GameState * state) {
    float dt = GetFrameTime();

    spawn_units       (state, dt);
    move_units        (state, dt);
    units_fight       (state, dt);
    guardian_fight    (state, dt);
    update_unit_state (state);
}
