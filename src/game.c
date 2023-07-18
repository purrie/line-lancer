#include "game.h"
#include "assets.h"
#include "std.h"
#include "ai.h"
#include "units.h"
#include "input.h"
#include "level.h"
#include "bridge.h"
#include "constants.h"
#include <raymath.h>

PlayerData * get_local_player (GameState *const state) {
    for (usize i = 0; i < state->players.len; i++) {
        PlayerData * player = &state->players.items[i];
        if (player->type == PLAYER_LOCAL) {
            return player;
        }
    }
    return NULL;
}

Result get_local_player_index (GameState *const state, usize * result) {
    for (usize i = 0; i < state->players.len; i++) {
        PlayerData * player = &state->players.items[i];
        if (player->type == PLAYER_LOCAL) {
            *result = i;
            return SUCCESS;
        }
    }
    return FAILURE;
}

Color get_player_color (usize player_id) {
    switch (player_id) {
        case 0: return LIGHTGRAY;
        case 1: return RED;
        case 2: return BLUE;
        case 3: return YELLOW;
        case 4: return GREEN;
        case 5: return LIME;
        case 6: return BEIGE;
        case 7: return BLACK;
        case 8: return DARKPURPLE;
        default: return PINK;
    }
}

Test spawn_unit (GameState * state, Building * building) {
    Unit * unit = unit_from_building(building);
    if (unit == NULL) {
        return NO;
    }
    listUnitAppend(&state->units, unit);
    return YES;
}

void spawn_units (GameState * state, float delta_time) {
    for (usize r = 0; r < state->map.regions.len; r++) {
        Region * region = &state->map.regions.items[r];
        if (region->player_id == 0)
            continue;

        PlayerData * player = &state->players.items[region->player_id];
        ListBuilding * buildings = &region->buildings;

        for (usize b = 0; b < buildings->len; b++) {
            Building * building = &buildings->items[b];
            if (building->type == BUILDING_EMPTY)
                continue;

            building->spawn_timer += delta_time;
            float interval = building_trigger_interval(building);
            if (building->spawn_timer < interval)
                continue;
            building->spawn_timer = 0.0f;

            switch (building->type) {
                case BUILDING_EMPTY:
                case BUILDING_TYPE_COUNT:
                    break;
                case BUILDING_RESOURCE: {
                    player->resource_gold += building_generated_income(building);
                } break;
                case BUILDING_FIGHTER:
                case BUILDING_ARCHER:
                case BUILDING_SUPPORT:
                case BUILDING_SPECIAL: {
                    usize cost_to_spawn = building_cost_to_spawn(building);
                    bool can_afford     = player->resource_gold >= cost_to_spawn;
                    if (can_afford && spawn_unit(state, building))
                        player->resource_gold -= cost_to_spawn;
                } break;
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
                        Region * region;
                        if (is_unit_at_path_end(unit) &&
                            is_unit_at_main_path(unit, &state->map.paths) &&
                            (region = map_get_region_at(&state->map, unit->position)) &&
                            region->player_id == unit->player_owned) {

                            PathEntry * entry = region_path_entry_from_bridge(region, unit->location->bridge);
                            if (bridge_is_enemy_present(&entry->castle_path, unit->player_owned)) {
                                TraceLog(LOG_INFO, "Moving to defend the path");
                                if (move_unit_towards(unit, entry->castle_path.start)) {
                                    TraceLog(LOG_ERROR, "Failed to move unit to defend");
                                }
                                continue;
                            }

                            Path * redirected = entry->redirects.items[entry->active_redirect].to;
                            PathEntry * redirect_entry = region_path_entry(region, redirected);
                            if (bridge_is_enemy_present(&redirect_entry->castle_path, unit->player_owned)) {
                                TraceLog(LOG_INFO, "Moving to defend redirect");
                                if (move_unit_towards(unit, entry->defensive_paths.items[entry->active_redirect].start)) {
                                    TraceLog(LOG_ERROR, "Failed to redirect unit to defend");
                                }
                                continue;
                            }
                        }

                        if (can_move_forward(unit) == NO) {
                            unit->move_direction = ! unit->move_direction;
                        }
                    }

                    if (can_move_forward(unit) == NO) {
                        unit->move_direction = ! unit->move_direction;
                    }
                    else if(move_unit_forward(unit) == FATAL) {
                        unit->move_direction = ! unit->move_direction;
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
                Region * region = region_by_guardian(&state->map.regions, target);
                region_change_ownership(state, region, unit->player_owned);
            }
            else {
                usize target_index = destroy_unit(&state->units, target);
                if (target_index < i) i--;
            }
        }
    }
}

