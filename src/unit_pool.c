#include "unit_pool.h"
#include "std.h"

#define MAX_UNITS 1000
Unit pool[MAX_UNITS];
ListUnit unused;

void unit_pool_init () {
    clear_memory(pool, sizeof(Unit) * MAX_UNITS);
    unused = listUnitInit(MAX_UNITS, perm_allocator());
    for (usize i = 0; i < MAX_UNITS; i++) {
        pool[i].pathfind = listWayPointInit(10, perm_allocator());
        pool[i].incoming_attacks = listAttackInit(10, perm_allocator());
        pool[i].effects = listMagicEffectInit(MAGIC_TYPE_LAST + 1, perm_allocator());
        listUnitAppend(&unused, &pool[i]);
    }
}
void unit_pool_deinit () {
    for (usize i = 0; i < MAX_UNITS; i++) {
        listWayPointDeinit(&pool[i].pathfind);
        listAttackDeinit(&pool[i].incoming_attacks);
        listMagicEffectDeinit(&pool[i].effects);
    }
    listUnitDeinit(&unused);
}
Unit * unit_alloc () {
    if (unused.len == 0) return NULL;

    unused.len --;
    Unit * unit = unused.items[unused.len];

    ListWayPoint pathfind = unit->pathfind;
    ListAttack attack = unit->incoming_attacks;
    ListMagicEffect magic = unit->effects;

    clear_memory(unit, sizeof(Unit));

    pathfind.len = 0;
    attack.len = 0;
    magic.len = 0;
    unit->pathfind = pathfind;
    unit->incoming_attacks = attack;
    unit->effects = magic;

    return unit;
}
void unit_release (Unit * unit) {
    listUnitAppend(&unused, unit);
}
