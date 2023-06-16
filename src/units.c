#include "units.h"
#include "std.h"
#include "game.h"

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

Result move_unit_forward (Unit * unit) {
    Node * next = unit->location;
    Movement dir = unit->move_direction;
    if (move_node(&next, &dir)) {
        TraceLog(LOG_ERROR, "Failed to get next node for unit movement");
        return FAILURE;
    }
    if (next->unit) {
        // TODO make units traversing in opposite direction pass each other
        if (next->unit->player_owned == unit->player_owned) {
            if (next->unit->move_direction != unit->move_direction) {
                unit->location->unit = next->unit;
                next->unit->location = unit->location;
                goto move;
            }
        }
        return FAILURE;
    }
    unit->location->unit = NULL;
    move:
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
        return NULL;
    }
    Unit * result = MemAlloc(sizeof(Unit));
    clear_memory(result, sizeof(Unit));
    result->position     = building->position;
    result->upgrade      = building->upgrades;
    result->player_owned = building->region->player_id;
    result->location     = spawn;

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

    spawn->unit = result;
    result->health *= result->upgrade + 1;
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
        Color col = BLACK;
        switch (unit->state) {
            case UNIT_STATE_FIGHTING: {
                col.r = 255;
            } break;
            case UNIT_STATE_MOVING: {
                col.g = 255;
            } break;
            case UNIT_STATE_GUARDING: {
                TraceLog(LOG_ERROR, "Unit is in guard state, that shouldn't happen!");
            } break;
        }
        col.b = unit->player_owned * 127;

        DrawCircleV(unit->position, 5.0f, col);
    }

    for (usize i = 0; i < state->current_map->regions.len; i++) {
        Region * region = &state->current_map->regions.items[i];
        DrawCircleV(region->castle.guardian.position, 6.0f, RED);
    }
}
