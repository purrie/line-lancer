#include "units.h"
#include "std.h"
#include "game.h"
#include "constants.h"
#include "particle.h"
#include "animation.h"
#include "unit_pool.h"
#include <raymath.h>


/* Info **********************************************************************/
Test unit_reached_waypoint (const Unit * unit) {
    const float min = 0.1f * 0.1f;
    if (Vector2DistanceSqr(unit->position, unit->waypoint->world_position) < min) {
        return YES;
    }
    return NO;
}
Test unit_has_path (const Unit * unit) {
    if (unit->pathfind.len == 0) {
        return NO;
    }
    usize next_point = unit->current_path + 1;
    if (unit->pathfind.len <= next_point) {
        return NO;
    }
    WayPoint * next = unit->pathfind.items[next_point];
    if (next->blocked || next->unit) {
        return NO;
    }
    return YES;
}
Test is_unit_at_own_region (const Unit * unit) {
    if (unit->waypoint->graph->type != GRAPH_REGION)
        return NO;

    Region * reg = unit->waypoint->graph->region;
    if (reg && reg->player_id == unit->player_owned) {
        return YES;
    }
    return NO;
}
Test is_unit_tied_to_building (const Unit * unit) {
    if (unit->origin &&
        unit->origin->region->player_id == unit->player_owned &&
        unit->origin->units_spawned > 0)
        return YES;
    return NO;
}
Test unit_has_effect (const Unit * unit, MagicType type, MagicEffect * found) {
    for (usize e = 0; e < unit->effects.len; e++) {
        if (unit->effects.items[e].type == type) {
            if (found != NULL) {
                *found = unit->effects.items[e];
            }
            return YES;
        }
    }
    return NO;
}
Test unit_should_repath (const Unit * unit) {
    if (unit->current_path >= unit->pathfind.len) return YES;
    if (unit->pathfind.len == 0) return YES;

    bool is_in_region = unit->waypoint->graph->type == GRAPH_REGION;
    if (is_in_region == false) return NO;

    WayPoint * destination = unit->pathfind.items[unit->pathfind.len - 1];
    Region * target = destination->graph->region;

    Region * unit_region = unit->waypoint->graph->region;
    if (unit_region->active_path >= unit_region->paths.len) {
        if (target != unit_region) return YES;
        return NO;
    }
    Path * path = unit_region->paths.items[unit_region->active_path];

    Region * next_region = path->region_a == unit_region ? path->region_b : path->region_a;
    if (target != next_region) {
        return YES;
    }
    return NO;
}

