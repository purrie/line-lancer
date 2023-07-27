#include "game.h"
#include "assets.h"
#include "std.h"
#include "ai.h"
#include "units.h"
#include "input.h"
#include "level.h"
#include "bridge.h"
#include "constants.h"
#include "alloc.h"
#include <raymath.h>

/* Information ***************************************************************/
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
usize find_unit (ListUnit * units, Unit * unit) {
    for (usize a = 0; a < units->len; a++) {
        if (units->items[a] == unit) {
            return a;
        }
    }
    return units->len;
}

/* Unit Simulation ***********************************************************/
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
void update_unit_state (GameState * state) {
    ListUnit buffer = listUnitInit(12, &temp_alloc, NULL);
    if (buffer.items == NULL) {
        TraceLog(LOG_WARNING, "Failed to allocate temporary memory, falling back to slow memory");
        buffer = listUnitInit(12, &MemAlloc, &MemFree);
    }

    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        if (unit->cooldown > 0)
            continue;

        switch (unit->state) {
            case UNIT_STATE_MOVING: {
                if (can_unit_progress(unit) == false) {
                    continue;
                }
                if (unit->type == UNIT_SUPPORT) {
                    switch (unit->faction) {
                        case FACTION_KNIGHTS: {
                            if (get_allies_in_range(unit, &buffer)) {
                                TraceLog(LOG_ERROR, "Failed to get knight support allies");
                                break;
                            }
                            while (buffer.len --> 0) {
                                Unit * ally = buffer.items[buffer.len];
                                float damage = get_unit_damage(ally);
                                if (damage > 1.0f && unit_has_effect(ally, MAGIC_HEALING) == NO) {
                                    unit->state = UNIT_STATE_SUPPORTING;
                                    goto next;
                                }
                            }
                        } break;
                        case FACTION_MAGES: {
                            if (get_enemies_in_range(unit, &buffer)) {
                                TraceLog(LOG_ERROR, "Failed to get mage support enemies");
                                break;
                            }
                            while (buffer.len --> 0) {
                                Unit * enemy = buffer.items[buffer.len];
                                if (unit_has_effect(enemy, MAGIC_WEAKNESS) == NO) {
                                    unit->state = UNIT_STATE_SUPPORTING;
                                    goto next;
                                }
                            }
                        } break;
                    }
                }
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
                    else if(move_unit_forward(unit) == FATAL) {
                        unit->move_direction = ! unit->move_direction;
                    }
                }
            } break;
            case UNIT_STATE_SUPPORTING: {
                switch (unit->faction) {
                    case FACTION_KNIGHTS: {
                        if (get_allies_in_range(unit, &buffer)) {
                            TraceLog(LOG_ERROR, "Failed to get allies for knight support state change");
                            unit->state = UNIT_STATE_MOVING;
                            continue;
                        }
                        while (buffer.len --> 0) {
                            Unit * endangered = buffer.items[buffer.len];
                            float damage = get_unit_damage(endangered);
                            if (damage > 1.0f && unit_has_effect(endangered, MAGIC_HEALING) == NO) {
                                goto next;
                            }
                        }
                        unit->state = UNIT_STATE_MOVING;
                    } break;
                    case FACTION_MAGES: {
                        if (get_enemies_in_range(unit, &buffer)) {
                            TraceLog(LOG_ERROR, "Failed to get enemies of mage support for state change");
                            unit->state = UNIT_STATE_MOVING;
                            continue;
                        }
                        while (buffer.len --> 0) {
                            Unit * enemy = buffer.items[buffer.len];
                            if (unit_has_effect(enemy, MAGIC_WEAKNESS) == NO)
                                goto next;
                        }
                        unit->state = UNIT_STATE_MOVING;
                    } break;
                }
            } break;
            case UNIT_STATE_GUARDING: {
                // do nothing, guardians are always in this state, regular units never enter this state
            } break;
            case UNIT_STATE_FIGHTING: {
                if (unit->type == UNIT_SUPPORT) {
                    switch (unit->faction) {
                        case FACTION_KNIGHTS: {
                            if (get_allies_in_range(unit, &buffer)) {
                                TraceLog(LOG_ERROR, "Failed to get knight support allies");
                                break;
                            }
                            while (buffer.len --> 0) {
                                Unit * ally = buffer.items[buffer.len];
                                float damage = get_unit_damage(ally);
                                if (damage > 1.0f && unit_has_effect(ally, MAGIC_HEALING) == NO) {
                                    unit->state = UNIT_STATE_SUPPORTING;
                                    goto next;
                                }
                            }
                        } break;
                        case FACTION_MAGES: {
                            if (get_enemies_in_range(unit, &buffer)) {
                                TraceLog(LOG_ERROR, "Failed to get mage support enemies");
                                break;
                            }
                            while (buffer.len --> 0) {
                                Unit * enemy = buffer.items[buffer.len];
                                if (unit_has_effect(enemy, MAGIC_WEAKNESS) == NO) {
                                    unit->state = UNIT_STATE_SUPPORTING;
                                    goto next;
                                }
                            }
                        } break;
                    }
                }
                if (get_enemy_in_range(unit) == NULL) {
                    unit->state = UNIT_STATE_MOVING;
                }
            } break;
        }
        next: {}
    }
    listUnitDeinit(&buffer);
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
void units_support (GameState * state, float delta_time) {
    (void)delta_time;
    ListUnit buffer = listUnitInit(12, &temp_alloc, NULL);
    if (buffer.items == NULL) {
        TraceLog(LOG_WARNING, "Failed to allocate temporary memory, falling back to slow memory");
        buffer = listUnitInit(12, &MemAlloc, &MemFree);
    }

    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];

        if (unit->type != UNIT_SUPPORT || unit->state != UNIT_STATE_SUPPORTING)
            continue;

        if (unit->cooldown > 0)
            continue;

        switch (unit->faction) {
            case FACTION_KNIGHTS: {
                if (get_allies_in_range(unit, &buffer)) {
                    TraceLog(LOG_ERROR, "Failed to get allies for knight support unit");
                    continue;
                }
                if (buffer.len == 0)
                    continue;

                Unit * most_hurt = NULL;
                float damage = -1.0f;
                for (usize u = 0; u < buffer.len; u++) {
                    Unit * check = buffer.items[u];
                    if (unit_has_effect(check, MAGIC_HEALING))
                        continue;
                    float check_damage = get_unit_damage(check);
                    if (check_damage > damage) {
                        most_hurt = check;
                        damage = check_damage;
                    }
                }
                if (most_hurt == NULL)
                    continue;

                MagicEffect magic;
                if (get_unit_support_power(unit, &magic)) {
                    TraceLog(LOG_ERROR, "Failed to get support unit power for knights");
                    continue;
                }
                unit->cooldown = get_unit_cooldown(unit);
                listMagicEffectAppend(&most_hurt->effects, magic);
            } break;
            case FACTION_MAGES: {
                if (get_enemies_in_range(unit, &buffer)) {
                    TraceLog(LOG_ERROR, "Failed to get enemies for mage support");
                    continue;
                }
                if (buffer.len == 0)
                    continue;

                for (usize u = 0; u < buffer.len; u++) {
                    Unit * target = buffer.items[u];
                    if (unit_has_effect(unit, MAGIC_WEAKNESS))
                        continue;
                    MagicEffect magic;
                    if (get_unit_support_power(unit, &magic)) {
                        TraceLog(LOG_ERROR, "Failed to get magic effect from mage support");
                        goto next;
                    }
                    unit->cooldown = get_unit_cooldown(unit);
                    listMagicEffectAppend(&target->effects, magic);
                    break;
                }
            } break;
        }
        next:{}
    }
    listUnitDeinit(&buffer);
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
                usize target_index = unit_kill(state, target);
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
                        unit_kill(state, node->unit);
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
void process_effects (GameState * state, float delta_time) {
    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];
        usize e = unit->effects.len;
        while (e --> 0) {
            MagicEffect * effect = &unit->effects.items[e];
            switch (effect->type) {
                case MAGIC_HEALING: {
                    float max_health = get_unit_health(unit->type, unit->faction, unit->upgrade);
                    float healing = effect->strength * delta_time;
                    float healed = unit->health + healing;
                    unit->health = max_health < healed ? max_health : healed;
                    effect->strength -= healing;
                } break;
                case MAGIC_HELLFIRE: {
                    float damage = effect->strength * delta_time;
                    unit->health -= damage;
                    effect->strength -= damage;
                    if (unit->health <= 0.0f) {
                        usize index = unit_kill(state, unit);
                        if (index < i)
                            --i;
                        goto next;
                    }
                } break;
                default: {
                    effect->strength -= delta_time;
                } break;
            }
            if (effect->strength <= 0.0f) {
                listMagicEffectRemove(&unit->effects, e);
            }
        }
        next: {}
    }

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        Unit * guardian = &region->castle.guardian;
        usize e = guardian->effects.len;
        while (e --> 0) {
            MagicEffect * effect = &guardian->effects.items[e];
            switch (effect->type) {
                case MAGIC_HEALING: {
                    float max_health = get_unit_health(guardian->type, guardian->faction, guardian->upgrade);
                    float healed = guardian->health + effect->strength;
                    guardian->health = max_health < healed ? max_health : healed;
                    effect->strength = -1.0f;
                } break;
                case MAGIC_HELLFIRE: {
                    float damage = effect->strength * delta_time;
                    guardian->health -= damage;
                    effect->strength -= damage;
                    if (guardian->health <= 0.0f) {
                        region_change_ownership(state, region, effect->source_player);
                        goto next_guard;
                    }

                } break;
                default: {
                    effect->strength -= delta_time;
                } break;
            }
            if (effect->strength <= 0.0f) {
                listMagicEffectRemove(&guardian->effects, e);
            }
        }
        next_guard: {}
    }
}
void simulate_units (GameState * state) {
    float dt = GetFrameTime();

    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];
        if (unit->cooldown > 0)
            unit->cooldown --;
    }

    process_effects   (state, dt);
    spawn_units       (state, dt);
    update_unit_state (state);
    move_units        (state, dt);
    units_support     (state, dt);
    units_fight       (state, dt);
    guardian_fight    (state, dt);
}

/* Gameplay Loop *************************************************************/
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
    #ifdef SIMULATE_PLAYER
    usize player_index = 0;
    #else
    usize player_index = 1;
    result->players.items[player_index].type = PLAYER_LOCAL;
    #endif

    for (usize i = player_index + 1; i < result->players.len; i++) {
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
