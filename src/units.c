#include "units.h"
#include "std.h"
#include "game.h"
#include "bridge.h"
#include "constants.h"
#include <raymath.h>

Result move_node (Node ** from, Movement * direction) {
    Node * orig = *from;
    Node * next = NULL;
    switch (*direction) {
        case MOVEMENT_DIR_FORWARD: {
            next = orig->next;
        } break;
        case MOVEMENT_DIR_BACKWARD: {
            next = orig->previous;
        } break;
        default: {
            return FAILURE;
        }
    }
    if (next == NULL) {
        return FAILURE;
    }

    if (next->bridge) {
        if (next->bridge != orig->bridge) {
            if (next == next->bridge->start) {
                *direction = MOVEMENT_DIR_FORWARD;
            }
            else {
                *direction = MOVEMENT_DIR_BACKWARD;
            }
        }
    }
    else {
        *direction = MOVEMENT_INVALID;
    }
    *from = next;

    return SUCCESS;
}

/* Info **********************************************************************/
Test is_unit_on_building_path (Unit *const unit) {
    if (unit->location->bridge->start->previous == NULL)
        return YES;
    return NO;
}
Test can_unit_progress (Unit *const unit) {
    const float min = 0.1f * 0.1f;
    if (Vector2DistanceSqr(unit->position, unit->location->position) < min) {
        return YES;
    }
    return NO;
}
Test is_unit_at_path_end (Unit *const unit) {
    switch (unit->move_direction) {
        case MOVEMENT_DIR_BACKWARD: {
            if (unit->location->previous && unit->location->previous->bridge != unit->location->bridge)
                return YES;
        } break;
        case MOVEMENT_DIR_FORWARD: {
            if (unit->location->next && unit->location->next->bridge != unit->location->bridge)
                return YES;
        } break;
        default: return NO;
    }
    return NO;
}
Test is_unit_at_main_path (Unit *const unit, ListPath *const paths) {
    for (usize i = 0; i < paths->len; i++) {
        if (unit->location->bridge == &paths->items[i].bridge)
            return YES;
    }
    return NO;
}
Test is_unit_at_own_region (Unit *const unit, Map *const map) {
    Region * reg = map_get_region_at(map, unit->position);
    if (reg && reg->player_id == unit->player_owned) {
        return YES;
    }
    return NO;
}
Test can_move_forward (Unit *const unit) {
    switch (unit->move_direction) {
        case MOVEMENT_DIR_BACKWARD: {
            if (unit->location->previous == NULL)
                return NO;
        } break;
        case MOVEMENT_DIR_FORWARD: {
            if (unit->location->next == NULL)
                return NO;
        } break;
        default: return NO;
    }
    return YES;
}
Test is_unit_tied_to_building (Unit *const unit) {
    if (unit->origin &&
        unit->origin->region->player_id == unit->player_owned &&
        unit->origin->units_spawned > 0)
        return YES;
    return NO;
}
Test unit_has_effect (Unit *const unit, MagicType type, MagicEffect * found) {
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

/* Combat ********************************************************************/
usize get_unit_range (Unit *const unit) {
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
            // TODO this probably won't be used since the guardian needs to always reach any attacker
            // Otherwise some units will be able to defeat guardian without a fight
            // This could be alievated if the guardian could move but that's not something I see happening
            switch (unit->faction) {
                case FACTION_KNIGHTS: return 2;
                case FACTION_MAGES: return 5;
            }
            break;
        default: {}
    }
    TraceLog(LOG_ERROR, "Failed to resolve unit to get range");
    return 1;
}
float get_unit_attack (Unit *const unit) {
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
Result get_unit_support_power (Unit *const unit, MagicEffect * effect) {
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
                case FACTION_KNIGHTS: return 2000.0f;
                case FACTION_MAGES:   return 2000.0f;
            }
            break;
    }
    TraceLog(LOG_ERROR, "Tried to obtain health of unsupported unit type");
    return 1.0f;
}
float get_unit_wounds (Unit *const unit) {
    return get_unit_health(unit->type, unit->faction, unit->upgrade) - unit->health;
}
usize get_unit_cooldown (Unit *const unit) {
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
float get_unit_attack_delay  (Unit *const unit) {
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
Unit * get_enemy_in_range (Unit *const unit) {
    usize range = get_unit_range(unit);
    Node * node = unit->location;
    Movement direction = unit->move_direction;
    while (range --> 0) {
        if (move_node(&node, &direction)) {
            return NULL;
        }
        if (node->unit && node->unit->player_owned != unit->player_owned) {
            return node->unit;
        }
    }
    return NULL;
}
Result get_enemies_in_range (Unit *const unit, ListUnit * result) {
    result->len = 0;
    usize range = get_unit_range(unit);
    Node * node = unit->location->next;
    for (usize i = 0; i < range; i++) {
        if (node == NULL)
            break;
        if (node->unit && node->unit->player_owned != unit->player_owned) {
            if (listUnitAppend(result, node->unit)) {
                return FAILURE;
            }
        }
        node = node->next;
    }
    node = unit->location->previous;
    for (usize i = 0; i < range; i++) {
        if (node == NULL)
            break;
        if (node->unit && node->unit->player_owned != unit->player_owned) {
            if (listUnitAppend(result, node->unit)) {
                return FAILURE;
            }
        }
        node = node->previous;
    }
    return SUCCESS;
}
Result get_allies_in_range (Unit *const unit, ListUnit * result) {
    result->len = 0;
    usize range = get_unit_range(unit);
    Node * node = unit->location->next;
    for (usize i = 0; i < range; i++) {
        if (node == NULL)
            break;
        if (node->unit && node->unit->player_owned == unit->player_owned) {
            if (listUnitAppend(result, node->unit)) {
                return FAILURE;
            }
        }
        node = node->next;
    }
    node = unit->location->previous;
    for (usize i = 0; i < range; i++) {
        if (node == NULL)
            break;
        if (node->unit && node->unit->player_owned == unit->player_owned) {
            if (listUnitAppend(result, node->unit)) {
                return FAILURE;
            }
        }
        node = node->previous;
    }
    return SUCCESS;
}
void unit_cursify (Unit * unit, usize player_source, PlayerData *const curser) {
    unit->effects.len = 0;
    unit->incoming_attacks.len = 0;
    unit->faction = curser->faction;
    unit->type = UNIT_SPECIAL;
    unit->health = get_unit_health(UNIT_SPECIAL, curser->faction, 0);
    unit->move_direction = !unit->move_direction;
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
Result step_over_unit (Unit * a, Node * anext, Movement adir) {
    Unit * b = anext->unit;
    usize guard = get_unit_range(b);
    while (b != NULL) {
        if (guard == 0) {
            return FAILURE;
        }
        if (b->state == UNIT_STATE_MOVING) {
            return FAILURE;
        }
        if (b->player_owned != a->player_owned) {
            return FAILURE;
        }
        if (move_node(&anext, &adir)) {
            return FAILURE;
        }
        b = anext->unit;
        guard--;
    }
    a->location->unit = NULL;
    a->location = anext;
    a->location->unit = a;
    a->move_direction = adir;
    return SUCCESS;
}
Result pass_units (Unit * a, Node * anext, Movement adir, Unit * b) {
    if (is_unit_on_building_path(a) && is_unit_on_building_path(b) == NO) {
        // bugfix: this prevents units leaving building path from pushing oncoming other units to walk back towards the building
        return FAILURE;
    }

    // if you want to see something funny, disable this check and give units high movement speed :3c
    if (can_unit_progress(b) == NO) {
        if (move_node(&anext, &adir)) {
            return FAILURE;
        }
        if (anext->unit == NULL) {
            a->location->unit = NULL;
            a->location = anext;
            a->location->unit = a;
            a->move_direction = adir;
            return SUCCESS;
        }
        return FAILURE;
    }

    if (a->location->bridge == b->location->bridge) {
        if (a->move_direction == b->move_direction) {
            return step_over_unit(a, anext, adir);
        }
        b->location = a->location;
        a->location = anext;
        b->location->unit = b;
        a->location->unit = a;
        return SUCCESS;
    }
    else {
        bool direction_aligned = (a->location->bridge->start == a->location) == (b->location->bridge->end == b->location);
        bool crossing = adir != b->move_direction;

        if (crossing) {
            b->location = a->location;
            a->location = anext;
            b->location->unit = b;
            a->location->unit = a;
            a->move_direction = adir;
            if (! direction_aligned)
                b->move_direction = b->move_direction == MOVEMENT_DIR_FORWARD ? MOVEMENT_DIR_BACKWARD : MOVEMENT_DIR_FORWARD;
            return SUCCESS;
        }
        else {
            return step_over_unit(a, anext, adir);
        }
    }
}
Result move_unit_forward (Unit * unit) {
    Node * next = unit->location;
    Movement dir = unit->move_direction;
    if (move_node(&next, &dir)) {
        return FAILURE;
    }
    if (dir == MOVEMENT_INVALID) {
        TraceLog(LOG_WARNING, "Next node has invalid movement, it's not connected to a bridge");
        return FATAL;
    }

    if (next->unit) {
        return pass_units(unit, next, dir, next->unit);
    }

    unit->location->unit = NULL;
    unit->location = next;
    unit->location->unit = unit;
    unit->move_direction = dir;
    return SUCCESS;
}
Result move_unit_towards (Unit * unit, Node * node) {
    Movement move = move_direction(unit->location, node);
    if (move == MOVEMENT_INVALID)
        return FAILURE;
    if (node->unit) {
        return pass_units(unit, node, move, node->unit);
    }
    unit->location->unit = NULL;
    unit->location = node;
    unit->location->unit = unit;
    unit->move_direction = move;
    return SUCCESS;
}
Movement move_direction (Node *const from, Node *const to) {
    if (from->bridge == to->bridge) {
        if (from->next == to)
            return MOVEMENT_DIR_FORWARD;
        if (from->previous == to)
            return MOVEMENT_DIR_BACKWARD;
    }
    else {
        if (to->bridge) {
            if (to->bridge->start == to)
                return MOVEMENT_DIR_FORWARD;
            else
                return MOVEMENT_DIR_BACKWARD;
        }
    }
    return MOVEMENT_INVALID;
}

/* Setup *********************************************************************/
Unit * unit_init() {
    Unit * unit = MemAlloc(sizeof(Unit));
    if (unit == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for unit");
        return NULL;
    }
    clear_memory(unit, sizeof(Unit));

    unit->effects = listMagicEffectInit(MAGIC_TYPE_LAST + 1, &MemAlloc, &MemFree);
    if (unit->effects.items == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate effect list");
        MemFree(unit);
        return NULL;
    }
    unit->incoming_attacks = listAttackInit(4, &MemAlloc, &MemFree);
    if (unit->incoming_attacks.items == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate attack list");
        MemFree(unit);
        return NULL;
    }
    return unit;
}
void unit_deinit(Unit * unit) {
    if (is_unit_tied_to_building(unit))
        unit->origin->units_spawned -= 1;
    unit->location->unit = NULL;
    listMagicEffectDeinit(&unit->effects);
    listAttackDeinit(&unit->incoming_attacks);
    MemFree(unit);
}
Unit * unit_from_building (Building *const building) {
    if (building->units_spawned >= BUILDING_MAX_UNITS)
        return NULL;

    Bridge * castle_path = building->defend_paths.items[building->active_spawn].end->next->bridge;
    Node * spawn;

    if (bridge_is_enemy_present(castle_path, building->region->player_id))
        spawn = building->defend_paths.items[building->active_spawn].start;
    else
        spawn = building->spawn_paths.items[building->active_spawn].start;

    if (spawn->unit) {
        // no spawn because the path is blocked. gotta wait until the unit moves
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
    result->location = spawn;
    result->position = building->position;
    result->player_owned = building->region->player_id;
    result->move_direction = MOVEMENT_DIR_FORWARD;

    result->origin = building;
    result->origin->units_spawned += 1;

    spawn->unit = result;
    return result;
}
void setup_unit_guardian (Region * region) {
    Unit * guardian = &region->castle.guardian;
    guardian->health       = get_unit_health(UNIT_GUARDIAN, region->faction, 0);
    guardian->player_owned = region->player_id;
    guardian->faction      = region->faction;
    guardian->position     = region->castle.guardian_spot.position;
    guardian->state        = UNIT_STATE_GUARDING;
    guardian->type         = UNIT_GUARDIAN;

    if (guardian->effects.items == NULL) {
        guardian->effects = listMagicEffectInit(MAGIC_TYPE_LAST + 1, &MemAlloc, &MemFree);
    }
    else {
        guardian->effects.len = 0;
    }
    if (guardian->incoming_attacks.items == NULL) {
        guardian->incoming_attacks = listAttackInit(6, &MemAlloc, &MemFree);
    }
    else {
        guardian->incoming_attacks.len = 0;
    }

    guardian->location = &region->castle.guardian_spot;
    region->castle.guardian_spot.unit = guardian;
}
void clear_unit_list (ListUnit * list) {
    for(usize i = 0; i < list->len; i++) {
        Unit * unit = list->items[i];
        unit_deinit(unit);
    }
    list->len = 0;
}
/* Visuals *******************************************************************/
void render_units (GameState *const state) {
    ListUnit * units = &state->units;
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
    }

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        DrawCircleV(region->castle.guardian.position, 6.0f, RED);
    }
}
