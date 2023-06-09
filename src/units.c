#include "units.h"
#include "std.h"
#include "game.h"
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

/* Combat ********************************************************************/
usize get_unit_range (Unit *const unit) {
    switch(unit->type) {
        case UNIT_FIGHTER:
            return 1;
        case UNIT_ARCHER:
            return 3 + unit->upgrade;
        case UNIT_SUPPORT:
            return 1 + unit->upgrade;
        case UNIT_SPECIAL:
            switch (unit->faction) {
                case FACTION_KNIGHTS: return 2;
                case FACTION_MAGES: return 1;
            }
        case UNIT_GUARDIAN:
            // TODO this probably won't be used since the guardian needs to always reach any attacker
            // Otherwise some units will be able to defeat guardian without a fight
            // This could be alievated if the guardian could move but that's not something I see happening
            switch (unit->faction) {
                case FACTION_KNIGHTS: return 2;
                case FACTION_MAGES: return 5;
            }
    }
}

float get_unit_attack (Unit * unit) {
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
                    attack = 10.0f * (unit->upgrade + 1);
                } break;
                case FACTION_MAGES: {
                    attack = 10.0f * (unit->upgrade + 1);
                } break;
            }
        } break;
        default: {
            TraceLog(LOG_ERROR, "Requested attack for unit with no valid type");
            return 1.0f;
        } break;
    }
    return attack;
}

float get_unit_health (UnitType type, FactionType faction, unsigned int upgrades) {
    switch (type) {
        case UNIT_FIGHTER:
            switch (faction) {
                case FACTION_KNIGHTS: return 100.0f * (upgrades + 1);
                case FACTION_MAGES:   return 140.0f * (upgrades + 1);
            }
        case UNIT_ARCHER:
            switch (faction) {
                case FACTION_KNIGHTS: return 80.0f * (upgrades + 1);
                case FACTION_MAGES:   return 70.0f * (upgrades + 1);
            }
        case UNIT_SUPPORT:
            switch (faction) {
                case FACTION_KNIGHTS: return 60.0f * (upgrades + 1);
                case FACTION_MAGES:   return 60.0f * (upgrades + 1);
            }
        case UNIT_SPECIAL:
            switch (faction) {
                case FACTION_KNIGHTS: return 120.0f * (upgrades + 1);
                case FACTION_MAGES:   return 50.0f * (upgrades + 1);
            }
        case UNIT_GUARDIAN:
            switch (faction) {
                case FACTION_KNIGHTS: return 2000.0f;
                case FACTION_MAGES:   return 2000.0f;
            }
    }
    TraceLog(LOG_ERROR, "Tried to obtain health of unsupported unit type");
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

/* Movement ******************************************************************/
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

Result step_over_unit (Unit * a, Node * anext, Movement adir) {
    Unit * b = anext->unit;
    usize guard = get_unit_range(b);
    while (b != NULL) {
        if (guard == 0) {
            return FAILURE;
        }
        if (b->state != UNIT_STATE_FIGHTING) {
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
        return FAILURE;
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

/* Setup *********************************************************************/
void clear_unit_list (ListUnit * list) {
    for(usize i = 0; i < list->len; i++) {
        MemFree(list->items[i]);
    }
    list->len = 0;
}

usize destroy_unit (ListUnit * list, Unit * unit) {
    for (usize i = 0; i < list->len; i++) {
        if (list->items[i] == unit) {
            unit->location->unit = NULL;
            MemFree(unit);
            listUnitRemove(list, i);
            return i;
        }
    }
    TraceLog(LOG_ERROR, "Can't destroy the unit, it's not in the list of units");
    return list->cap;
}

Unit * unit_from_building (Building *const building) {
    Node * spawn = building->spawn_paths.items[building->active_spawn].start;
    if (spawn->unit) {
        // no spawn because the path is blocked. gotta wait until the unit moves
        return NULL;
    }
    Unit * result = MemAlloc(sizeof(Unit));
    clear_memory(result, sizeof(Unit));

    switch (building->type) {
        case BUILDING_FIGHTER: {
            result->type = UNIT_FIGHTER;
        } break;
        case BUILDING_ARCHER: {
            result->type = UNIT_ARCHER;
        } break;
        case BUILDING_SUPPORT: {
            result->type = UNIT_SUPPORT;
        } break;
        case BUILDING_SPECIAL: {
            result->type = UNIT_SPECIAL;
        } break;
        default: {
            TraceLog(LOG_ERROR, "Tried to spawn unit from a building that doesn't spawn units");
            MemFree(result);
            return NULL;
        } break;
    }

    result->position        = building->position;
    result->upgrade         = building->upgrades;
    result->player_owned    = building->region->player_id;
    result->faction         = building->region->faction;
    result->location        = spawn;
    result->move_direction  = MOVEMENT_DIR_FORWARD;
    result->health          = get_unit_health(result->type, result->faction, result->upgrade);

    spawn->unit = result;
    return result;
}

void setup_unit_guardian (Region * region) {
    region->castle.guardian.health       = get_unit_health(UNIT_GUARDIAN, region->faction, 0);
    region->castle.guardian.player_owned = region->player_id;
    region->castle.guardian.faction      = region->faction;
    region->castle.guardian.position     = region->castle.guardian_spot.position;
    region->castle.guardian.state        = UNIT_STATE_GUARDING;
    region->castle.guardian.type         = UNIT_GUARDIAN;

    region->castle.guardian.location     = &region->castle.guardian_spot;
    region->castle.guardian_spot.unit    = &region->castle.guardian;
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
        Color state = GREEN;

        switch (unit->state) {
            case UNIT_STATE_FIGHTING: {
                state = RED;
            } break;
            case UNIT_STATE_MOVING: {
                state = LIME;
            } break;
            case UNIT_STATE_GUARDING: {
                TraceLog(LOG_ERROR, "Unit is in guard state, that shouldn't happen!");
            } break;
            default: {
                state = PINK;
            } break;
        }
        col.b = unit->player_owned * 127;

        DrawCircleV(unit->position, 7.0f, player);
        DrawCircleV(unit->position, 5.0f, state);
        DrawCircleV(unit->position, 3.0f, col);
    }

    for (usize i = 0; i < state->map.regions.len; i++) {
        Region * region = &state->map.regions.items[i];
        DrawCircleV(region->castle.guardian.position, 6.0f, RED);
    }
}