/* Balance *******************************************************************/
// @balance
// @volitile=faction
// @volitile=unit
usize unit_range[FACTION_COUNT][UNIT_TYPE_ALL_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = {
        [UNIT_FIGHTER]  = { 1, 1, 1 },
        [UNIT_ARCHER]   = { 3, 4, 5 },
        [UNIT_SUPPORT]  = { 4, 5, 6 },
        [UNIT_SPECIAL]  = { 2, 2, 2 },
        [UNIT_GUARDIAN] = { 8, 8, 8 },
    },
    [FACTION_MAGES] = {
        [UNIT_FIGHTER]  = { 1, 1, 1 },
        [UNIT_ARCHER]   = { 3, 4, 5 },
        [UNIT_SUPPORT]  = { 4, 5, 6 },
        [UNIT_SPECIAL]  = { 2, 2, 2 },
        [UNIT_GUARDIAN] = { 8, 8, 8 },
    },
};
float unit_damage[FACTION_COUNT][UNIT_TYPE_ALL_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = {
        [UNIT_FIGHTER]  = { 25, 28, 30 },
        [UNIT_ARCHER]   = { 25, 25, 35 },
        [UNIT_SUPPORT]  = { 20, 20, 20 },
        [UNIT_SPECIAL]  = { 25, 28, 30 },
        [UNIT_GUARDIAN] = { 15 },
    },
    [FACTION_MAGES] = {
        [UNIT_FIGHTER]  = { 50, 60, 70 },
        [UNIT_ARCHER]   = { 35, 40, 45 },
        [UNIT_SUPPORT]  = { 10, 20, 30 },
        [UNIT_SPECIAL]  = { 10, 20, 30 },
        [UNIT_GUARDIAN] = { 20, 20, 20 },
    },
};
float unit_health_max[FACTION_COUNT][UNIT_TYPE_ALL_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = {
        [UNIT_FIGHTER]  = { 125, 135, 145 },
        [UNIT_ARCHER]   = { 110, 115, 120 },
        [UNIT_SUPPORT]  = { 110, 115, 120 },
        [UNIT_SPECIAL]  = { 125, 135, 145 },
        [UNIT_GUARDIAN] = { 4000 },
    },
    [FACTION_MAGES] = {
        [UNIT_FIGHTER]  = { 105, 105, 105 },
        [UNIT_ARCHER]   = { 100, 100, 100 },
        [UNIT_SUPPORT]  = { 100, 100, 100 },
        [UNIT_SPECIAL]  = { 105, 105, 105 },
        [UNIT_GUARDIAN] = { 4000, 4000, 4000 },
    },
};
float unit_attack_cooldown[FACTION_COUNT][UNIT_TYPE_ALL_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = {
        [UNIT_FIGHTER]  = { 0.4f, 0.38f, 0.36f },
        [UNIT_ARCHER]   = { 0.7f, 0.7f, 1.2f },
        [UNIT_SUPPORT]  = { 1.0f, 1.0f, 1.0f },
        [UNIT_SPECIAL]  = { 0.4f, 0.38f, 0.36f },
        [UNIT_GUARDIAN] = { 1.0f },
    },
    [FACTION_MAGES] = {
        [UNIT_FIGHTER]  = { 0.5f, 0.5f, 0.5f },
        [UNIT_ARCHER]   = { 1.7f, 1.7f, 1.7f },
        [UNIT_SUPPORT]  = { 1.9f, 1.9f, 1.9f },
        [UNIT_SPECIAL]  = { 0.2f, 0.15f, 0.1f },
        [UNIT_GUARDIAN] = { 1.1f, 1.1f, 1.1f },
    },
};
float unit_projectile_hit_delay[FACTION_COUNT][UNIT_TYPE_ALL_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = {
        [UNIT_FIGHTER]  = { 0.2f, 0.2f, 0.2f },
        [UNIT_ARCHER]   = { 1.0f, 1.0f, 0.6f },
        [UNIT_SUPPORT]  = { 1.0f, 1.0f, 1.0f },
        [UNIT_SPECIAL]  = { 0.2f, 0.2f, 0.2f },
        [UNIT_GUARDIAN] = { 1.0f },
    },
    [FACTION_MAGES] = {
        [UNIT_FIGHTER]  = { 0.4f, 0.4f, 0.4f },
        [UNIT_ARCHER]   = { 1.7f, 1.7f, 1.7f },
        [UNIT_SUPPORT]  = { 0.4f, 0.4f, 0.4f },
        [UNIT_SPECIAL]  = { 0.19f, 0.14f, 0.09f },
        [UNIT_GUARDIAN] = { 1.0f, 1.0f, 1.0f },
    },
};
float unit_move_speed[FACTION_COUNT][UNIT_TYPE_ALL_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = {
        [UNIT_FIGHTER]  = { 20.0f, 20.0f, 20.0f },
        [UNIT_ARCHER]   = { 20.0f, 20.0f, 20.0f },
        [UNIT_SUPPORT]  = { 20.0f, 20.0f, 20.0f },
        [UNIT_SPECIAL]  = { 30.0f, 35.0f, 40.0f },
        [UNIT_GUARDIAN] = { 0.0f },
    },
    [FACTION_MAGES] = {
        [UNIT_FIGHTER]  = { 15.0f, 20.0f, 25.0f },
        [UNIT_ARCHER]   = { 15.0f, 18.0f, 22.0f },
        [UNIT_SUPPORT]  = { 25.0f, 30.0f, 35.0f },
        [UNIT_SPECIAL]  = { 25.0f, 30.0f, 35.0f },
        [UNIT_GUARDIAN] = { 0.0f, 0.0f, 0.0f },
    },
};
float unit_support_power_strength[FACTION_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = { 0.05f, 0.1f, 0.15f },
    [FACTION_MAGES]   = { 0.15f, 0.3f, 0.5f },
};
float unit_support_power_duration[FACTION_COUNT][UNIT_LEVELS] = {
    [FACTION_KNIGHTS] = { 5.0f, 10.0f, 15.0f },
    [FACTION_MAGES]   = { 5.0f, 10.0f, 15.0f },
};
float unit_support_power_type[FACTION_COUNT] = {
    [FACTION_KNIGHTS] = MAGIC_HEALING,
    [FACTION_MAGES] = MAGIC_WEAKNESS,
};

