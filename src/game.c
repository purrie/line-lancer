#include "game.h"
#include "assets.h"
#include "std.h"
#include "units.h"
#include "level.h"
#include "constants.h"
#include <raymath.h>


void spawn_unit (GameState * state, Building * building) {
    Unit * unit = unit_from_building(building);
    if (unit == NULL) {
        return;
    }
    listUnitAppend(&state->units, unit);
    TraceLog(LOG_INFO, "Unit spawned for player %zu", unit->player_owned);
}

void spawn_units (GameState * state) {
    for (usize r = 0; r < state->current_map->regions.len; r++) {
        Region * region = &state->current_map->regions.items[r];
        if (region->player_id == 0)
            continue;

        PlayerData * player = &state->players.items[region->player_id];
        ListBuilding * buildings = &region->buildings;

        for (usize b = 0; b < buildings->len; b++) {
            Building * building = &buildings->items[b];
            if (building->type == BUILDING_EMPTY)
                continue;

            building->spawn_timer += 1;
            usize cost_to_spawn   = 1 + building->upgrades;
            bool is_time_to_spawn = building->spawn_timer >= FPS * BUILDING_SPAWN_INTERVAL;
            bool can_afford       = player->resource_gold >= cost_to_spawn;

            if (is_time_to_spawn && can_afford) {
                player->resource_gold -= cost_to_spawn;
                building->spawn_timer  = 0;
                spawn_unit(state, building);
            }
        }
    }
}

usize find_unit (ListUnit * units, Unit * unit) {
    for (usize a = 0; a < units->len; a++) {
        if (units->items[a] == unit) {
            return a;
        }
    }
    return units->len;
}

void update_unit_state (GameState * state) {
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

void move_units (GameState * state, float delta_time) {
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

void units_fight (GameState * state, float delta_time) {
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

void guardian_fight (GameState * state, float delta_time) {
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

void simulate_units (GameState * state) {
    float dt = GetFrameTime();

    spawn_units       (state);
    move_units        (state, dt);
    units_fight       (state, dt);
    guardian_fight    (state, dt);
    update_unit_state (state);
}

void update_resources (GameState * state) {
    if (state->turn % (FPS * BUILDING_RESOURCE_INTERVAL))
        return;

    for (usize i = 0; i < state->current_map->regions.len; i++) {
        Region * region = &state->current_map->regions.items[i];
        if (region->player_id == 0)
            continue;
        usize gold = 6;

        for (usize b = 0; b < region->buildings.len; b++) {
            if (region->buildings.items[b].type == BUILDING_RESOURCE) {
                gold += 3 * (region->buildings.items[b].upgrades + 1);
            }
        }

        state->players.items[region->player_id].resource_gold += gold;
    }
}

GameState create_game_state (Map * map) {
    GameState state   = {0};
    state.current_map = map;
    state.camera      = setup_camera(map);
    state.units       = listUnitInit(120, &MemAlloc, &MemFree);
    state.players     = listPlayerDataInit(map->player_count + 1, &MemAlloc, &MemFree);
    state.players.len = map->player_count + 1;

    clear_memory(state.players.items, sizeof(PlayerData) * state.players.len);

    return state;
}

void destroy_game_state (GameState state) {
    clear_unit_list(&state.units);
    listPlayerDataDeinit(&state.players);
    level_unload(state.current_map);
}
