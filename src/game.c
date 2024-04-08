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
#include "audio.h"
#include "unit_pool.h"
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
        case 0: return (Color) { 0x80, 0x80, 0x80, 255 };
        case 1: return (Color) { 0x9f, 0x40, 0x40, 255 };
        case 2: return (Color) { 0x50, 0x90, 0xcf, 255 };
        case 3: return (Color) { 0xaa, 0xff, 0x66, 255 };
        case 4: return (Color) { 0x5a, 0x32, 0x64, 255 };
        case 5: return (Color) { 0x30, 0x3f, 0x2f, 255 };
        case 6: return (Color) { 0x01, 0x08, 0x10, 255 };
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
                if (ally->type == UNIT_GUARDIAN) continue;
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
                if (enemy->type == UNIT_GUARDIAN) continue;
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
void update_buildings (GameState * state, float delta_time) {
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
                        unit->state_time = 0;
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_range(unit)) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_FIGHTING;
                    continue;
                }
                if (unit->current_path < unit->pathfind.len) {
                    if (get_enemy_in_sight(unit)) {
                        unit->state_time = 0;
                        unit->state = UNIT_STATE_CHASING;
                    }
                    else {
                        unit->state_time = 0;
                        unit->state = UNIT_STATE_MOVING;
                    }
                }
            } break;
            case UNIT_STATE_MOVING: {
                if (unit_reached_waypoint(unit) == NO) continue;

                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state_time = 0;
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_sight(unit)) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_IDLE;
                }

                if (!unit_has_path(unit)) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_IDLE;
                }
            } break;
            case UNIT_STATE_CHASING: {
                if (unit_reached_waypoint(unit) == NO) continue;

                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state_time = 0;
                        unit->state = UNIT_STATE_SUPPORTING;
                        continue;
                    }
                }
                if (get_enemy_in_range(unit)) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_FIGHTING;
                }

                if (!unit_has_path(unit)) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_IDLE;
                }
            } break;
            case UNIT_STATE_SUPPORTING: {
                if (support_can_support(unit, &buffer) == NO) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_IDLE;
                    unit->attacked = false;
                }
            } break;
            // do nothing, guardians are always in this state, regular units never enter this state
            case UNIT_STATE_GUARDING: continue;
            case UNIT_STATE_FIGHTING: {
                if (unit->type == UNIT_SUPPORT) {
                    if (support_can_support(unit, &buffer)) {
                        unit->state_time = 0;
                        unit->state = UNIT_STATE_SUPPORTING;
                        unit->attacked = false;
                        continue;
                    }
                }
                if (get_enemy_in_range(unit) == NULL) {
                    unit->state_time = 0;
                    unit->state = UNIT_STATE_IDLE;
                    unit->attacked = false;
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
                unit->cooldown = 0.1f;
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
                        navtarget.region = path->region_a == region ? path->region_b : path->region_a;
                        if (nav_find_path(unit->waypoint, navtarget, &unit->pathfind)) {
                            TraceLog(LOG_DEBUG, "Failed to find path to neighboring region");
                        }
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
                            goto go_idle;
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
                            TraceLog(LOG_DEBUG, "Failed to find idling path inside region");
                            unit->cooldown = 1.0f;
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
                        unit->pathfind.len = 0;
                        continue;
                    }
                }
                unit->position = Vector2MoveTowards(
                    unit->position,
                    unit->waypoint->world_position,
                    get_unit_speed(unit) * delta_time
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

        const AnimationSet * animations =
            &state->resources->animations.sets[unit->faction][unit->type][unit->upgrade];

        float cast_len = animations->cast_duration;
        if (unit->state_time >= cast_len) {
            unit->state_time = 0;
            unit->attacked = false;
        }
        if (unit->attacked) continue;

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
                    if (check->type == UNIT_GUARDIAN) continue;
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
                unit->attacked = true;
                unit->facing_direction = Vector2Normalize(Vector2Subtract(most_hurt->position, unit->position));
                listMagicEffectAppend(&most_hurt->effects, magic);
                particles_magic(state, unit, most_hurt);
                play_sound_concurent(state, SOUND_MAGIC_HEALING, unit->position);
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
                    if (target->type == UNIT_GUARDIAN) continue;
                    if (unit_has_effect(target, MAGIC_WEAKNESS, NULL))
                        continue;
                    MagicEffect magic;
                    if (get_unit_support_power(unit, &magic)) {
                        TraceLog(LOG_ERROR, "Failed to get magic effect from mage support");
                        goto next;
                    }
                    unit->cooldown = get_unit_cooldown(unit);
                    unit->attacked = true;
                    unit->facing_direction = Vector2Normalize(Vector2Subtract(target->position, unit->position));
                    listMagicEffectAppend(&target->effects, magic);
                    particles_magic(state, unit, target);
                    play_sound_concurent(state, SOUND_MAGIC_WEAKNESS, unit->position);
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

        const AnimationSet * animations =
            &state->resources->animations.sets[unit->faction][unit->type][unit->upgrade];

        float attack_len = animations->attack_duration;
        if (unit->state_time >= attack_len) {
            unit->state_time = 0;
            unit->attacked = false;
        }

        if (unit->attacked) continue;

        float attack_time = unit->state_time;
        uint8_t attack_frame = animations->attack_start;
        while (attack_time > animations->frames.items[attack_frame].duration) {
            attack_time -= animations->frames.items[attack_frame].duration;
            attack_frame ++;
        }
        if (attack_frame < animations->attack_trigger) continue;

        Unit * target = get_enemy_in_range(unit);
        if (target) {
            Attack attack = {
                .damage = get_unit_attack_damage(unit),
                .attacker_player_id = unit->player_owned,
                .attacker_faction = unit->faction,
                .attacker_type = unit->type,
                .origin_position = unit->position,
                .delay = get_unit_attack_delay(unit),
                .timer = 0.0f,
            };
            listAttackAppend(&target->incoming_attacks, attack);
            unit->cooldown = get_unit_cooldown(unit);
            unit->attacked = true;
            unit->facing_direction = Vector2Normalize(Vector2Subtract(target->position, unit->position));
            play_unit_attack_sound(state, unit);
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
                play_unit_hurt_sound(state, unit);
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
        if (attacks == 0) { // regenerate if not under attack
            ListWayPoint nav = region->nav_graph.waypoints;
            // skip regeneration if enemies are in the region
            for (usize p = 0; p < nav.len; p++) {
                if (nav.items[p] && nav.items[p]->unit) {
                    if(nav.items[p]->unit->faction != guardian->faction)
                        goto next_guard;
                }
            }
            float max_health = get_unit_health(UNIT_GUARDIAN, region->faction, 0);
            if (guardian->health < max_health) {
                // @balance
                guardian->health += delta_time * max_health * 0.01;
                if (guardian->health > max_health)
                    guardian->health = max_health;
            }
        }
        else
        while (attacks --> 0) {
            Attack * attack = &guardian->incoming_attacks.items[attacks];
            attack->timer += delta_time;
            if (attack->timer < attack->delay)
                continue;

            if (attack->attacker_player_id != guardian->player_owned) {
                guardian->health -= attack->damage;
                play_unit_hurt_sound(state, guardian);
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
                .damage = get_unit_attack_damage(guardian),
                .attacker_player_id = guardian->player_owned,
                .attacker_faction = guardian->faction,
                .attacker_type = UNIT_GUARDIAN,
                .origin_position = guardian->position,
                .delay = get_unit_attack_delay(guardian),
                .timer = 0.0f,
            };
            listAttackAppend(&target->incoming_attacks, attack);
            guardian->cooldown = get_unit_cooldown(guardian);
            play_unit_attack_sound(state, guardian);
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
            // @balance
            switch (effect->type) {
                case MAGIC_HEALING: {
                    float max_health = get_unit_health(unit->type, unit->faction, unit->upgrade);
                    float healing = max_health * effect->strength * delta_time;
                    float healed = unit->health + healing;
                    unit->health = max_health < healed ? max_health : healed;
                } break;
                default: {
                } break;
            }
            effect->duration -= delta_time;
            if (effect->duration <= 0.0f) {
                listMagicEffectRemove(&unit->effects, e);
            }
        }
    }
}
void simulate_units (GameState * state, float dt) {
    for (usize i = 0; i < state->units.len; i++) {
        Unit * unit = state->units.items[i];
        if (unit->cooldown > 0)
            unit->cooldown -= dt;
        unit->state_time += dt;
    }
    for (usize i = 0; i < state->map.regions.len; i++) {
        Unit * guard = &state->map.regions.items[i].castle;
        if (guard->cooldown > 0)
            guard->cooldown -= dt;
    }

    update_buildings  (state, dt);
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
void setup_camera(GameState * state, const Theme * theme) {
    Map * map = &state->map;
    state->camera = (Camera2D){0};
    state->camera.offset = (Vector2) { GetScreenWidth() / 2 , GetScreenHeight() / 2 };
    usize player;
    if (get_local_player_index(state, &player)) {
        TraceLog(LOG_WARNING, "Couldn't find player to set up camera");
    no_player: {}
        Vector2 map_size;
        map_size.x = (GetScreenWidth()  - 20.0f) / (float)map->width;
        map_size.y = (GetScreenHeight() - 20.0f - theme->info_bar_height) / (float)map->height;
        state->camera.zoom   = (map_size.x < map_size.y) ? map_size.x : map_size.y;
        state->camera.target = (Vector2) { map->width / 2 , map->height / 2 - theme->info_bar_height * 0.5f };
        return;
    }

    Region * region = NULL;
    for (usize i = 0; i < map->regions.len; i++) {
        if (map->regions.items[i].player_id == player) {
            region = &map->regions.items[i];
            break;
        }
    }
    if (region == NULL) {
        TraceLog(LOG_WARNING, "Couldn't find player region to set up camera");
        goto no_player;
    }
    Rectangle bounds = area_bounds(&region->area);
    state->camera.target = (Vector2) { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
    bounds.width = ( GetScreenWidth() - 20.0f ) / ( bounds.width  * 2.0f );
    bounds.height = ( GetScreenHeight() - 20.0f - theme->info_bar_height ) / ( bounds.height * 2.0f );
    state->camera.zoom = (bounds.width < bounds.height) ? bounds.width : bounds.height;
}
Result game_state_prepare (GameState * result, const Map * prefab) {
    TraceLog(LOG_INFO, "Cloning map for gameplay");
    if (map_clone(&result->map, prefab)) {
        TraceLog(LOG_ERROR, "Failed to set up map %s, for gameplay", prefab->name);
        return FAILURE;
    }
    TraceLog(LOG_INFO, "Preparing new map for gameplay");
    for (usize r = 0; r < result->map.regions.len; r++) {
        Region * region = &result->map.regions.items[r];
        region->faction = result->players.items[region->player_id].faction;
    }
    if (map_prepare_to_play(result->resources, &result->map)) {
        TraceLog(LOG_ERROR, "Failed to finalize setup for map %s", prefab->name);
        map_deinit(&result->map);
        return FAILURE;
    }
    TraceLog(LOG_INFO, "Map ready to play");

    result->active_sounds = listSFXInit(40, perm_allocator());
    result->disabled_sounds = listSFXInit(40, perm_allocator());
    result->units  = unit_pool_get_new();

    result->particles_available = listParticleInit(PARTICLES_MAX, perm_allocator());
    result->particles_in_use    = listParticleInit(PARTICLES_MAX, perm_allocator());
    for (usize i = 0; i < PARTICLES_MAX; i++) {
        listParticleAppend(&result->particles_available, (Particle*)&result->resources->particle_pool[i]);
    }

    result->players.len = result->map.player_count + 1;

    setup_camera(result, &result->settings->theme);

    // player 0 is neutral faction
    for (usize i = 1; i < result->players.len; i++) {
        if (result->players.items[i].type == PLAYER_AI) {
            ai_init(i, result);
        }
        result->players.items[i].resource_gold = 25;
    }

    return SUCCESS;
}
void game_state_deinit (GameState * state) {
    unit_pool_reset();
    for (usize p = 0; p < state->players.len; p++) {
        if (state->players.items[p].type == PLAYER_AI) {
            ai_deinit(&state->players.items[p]);
        }
    }
    listPlayerDataDeinit(&state->players);
    listParticleDeinit(&state->particles_available);
    listParticleDeinit(&state->particles_in_use);
    stop_sounds(&state->active_sounds);
    stop_sounds(&state->disabled_sounds);
    listSFXDeinit(&state->active_sounds);
    listSFXDeinit(&state->disabled_sounds);
    map_deinit(&state->map);
    clear_memory(state, sizeof(GameState));
}
void game_tick (GameState * state) {
    float dt = GetFrameTime();
    update_input_state(state);

    #if defined(GAME_SUPER_SPEED)
    int counter = GAME_SUPER_SPEED;
    while (counter --> 0) {
    #endif

    state->turn ++;

    update_resources(state);
    simulate_ai(state);
    simulate_units(state, dt);
    clean_sounds(state);

    particles_advance(state->particles_in_use.items, state->particles_in_use.len, dt);
    particles_clean(state);

    #if defined(GAME_SUPER_SPEED)
    }
    #endif
}
usize game_winner (GameState * game) {
    usize player = 0;
    for (usize i = 0; i < game->map.regions.len; i++) {
        Region * region = &game->map.regions.items[i];
        if (region->player_id != player) {
            if (player == 0) {
                player = region->player_id;
            }
            else if (region->player_id != 0) {
                return 0;
            }
        }
    }
    return player;
}