/* Combat ********************************************************************/
usize get_unit_range (const Unit * unit) {
    return unit_range[unit->faction][unit->type][unit->upgrade];
}
float get_unit_attack_damage (const Unit * unit) {
    float attack = unit_damage[unit->faction][unit->type][unit->upgrade];
    float bonus = 0.0f;
    for (usize i = 0; i < unit->effects.len; i++) {
        switch (unit->effects.items[i].type) {
            // this assumes only one effect of a given type can be on a unit at the same time
            case MAGIC_WEAKNESS: {
                bonus -= attack * unit->effects.items[i].strength;
            } break;
            default: {}
        }
    }
    return attack + bonus;
}
Result get_unit_support_power (const Unit * unit, MagicEffect * effect) {
    if (unit->type != UNIT_SUPPORT)
        return FAILURE;

    *effect = (MagicEffect) {
        .type = unit_support_power_type[unit->faction],
        .strength = unit_support_power_strength[unit->faction][unit->upgrade],
        .duration = unit_support_power_duration[unit->faction][unit->upgrade],
        .source_player = unit->player_owned,
    };

    return SUCCESS;
}
float get_unit_health (UnitType type, FactionType faction, unsigned int upgrades) {
    return unit_health_max[faction][type][upgrades];
}
float get_unit_wounds (const Unit * unit) {
    return get_unit_health(unit->type, unit->faction, unit->upgrade) - unit->health;
}
float get_unit_cooldown (const Unit * unit) {
    return unit_attack_cooldown[unit->faction][unit->type][unit->upgrade];
}
float get_unit_attack_delay  (const Unit * unit) {
    return unit_projectile_hit_delay[unit->faction][unit->type][unit->upgrade];
}
float get_unit_speed (const Unit * unit) {
    return unit_move_speed[unit->faction][unit->type][unit->upgrade];
}
Unit * get_enemy_in_range (const Unit * unit) {
    WayPoint * node = unit->waypoint;
    NavRangeSearchContext context = {
        .type = NAV_CONTEXT_HOSTILE,
        .amount = NAV_CONTEXT_SINGLE,
        .player_id = unit->player_owned,
        .range = get_unit_range(unit),
    };
    if (nav_range_search(node, &context)) {
        return NULL;
    }
    return context.unit_found;
}
Unit * get_enemy_in_sight (const Unit * unit) {
    WayPoint * node = unit->waypoint;
    NavRangeSearchContext context = {
        .type = NAV_CONTEXT_HOSTILE,
        .amount = NAV_CONTEXT_SINGLE,
        .player_id = unit->player_owned,
        .range = UNIT_MAX_RANGE,
    };
    if (nav_range_search(node, &context)) {
        return NULL;
    }
    return context.unit_found;

}
Result get_enemies_in_range (const Unit * unit, ListUnit * result) {
    result->len = 0;
    WayPoint * node = unit->waypoint;
    NavRangeSearchContext context = {
        .type = NAV_CONTEXT_HOSTILE,
        .amount = NAV_CONTEXT_LIST,
        .player_id = unit->player_owned,
        .range = get_unit_range(unit),
        .unit_list = result,
    };
    if (nav_range_search(node, &context)) {
        return FAILURE;
    }
    return SUCCESS;
}
Result get_allies_in_range (const Unit * unit, ListUnit * result) {
    result->len = 0;
    WayPoint * node = unit->waypoint;
    NavRangeSearchContext context = {
        .type = NAV_CONTEXT_FRIENDLY,
        .amount = NAV_CONTEXT_LIST,
        .player_id = unit->player_owned,
        .range = get_unit_range(unit),
        .unit_list = result,
    };
    if (nav_range_search(node, &context)) {
        return FAILURE;
    }
    return SUCCESS;
}
void unit_cursify (Unit * unit, usize player_source, const PlayerData * curser) {
    unit->effects.len = 0;
    unit->incoming_attacks.len = 0;
    unit->faction = curser->faction;
    unit->type = UNIT_SPECIAL;
    unit->health = get_unit_health(UNIT_SPECIAL, curser->faction, 0);
    unit->player_owned = player_source;
    unit->upgrade = 0;
    if (is_unit_tied_to_building(unit)) {
        unit->origin->units_spawned -= 1;
        unit->origin = NULL;
    }
}
void unit_kill (GameState * state, Unit * unit) {
    ListUnit * list = &state->units;

    for (usize i = 0; i < list->len; i++) {
        if (list->items[i] == unit) {
            listUnitRemove(list, i);
            unit_deinit(unit);
            return;
        }
    }
    TraceLog(LOG_ERROR, "Can't destroy the unit, it's not in the list of units");
    return ;
}

