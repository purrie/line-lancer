#include "units.h"
#include "std.h"
#include "game.h"
#include "constants.h"
#include "particle.h"
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

/* Combat ********************************************************************/
usize get_unit_range (const Unit * unit) {
    // TODO see if units for different factions need different ranges
    switch(unit->type) {
        case UNIT_FIGHTER:
            return 1;
        case UNIT_ARCHER:
            return 3 + unit->upgrade;
        case UNIT_SUPPORT:
            return 4 + unit->upgrade;
        case UNIT_SPECIAL:
            switch (unit->faction) {
                case FACTION_KNIGHTS: return 2;
                case FACTION_MAGES: return 1;
            }
            break;
        case UNIT_GUARDIAN:
            switch (unit->faction) {
                case FACTION_KNIGHTS: return UNIT_MAX_RANGE + 2;
                case FACTION_MAGES: return UNIT_MAX_RANGE + 2;
            }
            break;
    }
    TraceLog(LOG_ERROR, "Failed to resolve unit to get range");
    return 1;
}
float get_unit_attack (const Unit * unit) {
    float attack;
    switch (unit->type) {
        case UNIT_FIGHTER: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    attack = 60.0f * (unit->upgrade + 1);
                } break;
                case FACTION_MAGES: {
                    attack = 60.0f * (unit->upgrade + 1);
                } break;
            }
        } break;
        case UNIT_ARCHER: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    attack = 50.0f * (unit->upgrade + 1);
                } break;
                case FACTION_MAGES: {
                    attack = 50.0f * (unit->upgrade + 1);
                } break;
            }
        } break;
        case UNIT_SUPPORT: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    attack = 30.0f * (unit->upgrade + 1);
                } break;
                case FACTION_MAGES: {
                    attack = 30.0f * (unit->upgrade + 1);
                } break;
            }
        } break;
        case UNIT_SPECIAL: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    attack = 100.0f * (unit->upgrade + 1);
                } break;
                case FACTION_MAGES: {
                    attack = 100.0f * (unit->upgrade + 1);
                } break;
            }
        } break;
        case UNIT_GUARDIAN: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    attack = 15.0f;
                } break;
                case FACTION_MAGES: {
                    attack = 15.0f;
                } break;
            }
        } break;
        default: {
            TraceLog(LOG_ERROR, "Requested attack for unit with no valid type");
            return 1.0f;
        } break;
    }
    float bonus = 0.0f;
    for (usize i = 0; i < unit->effects.len; i++) {
        switch (unit->effects.items[i].type) {
            case MAGIC_STRENGTH: {
                bonus += attack * 0.5f;
            } break;
            case MAGIC_WEAKNESS: {
                bonus -= attack * 0.5f;
            } break;
            default: {}
        }
    }
    return attack + bonus;
}
Result get_unit_support_power (const Unit * unit, MagicEffect * effect) {
    if (unit->type != UNIT_SUPPORT)
        return FAILURE;
    switch (unit->faction) {
        case FACTION_KNIGHTS: {
            *effect = (MagicEffect){
                .type = MAGIC_HEALING,
                .strength = 5.0f,
                .source_player = unit->player_owned,
            };
            return SUCCESS;
        }
        case FACTION_MAGES: {
            *effect = (MagicEffect){
                .type = MAGIC_WEAKNESS,
                .strength = 10.0f,
                .source_player = unit->player_owned,
            };
            return SUCCESS;
        }
    }
    TraceLog(LOG_ERROR, "Unsupported support unit");
    return FAILURE;
}
float get_unit_health (UnitType type, FactionType faction, unsigned int upgrades) {
    switch (type) {
        case UNIT_FIGHTER:
            switch (faction) {
                case FACTION_KNIGHTS: return 100.0f * (upgrades + 1);
                case FACTION_MAGES:   return 140.0f * (upgrades + 1);
            }
            break;
        case UNIT_ARCHER:
            switch (faction) {
                case FACTION_KNIGHTS: return 80.0f * (upgrades + 1);
                case FACTION_MAGES:   return 70.0f * (upgrades + 1);
            }
            break;
        case UNIT_SUPPORT:
            switch (faction) {
                case FACTION_KNIGHTS: return 60.0f * (upgrades + 1);
                case FACTION_MAGES:   return 60.0f * (upgrades + 1);
            }
            break;
        case UNIT_SPECIAL:
            switch (faction) {
                case FACTION_KNIGHTS: return 120.0f * (upgrades + 1);
                case FACTION_MAGES:   return 50.0f * (upgrades + 1);
            }
            break;
        case UNIT_GUARDIAN:
            switch (faction) {
                case FACTION_KNIGHTS: return 4000.0f;
                case FACTION_MAGES:   return 4000.0f;
            }
            break;
    }
    TraceLog(LOG_ERROR, "Tried to obtain health of unsupported unit type");
    return 1.0f;
}
float get_unit_wounds (const Unit * unit) {
    return get_unit_health(unit->type, unit->faction, unit->upgrade) - unit->health;
}
usize get_unit_cooldown (const Unit * unit) {
    switch (unit->faction) {
        case FACTION_KNIGHTS: {
            switch (unit->type) {
                case UNIT_FIGHTER:  return FPS - unit->upgrade;
                case UNIT_ARCHER:   return FPS - unit->upgrade;
                case UNIT_SUPPORT:  return FPS - unit->upgrade;
                case UNIT_SPECIAL:  return FPS - unit->upgrade;
                case UNIT_GUARDIAN: return FPS;
            }
        } break;
        case FACTION_MAGES: {
            switch (unit->type) {
                case UNIT_FIGHTER:  return FPS - unit->upgrade;
                case UNIT_ARCHER:   return FPS - unit->upgrade;
                case UNIT_SUPPORT:  return FPS - unit->upgrade;
                case UNIT_SPECIAL:  return FPS - unit->upgrade;
                case UNIT_GUARDIAN: return FPS;
            }
        } break;
    }
    TraceLog(LOG_ERROR, "Attempted to get cooldown from unsupported unit");
    return FPS;
}
float get_unit_attack_delay  (const Unit * unit) {
    switch (unit->faction) {
        case FACTION_KNIGHTS: {
            switch (unit->type) {
                case UNIT_FIGHTER:  return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_ARCHER:   return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_SUPPORT:  return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_SPECIAL:  return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_GUARDIAN: return 1.0f;
            }
        } break;
        case FACTION_MAGES: {
            switch (unit->type) {
                case UNIT_FIGHTER:  return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_ARCHER:   return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_SUPPORT:  return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_SPECIAL:  return 1.0f - (unit->upgrade * 0.1f);
                case UNIT_GUARDIAN: return 1.0f;
            }
        } break;
    }
    TraceLog(LOG_ERROR, "Attempted to get cooldown from unsupported unit");
    return 1.0f;
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

    MagicEffect curse;
    if (unit_has_effect(unit, MAGIC_CURSE, &curse)) {
        unit_cursify(unit, curse.source_player, &state->players.items[curse.source_player]);
        return;
    }

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
    Unit * unit = MemAlloc(sizeof(Unit));
    if (unit == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for unit");
        return NULL;
    }
    clear_memory(unit, sizeof(Unit));

    unit->effects = listMagicEffectInit(MAGIC_TYPE_LAST + 1, perm_allocator());
    if (unit->effects.items == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate effect list");
        MemFree(unit);
        return NULL;
    }
    unit->incoming_attacks = listAttackInit(4, perm_allocator());
    if (unit->incoming_attacks.items == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate attack list");
        listMagicEffectDeinit(&unit->effects);
        MemFree(unit);
        return NULL;
    }
    unit->pathfind = listWayPointInit(64, perm_allocator());
    if (unit->pathfind.items == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate pathfinding list");
        listMagicEffectDeinit(&unit->effects);
        listAttackDeinit(&unit->incoming_attacks);
        MemFree(unit);
        return NULL;
    }
    return unit;
}
void unit_deinit(Unit * unit) {
    if (is_unit_tied_to_building(unit))
        unit->origin->units_spawned -= 1;
    unit->waypoint->unit = NULL;
    listMagicEffectDeinit(&unit->effects);
    listAttackDeinit(&unit->incoming_attacks);
    listWayPointDeinit(&unit->pathfind);
    MemFree(unit);
}
Unit * unit_from_building (const Building * building) {
    if (building->units_spawned >= BUILDING_MAX_UNITS)
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
    result->health = get_unit_health(result->type, result->faction, result->upgrade);
    result->upgrade = building->upgrades;
    result->faction = building->region->faction;
    result->position = building->position;
    result->player_owned = building->region->player_id;
    result->state = UNIT_STATE_MOVING;

    result->origin = (Building*) building;
    result->origin->units_spawned += 1;

    result->pathfind.len = 1;
    result->pathfind.items[0] = spawn;
    result->current_path = 0;
    result->waypoint = spawn;
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
void clear_unit_list (ListUnit * list) {
    for(usize i = 0; i < list->len; i++) {
        Unit * unit = list->items[i];
        unit_deinit(unit);
    }
    list->len = 0;
}

/* Visuals *******************************************************************/
void render_units (const GameState * state) {
    const ListUnit * units = &state->units;
    for (usize i = 0; i < units->len; i ++) {
        Unit * unit = units->items[i];
        Color col;
        switch (unit->type) {
            case UNIT_FIGHTER: {
                col = RED;
            } break;
            case UNIT_ARCHER: {
                col = GREEN;
            } break;
            case UNIT_SUPPORT: {
                col = YELLOW;
            } break;
            case UNIT_SPECIAL: {
                col = BLUE;
            } break;
            default: {
                col = BLACK;
            } break;
        }

        Color player = get_player_color(unit->player_owned);
        Color unit_state = GREEN;

        switch (unit->state) {
            case UNIT_STATE_FIGHTING: {
                unit_state = RED;
            } break;
            case UNIT_STATE_MOVING: {
                unit_state = LIME;
            } break;
            case UNIT_STATE_SUPPORTING: {
                unit_state = BLUE;
            } break;
            case UNIT_STATE_GUARDING: {
                TraceLog(LOG_ERROR, "Unit is in guard state, that shouldn't happen!");
                unit_state = PINK;
            } break;
            default: {
                unit_state = PINK;
            } break;
        }
        col.b = unit->player_owned * 127;

        DrawCircleV(unit->position, 7.0f, player);
        DrawCircleV(unit->position, 5.0f, unit_state);
        DrawCircleV(unit->position, 3.0f, col);

        particles_render_attacks(state, unit);
    }

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        DrawCircleV(region->castle.position, 6.0f, RED);

        particles_render_attacks(state, &region->castle);
    }
}
