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

    if (next->bridge != orig->bridge) {
        if (next == next->bridge->start) {
            *direction = MOVEMENT_DIR_FORWARD;
        }
        else {
            *direction = MOVEMENT_DIR_BACKWARD;
        }
    }
    *from = next;

    return SUCCESS;
}

Node * next_node (Node *const from, Movement direction) {
    switch (direction) {
        case MOVEMENT_DIR_FORWARD:
            return from->next;
        case MOVEMENT_DIR_BACKWARD:
            return from->previous;
    }
    TraceLog(LOG_ERROR, "Failed to get next node");
    return NULL;
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

Result move_unit_forward ( Unit * unit ) {
    Node * next = unit->location;
    Movement dir = unit->move_direction;
    if (move_node(&next, &dir)) {
        TraceLog(LOG_ERROR, "Failed to get next node for unit movement");
        return FAILURE;
    }
    if (next->unit) {
        // TODO make units traversing in opposite direction pass each other
        return FAILURE;
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
    result->player_owned = building->region->player_owned.value;
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

void render_units(ListUnit *const units) {
    for (usize i = 0; i < units->len; i ++) {
        Unit * unit = units->items[i];
        if (unit->player_owned == 1) {
            DrawCircleV(unit->position, 5.0f, BLACK);
        }
        else {
            DrawCircleV(unit->position, 5.0f, ORANGE);
        }
    }
}
