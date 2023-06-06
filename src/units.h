#ifndef UNITS_H_
#define UNITS_H_

#include <raylib.h>
#include "level.h"

typedef enum {
    UNIT_FIGHTER,
    UNIT_ARCHER,
    UNIT_SUPPORT,
    UNIT_SPECIAL,
} UnitType;

typedef enum {
    UNIT_STATE_APPROACHING_ROAD = 0,
    UNIT_STATE_FOLLOWING_ROAD,
    UNIT_STATE_ATTACKING,
    UNIT_STATE_HURT,
    UNIT_STATE_DEFENDING_REGION,
    UNIT_STATE_ATTACKING_REGION,
} UnitState;

typedef struct Unit Unit;

struct Unit {
    Vector2     position;
    UnitType    type;
    ushort      upgrade;
    UnitState   state;
    float       progress;
    Path      * path;
    Region    * region;
    Unit      * target;
    usize       player_owned;
};

makeList(Unit*, Unit);

Unit * unit_from_building(Building *const building);
void clear_unit_list(ListUnit * list);
void render_units(ListUnit * units);

#endif // UNITS_H_