/* Movement ******************************************************************/
Result unit_progress_path (Unit * unit) {
    unit->current_path += 1;
    if (unit->current_path >= unit->pathfind.len) {
        return FAILURE;
    }
    WayPoint * next = unit->pathfind.items[unit->current_path];
    if (next->blocked || next->unit) {
        return FAILURE;
    }
    unit->waypoint->unit = NULL;
    unit->waypoint = next;
    unit->waypoint->unit = unit;
    unit->facing_direction = Vector2Normalize(Vector2Subtract(unit->waypoint->world_position, unit->position));
    return SUCCESS;
}
Result unit_calculate_path (Unit * unit) {
    Region * region = unit->waypoint->graph->region;
    if (region->player_id == unit->player_owned) {
        // follow active path or approach own castle
    }
    else {
        // go towards castle of selected region path
    }
    return SUCCESS;
}

/* Setup *********************************************************************/
Unit * unit_init() {
    Unit * unit = unit_alloc();
    if (unit == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for unit");
        return NULL;
    }

    return unit;
}
void unit_deinit(Unit * unit) {
    if (is_unit_tied_to_building(unit))
        unit->origin->units_spawned -= 1;
    unit->waypoint->unit = NULL;
    unit_release(unit);
}
Unit * unit_from_building (const Building * building) {
    if (building->units_spawned >= building_max_units(building))
        return NULL;

    WayPoint * spawn = NULL;

    for (usize i = 0; i < building->spawn_points.len; i++) {
        WayPoint * point = building->spawn_points.items[i];
        if (point->blocked || point->unit)
            continue;
        spawn = point;
        break;
    }

    if (spawn == NULL) {
        TraceLog(LOG_DEBUG, "Building has no free spawn points");
        return NULL;
    }

    UnitType unit_type;
    switch (building->type) {
        case BUILDING_FIGHTER: {
            unit_type = UNIT_FIGHTER;
        } break;
        case BUILDING_ARCHER: {
            unit_type = UNIT_ARCHER;
        } break;
        case BUILDING_SUPPORT: {
            unit_type = UNIT_SUPPORT;
        } break;
        case BUILDING_SPECIAL: {
            unit_type = UNIT_SPECIAL;
        } break;
        default: {
            TraceLog(LOG_ERROR, "Tried to spawn unit from a building that doesn't spawn units");
            return NULL;
        } break;
    }

    Unit * result = unit_init();
    if (result == NULL) {
        return NULL;
    }

    result->type = unit_type;
    result->faction = building->region->faction;
    result->upgrade = building->upgrades;
    result->health = get_unit_health(result->type, result->faction, result->upgrade);
    result->position = building->position;
    result->player_owned = building->region->player_id;
    result->state = UNIT_STATE_MOVING;

    result->origin = (Building*) building;
    result->origin->units_spawned += 1;

    result->pathfind.len = 1;
    result->pathfind.items[0] = spawn;
    result->current_path = 0;
    result->waypoint = spawn;
    result->facing_direction = Vector2Normalize(Vector2Subtract(spawn->world_position, result->position));
    spawn->unit = result;

    return result;
}
Result setup_unit_guardian (Region * region) {
    Unit * guardian = &region->castle;
    guardian->health       = get_unit_health(UNIT_GUARDIAN, region->faction, 0);
    guardian->player_owned = region->player_id;
    guardian->faction      = region->faction;
    guardian->state        = UNIT_STATE_GUARDING;
    guardian->type         = UNIT_GUARDIAN;

    if (guardian->effects.items == NULL) {
        guardian->effects = listMagicEffectInit(MAGIC_TYPE_LAST + 1, perm_allocator());
    }
    else {
        guardian->effects.len = 0;
    }
    if (guardian->incoming_attacks.items == NULL) {
        guardian->incoming_attacks = listAttackInit(6, perm_allocator());
    }
    else {
        guardian->incoming_attacks.len = 0;
    }

    return SUCCESS;
}

