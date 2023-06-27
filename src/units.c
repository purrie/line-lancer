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
            TraceLog(LOG_ERROR, "Node movement has unexpected value");
            return FAILURE;
        }
    }
    if (next == NULL) {
        TraceLog(LOG_ERROR, "Failed to move node");
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

usize get_unit_range (Unit *const unit) {
    switch(unit->type) {
        case UNIT_FIGHTER:
            return 1;
        case UNIT_ARCHER:
            return 3 + unit->upgrade;
        case UNIT_SUPPORT:
            return 1 + unit->upgrade;
        case UNIT_SPECIAL:
            return 2 + unit->upgrade;
        case UNIT_GUARDIAN:
            return 3;
    }
}

Unit * get_enemy_in_range (Unit *const unit) {
    usize range = get_unit_range(unit);
    Node * node = unit->location;
    Movement direction = unit->move_direction;
    while (range --> 0) {
        if (move_node(&node, &direction)) {
            TraceLog(LOG_ERROR, "Failed to move node to get enemy in range [%d]", range);
            return NULL;
        }
        if (node->unit && node->unit->player_owned != unit->player_owned) {
            return node->unit;
        }
    }
    return NULL;
}

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

Result pass_units(Unit * a, Node * anext, Movement adir, Unit * b) {
    if (is_unit_on_building_path(a)) {
        // bugfix: this prevents units leaving building path from pushing oncoming other units to walk back towards the building
        return FAILURE;
    }
    if (can_unit_progress(b) == NO) {
        // if you want to see something funny, disable this check and give units high movement speed :3c
        return FAILURE;
    }

    if (a->location->bridge == b->location->bridge) {
        if (a->move_direction == b->move_direction) {
            return FAILURE;
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
            return FAILURE;
        }
    }
}

Result move_unit_forward (Unit * unit) {
    Node * next = unit->location;
    Movement dir = unit->move_direction;
    if (move_node(&next, &dir)) {
        TraceLog(LOG_ERROR, "Failed to get next node for unit movement");
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

void clear_unit_list(ListUnit * list) {
    for(usize i = 0; i < list->len; i++) {
        MemFree(list->items[i]);
    }
    list->len = 0;
}

usize destroy_unit(ListUnit * list, Unit * unit) {
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

float get_unit_attack(Unit * unit) {
    float attack;
    switch (unit->type) {
        case UNIT_FIGHTER: {
            attack = 60.0f;
        } break;
        case UNIT_ARCHER: {
            attack = 50.0f;
        } break;
        case UNIT_SUPPORT: {
            attack = 30.0f;
        } break;
        case UNIT_SPECIAL: {
            attack = 100.0f;
        } break;
        case UNIT_GUARDIAN: {
            attack = 10.0f;
        } break;
        default: {
            TraceLog(LOG_ERROR, "Requested attack for unity with no valid type");
            return 1.0f;
        } break;
    }
    attack *= unit->upgrade + 1;
    return attack;
}

Unit * unit_from_building(Building *const building) {
    Node * spawn = building->spawn_paths.items[building->active_spawn].start;
    if (spawn->unit) {
        TraceLog(LOG_WARNING, "Failed to spawn unit because spawn point is occupied by another unit");
        return NULL;
    }
    Unit * result = MemAlloc(sizeof(Unit));
    clear_memory(result, sizeof(Unit));

    switch (building->type) {
        case BUILDING_FIGHTER: {
            result->type = UNIT_FIGHTER;
            result->health = 100.0f;
        } break;
        case BUILDING_ARCHER: {
            result->type = UNIT_ARCHER;
            result->health = 80.0f;
        } break;
        case BUILDING_SUPPORT: {
            result->type = UNIT_SUPPORT;
            result->health = 60.0f;
        } break;
        case BUILDING_SPECIAL: {
            result->type = UNIT_SPECIAL;
            result->health = 120.0f;
        } break;
        default: {
            TraceLog(LOG_ERROR, "Tried to spawn unit from a building that doesn't spawn units: %s", STRINGIFY_VALUE(building->type));
            MemFree(result);
            return NULL;
        } break;
    }

    result->position        = building->position;
    result->upgrade         = building->upgrades;
    result->player_owned    = building->region->player_id;
    result->location        = spawn;
    result->move_direction  = MOVEMENT_DIR_FORWARD;
    result->health         *= result->upgrade + 1;

    spawn->unit = result;
    return result;
}

void unit_guardian (Region * region) {
    region->castle.guardian.health       = 500.0f;
    region->castle.guardian.player_owned = region->player_id;
    region->castle.guardian.position     = region->castle.guardian_spot.position;
    region->castle.guardian.state        = UNIT_STATE_GUARDING;
    region->castle.guardian.type         = UNIT_GUARDIAN;
    region->castle.guardian.location     = &region->castle.guardian_spot;

    region->castle.guardian_spot.unit    = &region->castle.guardian;
}

void render_units(GameState *const state) {
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

    for (usize i = 0; i < state->current_map->regions.len; i++) {
        Region * region = &state->current_map->regions.items[i];
        DrawCircleV(region->castle.guardian.position, 6.0f, RED);
    }
}
