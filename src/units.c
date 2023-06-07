#include "units.h"
#include "std.h"
#include "game.h"

implementList(Unit*, Unit)

void clear_unit_list(ListUnit * list) {
    for(usize i = 0; i < list->len; i++) {
        MemFree(list->items[i]);
    }
    list->len = 0;
}

void delete_unit(ListUnit * list, Unit * unit) {
    for (usize i = 0; i < list->len; i++) {
        if (list->items[i] == unit) {
            MemFree(unit);
            listUnitRemove(list, i);
            return;
        }
    }
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
    Unit * result = MemAlloc(sizeof(Unit));
    clear_memory(result, sizeof(Unit));
    result->position     = building->position;
    result->upgrade      = building->upgrades;
    result->path         = building->spawn_target;
    result->region       = building->region;
    result->player_owned = building->region->player_owned.value;

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
            MemFree(result);
            return NULL;
        } break;
    }

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
            DrawCircleV(unit->position, 5.0f, VIOLET);
        }
    }
}
