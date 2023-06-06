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

Unit * unit_from_building(Building *const building) {
    Unit * result = MemAlloc(sizeof(Unit));
    clear_memory(result, sizeof(Unit));
    result->position     = building->position;
    result->upgrade      = building->upgrades;
    result->path         = building->spawn_target.path;
    result->region       = building->region;
    result->player_owned = building->region->player_owned.value;

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
            MemFree(result);
            return NULL;
        } break;
    }

    return result;
}

void render_units(ListUnit * units) {
    for (usize i = 0; i < units->len; i ++) {
        Unit * unit = units->items[i];

        DrawCircleV(unit->position, 5.0f, BLACK);
    }
}