void guardian_fight (GameState * state, float delta_time) {
    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];

        for (usize p = 0; p < region->paths.len; p++) {
            PathEntry * entry = &region->paths.items[p];
            Node * node = entry->castle_path.end;

            while (node != entry->castle_path.start) {
                if (node->unit && node->unit->player_owned != region->player_id) {
                    node->unit->health -= get_unit_attack(&region->castle.guardian) * delta_time;
                    if (node->unit->health <= 0.0f) {
                        destroy_unit(&state->units, node->unit);
                    }
                    // damage only the first on the path
                    goto once;
                }
                node = node->previous;
            }
        }
        once: {}
    }
}

void simulate_units (GameState * state) {
    float dt = GetFrameTime();

    spawn_units       (state, dt);
    move_units        (state, dt);
    units_fight       (state, dt);
    guardian_fight    (state, dt);
    update_unit_state (state);
}

void update_resources (GameState * state) {
    if (state->turn % (FPS * REGION_INCOME_INTERVAL))
        return;

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        if (region->player_id == 0)
            continue;

        state->players.items[region->player_id].resource_gold += REGION_INCOME;
    }
}

Camera2D setup_camera(Map * map) {
    Camera2D cam = {0};
    Vector2 map_size;
    map_size.x = (GetScreenWidth()  - 20.0f) / (float)map->width;
    map_size.y = (GetScreenHeight() - 20.0f) / (float)map->height;
    cam.zoom   = (map_size.x < map_size.y) ? map_size.x : map_size.y;

    cam.offset = (Vector2) { GetScreenWidth() / 2 , GetScreenHeight() / 2 };
    cam.target = (Vector2) { map->width / 2       , map->height / 2 };

    return cam;
}

Result game_state_prepare (GameState * result, Map *const prefab) {
    // TODO use map name
    TraceLog(LOG_INFO, "Cloning map for gameplay");
    if (map_clone(&result->map, prefab)) {
        TraceLog(LOG_ERROR, "Failed to set up map %s, for gameplay", prefab->name);
        return FAILURE;
    }
    TraceLog(LOG_INFO, "Preparing new map for gameplay");
    if (map_prepare_to_play(&result->map)) {
        TraceLog(LOG_ERROR, "Failed to finalize setup for map %s", prefab->name);
        map_deinit(&result->map);
        return FAILURE;
    }
    TraceLog(LOG_INFO, "Map ready to play");

    result->camera      = setup_camera(&result->map);
    result->units       = listUnitInit(120, &MemAlloc, &MemFree);

    result->players     = listPlayerDataInit(result->map.player_count + 1, &MemAlloc, &MemFree);
    result->players.len = result->map.player_count + 1;
    clear_memory(result->players.items, sizeof(PlayerData) * result->players.len);

    // TODO make better way to set which is the local player, especially after implementing multiplayer
    result->players.items[1].type = PLAYER_LOCAL;
    for (usize i = 1; i < result->players.len; i++) {
        result->players.items[i].type = PLAYER_AI;
    }

    for (usize i = 1; i < result->players.len; i++) {
        result->players.items[i].resource_gold = 20;
    }

    for (usize r = 0; r < result->map.regions.len; r++) {
        Region * region = &result->map.regions.items[r];
        region->faction = result->players.items[region->player_id].faction;
    }

    return SUCCESS;
}

void game_state_deinit (GameState * state) {
    clear_unit_list(&state->units);
    listPlayerDataDeinit(&state->players);
    map_deinit(&state->map);
    clear_memory(state, sizeof(GameState));
}

void game_tick (GameState * state) {
    state->turn ++;
    update_input_state(state);
    update_resources(state);
    simulate_ai(state);
    simulate_units(state);
}
