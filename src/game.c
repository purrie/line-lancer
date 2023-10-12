#include "game.h"
#include "assets.h"
#include "std.h"
#include "ai.h"
#include "units.h"
#include "input.h"
#include "level.h"
#include "constants.h"
#include "particle.h"
#include "alloc.h"
#include <raymath.h>

/* Information ***************************************************************/
PlayerData * get_local_player (const GameState * state) {
    for (usize i = 0; i < state->players.len; i++) {
        PlayerData * player = &state->players.items[i];
        if (player->type == PLAYER_LOCAL) {
            return player;
        }
    }
    return NULL;
}
Result get_local_player_index (const GameState * state, usize * result) {
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
Test support_can_support (const Unit * unit, ListUnit * buffer) {
    switch (unit->faction) {
        case FACTION_KNIGHTS: {
            if (get_allies_in_range(unit, buffer)) {
                return NO;
            }
            while (buffer->len --> 0) {
                Unit * ally = buffer->items[buffer->len];
                float damage = get_unit_wounds(ally);
                if (damage > 1.0f && unit_has_effect(ally, MAGIC_HEALING, NULL) == NO) {
                    return YES;
                }
            }
        } break;
        case FACTION_MAGES: {
            if (get_enemies_in_range(unit, buffer)) {
                return NO;
            }
            while (buffer->len --> 0) {
                Unit * enemy = buffer->items[buffer->len];
                if (unit_has_effect(enemy, MAGIC_WEAKNESS, NULL) == NO) {
                    return YES;
                }
            }
        } break;
    }
    return NO;
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
                    if (can_afford && spawn_unit(state, building)) {
                        TraceLog(LOG_DEBUG, "Spawned unit for player %zu for %zu", region->player_id, cost_to_spawn);
                        player->resource_gold -= cost_to_spawn;
                    }
                } break;
            }
        }
    }
}
void update_unit_state (GameState * state) {
    ListUnit buffer = listUnitInit(12, temp_allocator());
    if (buffer.items == NULL) {
        TraceLog(LOG_WARNING, "Failed to allocate temporary memory, falling back to slow memory");
        buffer = listUnitInit(12, perm_allocator());
    }

    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        if (unit->cooldown > 0)
            continue;

        switch (unit->state) {
            case UNIT_STATE_IDLE: {
                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_range(unit)) {
                    unit->state = UNIT_STATE_FIGHTING;
                    continue;
                }
                if (unit->current_path < unit->pathfind.len) {
                    if (get_enemy_in_sight(unit)) {
                        unit->state = UNIT_STATE_CHASING;
                    }
                    else {
                        unit->state = UNIT_STATE_MOVING;
                    }
                }
            } break;
            case UNIT_STATE_MOVING: {
                if (unit_reached_waypoint(unit) == NO) continue;

                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_sight(unit)) {
                    unit->state = UNIT_STATE_IDLE;
                }

                if (!unit_has_path(unit)) {
                    unit->state = UNIT_STATE_IDLE;
                }
            } break;
            case UNIT_STATE_CHASING: {
                if (unit_reached_waypoint(unit) == NO) continue;

                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_range(unit)) {
                    unit->state = UNIT_STATE_FIGHTING;
                }

                if (!unit_has_path(unit)) {
                    unit->state = UNIT_STATE_IDLE;
                }

            } break;
            case UNIT_STATE_SUPPORTING: {
                if (support_can_support(unit, &buffer) == NO) {
                    unit->state = UNIT_STATE_IDLE;
                }
            } break;
            // do nothing, guardians are always in this state, regular units never enter this state
            case UNIT_STATE_GUARDING: continue;
            case UNIT_STATE_FIGHTING: {
                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_range(unit) == NULL) {
                    unit->state = UNIT_STATE_IDLE;
                }
            } break;
        }
    }
    listUnitDeinit(&buffer);
}
void move_units (GameState * state, float delta_time) {
    for (usize u = 0; u < state->units.len; u++) {
        Unit * unit = state->units.items[u];

        switch (unit->state) {
            case UNIT_STATE_IDLE: {
                if (unit->cooldown > 0) {
                    continue;
                }
                unit->cooldown = FPS / 10;
                unit->current_path = 0;

                NavRangeSearchContext context = {
                    .type = NAV_CONTEXT_HOSTILE,
                    .amount = NAV_CONTEXT_SINGLE,
                    .player_id = unit->player_owned,
                    .range = UNIT_MAX_RANGE
                };
                if (nav_range_search(unit->waypoint, &context) == SUCCESS) {
                    NavTarget target = {
                        .approach_only = true,
                        .waypoint = context.unit_found->waypoint,
                        .type = NAV_TARGET_WAYPOINT
                    };
                    if (nav_find_path(unit->waypoint, target, &unit->pathfind) == SUCCESS) {
                        continue;
                    }
                }

                if (unit->waypoint->graph->type == GRAPH_PATH) {
                    Path * path = unit->waypoint->graph->path;
                    Region * region = path->region_a->player_id == unit->player_owned ? path->region_b : path->region_a;
                    NavTarget navtarget = (NavTarget){
                        .region = region,
                        .approach_only = false,
                        .type = NAV_TARGET_REGION
                    };
                    if (nav_find_path(unit->waypoint, navtarget, &unit->pathfind)) {
                        TraceLog(LOG_ERROR, "Failed to find path to neighboring region");
                    }
                    continue;
                }

                Region * region = unit->waypoint->graph->region;
                if (region->player_id == unit->player_owned) {
                    // exit path of a region has been specified  v
                    if (region->active_path < region->paths.len) {
                        Path * path = region->paths.items[region->active_path];
                        Region * target = path->region_a == region ? path->region_b : path->region_a;
                        NavTarget navtarget = (NavTarget){
                            .region = target,
                            .approach_only = false,
                            .type = NAV_TARGET_REGION
                        };
                        if (nav_find_path(unit->waypoint, navtarget, &unit->pathfind)) {
                            TraceLog(LOG_ERROR, "Failed to find path to neighboring region");
                        }
                        continue;
                    }
                go_idle: {}
                    usize allowed_attempts = 10;
                    while (allowed_attempts --> 0) {
                        usize random = GetRandomValue(0, region->nav_graph.waypoints.len - 1);
                        WayPoint * target = region->nav_graph.waypoints.items[random];
                        if (target == NULL) continue;
                        if (target->blocked || target->unit) continue;
                        NavTarget navtarget = {
                            .approach_only = true,
                            .waypoint = target,
                            .type = NAV_TARGET_WAYPOINT
                        };
                        if (nav_find_path(unit->waypoint, navtarget, &unit->pathfind)) {
                            TraceLog(LOG_ERROR, "Failed to find idling path inside region");
                            unit->cooldown = FPS;
                        }
                        break;
                    }
                }
                else {
                    NavTarget target = {
                        .approach_only = true,
                        .waypoint = region->castle.waypoint,
                        .type = NAV_TARGET_WAYPOINT
                    };
                    if (nav_find_path(unit->waypoint, target, &unit->pathfind)) {
                        goto go_idle;
                    }
                }

            } break;
            case UNIT_STATE_CHASING:
            case UNIT_STATE_MOVING: {
                if (unit_reached_waypoint(unit)) {
                    if (unit_progress_path(unit)) {
                        TraceLog(LOG_WARNING, "Failed to progress unit path");
                        unit->pathfind.len = 0;
                        continue;
                    }
                }
                unit->position = Vector2MoveTowards(
                    unit->position,
                    unit->waypoint->world_position,
                    UNIT_SPEED * delta_time
                );
            } break;
            default: break;
        }
    }
}
void units_support (GameState * state, float delta_time) {
    (void)delta_time;
    ListUnit buffer = listUnitInit(12, temp_allocator());
    if (buffer.items == NULL) {
        TraceLog(LOG_WARNING, "Failed to allocate temporary memory, falling back to slow memory");
        buffer = listUnitInit(12, perm_allocator());
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
                    if (unit_has_effect(check, MAGIC_HEALING, NULL))
                        continue;
                    float check_damage = get_unit_wounds(check);
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
                    if (unit_has_effect(unit, MAGIC_WEAKNESS, NULL))
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
    (void)delta_time;
    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];
        if (unit->state != UNIT_STATE_FIGHTING)
            continue;
        if (unit->cooldown > 0)
            continue;

        Unit * target = get_enemy_in_range(unit);
        if (target) {
            Attack attack = {
                .damage = get_unit_attack(unit),
                .attacker_player_id = unit->player_owned,
                .attacker_faction = unit->faction,
                .attacker_type = unit->type,
                .origin_position = unit->position,
                .delay = get_unit_attack_delay(unit),
                .timer = 0.0f,
            };
            listAttackAppend(&target->incoming_attacks, attack);
            unit->cooldown = get_unit_cooldown(unit);
        }
    }
}
void units_damage(GameState * state, float delta_time) {
    usize units = state->units.len;
    while (units --> 0) {
        Unit * unit = state->units.items[units];
        usize attacks = unit->incoming_attacks.len;
        while (attacks --> 0) {
            Attack * attack = &unit->incoming_attacks.items[attacks];
            attack->timer += delta_time;
            if (attack->timer < attack->delay)
                continue;

            if (attack->attacker_player_id != unit->player_owned) {
                unit->health -= attack->damage;
                particles_blood(state, unit, *attack);
                if (unit->health <= 0.0f) {
                    unit_kill(state, unit);
                    goto next_unit;
                }
            }

            listAttackRemove(&unit->incoming_attacks, attacks);
        }
        next_unit:{}
    }

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        Unit * guardian = &region->castle;
        usize attacks = guardian->incoming_attacks.len;
        while (attacks --> 0) {
            Attack * attack = &guardian->incoming_attacks.items[attacks];
            attack->timer += delta_time;
            if (attack->timer < attack->delay)
                continue;


            if (attack->attacker_player_id != guardian->player_owned) {
                guardian->health -= attack->damage;
                particles_blood(state, guardian, *attack);
                if (guardian->health <= 0.0f) {
                    region_change_ownership(state, region, attack->attacker_player_id);
                    goto next_guard;
                }
            }

            listAttackRemove(&guardian->incoming_attacks, attacks);
        }
        next_guard:{}
    }
}
void guardian_fight (GameState * state, float delta_time) {
    (void)delta_time;
    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];

        Unit * guardian = &region->castle;
        if (guardian->cooldown > 0)
            continue;

        Unit * target = get_enemy_in_range(guardian);
        if (target) {
            Attack attack = {
                .damage = get_unit_attack(guardian),
                .attacker_player_id = guardian->player_owned,
                .attacker_faction = guardian->faction,
                .attacker_type = UNIT_GUARDIAN,
                .origin_position = guardian->position,
                .delay = get_unit_attack_delay(guardian),
                .timer = 0.0f,
            };
            listAttackAppend(&target->incoming_attacks, attack);
            guardian->cooldown = get_unit_cooldown(guardian);
        }
    }
}
void process_effects (GameState * state, float delta_time) {
    usize unit_count = state->units.len;
    while (unit_count --> 0) {
        Unit * unit = state->units.items[unit_count];
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
                        unit_kill(state, unit);
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
        Unit * guardian = &region->castle;
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
    for (usize i = 0; i < state->map.regions.len; i++) {
        Unit * guard = &state->map.regions.items[i].castle;
        if (guard->cooldown > 0)
            guard->cooldown --;
    }

    spawn_units       (state, dt);
    update_unit_state (state);
    move_units        (state, dt);
    units_support     (state, dt);
    units_fight       (state, dt);
    guardian_fight    (state, dt);
    units_damage      (state, dt);
    process_effects   (state, dt);
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
Result game_state_prepare (GameState * result, const Map * prefab) {
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

    result->camera = setup_camera(&result->map);
    result->units  = listUnitInit(120, perm_allocator());

    result->particles_available = listParticleInit(PARTICLES_MAX, perm_allocator());
    result->particles_in_use    = listParticleInit(PARTICLES_MAX, perm_allocator());
    for (usize i = 0; i < PARTICLES_MAX; i++) {
        listParticleAppend(&result->particles_available, (Particle*)&result->resources->particle_pool[i]);
    }

    result->players     = listPlayerDataInit(result->map.player_count + 1, perm_allocator());
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
        unsigned int max = -1;
        max /= 2;
        max -= 1;
        result->players.items[i].seed = GetRandomValue(0, (int)max);
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
    listParticleDeinit(&state->particles_available);
    listParticleDeinit(&state->particles_in_use);
    map_deinit(&state->map);
    clear_memory(state, sizeof(GameState));
}
void game_tick (GameState * state) {
    state->turn ++;
    update_input_state(state);
    update_resources(state);
    simulate_ai(state);
    simulate_units(state);

    particles_advance(state->particles_in_use.items, state->particles_in_use.len, GetFrameTime());
    particles_clean(state);
}