/* Visuals *******************************************************************/
void render_unit_health (const Unit * unit) {
    float max_health = get_unit_health(unit->type, unit->faction, unit->upgrade);
    float health = unit->health;
    if (health >= max_health)
        return;
    float percent = health / max_health;
    float angle = percent * 360.0f * 0.5f + 180.0f;

    Color color = (Color) {
        .r = percent > 0.5f ? (unsigned char) (255 * (1.0f - percent)) : 255,
        .g = percent < 0.5f ? (unsigned char) (255 * (percent + percent)) : 255,
        .b = 0,
        .a = 128,
    };

    DrawRing(unit->position, NAV_GRID_SIZE - 2.0f, NAV_GRID_SIZE, 360.0f - angle - 90.0f, angle - 90.0f, 16, color);
}
void render_units (const GameState * state) {
    const ListUnit * units = &state->units;
    Vector2 top_left = GetScreenToWorld2D((Vector2) { -NAV_GRID_SIZE, -NAV_GRID_SIZE}, state->camera);
    Vector2 bot_righ = GetScreenToWorld2D((Vector2) { GetScreenWidth() + NAV_GRID_SIZE, GetScreenHeight() + NAV_GRID_SIZE}, state->camera);
    Rectangle screen = {
        .x = top_left.x,
        .y = top_left.y,
        .width = bot_righ.x - top_left.x,
        .height = bot_righ.y - top_left.y
    };

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        if (! CheckCollisionPointRec(region->castle.position, screen)) {
            continue;
        }
        Texture2D sprite;
        if (region->player_id) {
            sprite = state->resources->buildings[region->faction].castle;
        }
        else {
            sprite = state->resources->neutral_castle;
        }
        Rectangle source = (Rectangle) { 0, 0, sprite.width, sprite.height };
        Rectangle destination = (Rectangle){
            region->castle.position.x,
            region->castle.position.y,
            NAV_GRID_SIZE * 2,
            NAV_GRID_SIZE * 2,
        };
        Vector2 origin = (Vector2){ destination.width * 0.5f, destination.height * 0.5f };
        DrawTexturePro(sprite, source, destination, origin, 0.0f, WHITE);

        particles_render_attacks(state, &region->castle);
        particles_render_effects(state, &region->castle);
        render_unit_health(&region->castle);
    }

    BeginShaderMode(state->resources->outline_shader);
    for (usize i = 0; i < units->len; i ++) {
        Unit * unit = units->items[i];

        if (CheckCollisionPointRec(unit->position, screen)) {
            animate_unit(state, unit);
        }
    }
    EndShaderMode();
    for (usize i = 0; i < units->len; i ++) {
        Unit * unit = units->items[i];

        if (CheckCollisionPointRec(unit->position, screen)) {
            particles_render_attacks(state, unit);
            particles_render_effects(state, unit);
            render_unit_health(unit);
        }
    }
}
