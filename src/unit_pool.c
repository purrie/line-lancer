#include "unit_pool.h"
#include "std.h"

Unit pool[MAX_UNITS];
Unit * unused_pool[MAX_UNITS];
Unit * used_pool[MAX_UNITS];
ListUnit unused;
ListUnit used;

void unit_pool_init () {
    unused = (ListUnit) { .items = unused_pool, .cap = MAX_UNITS };
    used = (ListUnit) { .items = used_pool, .cap = MAX_UNITS };
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
}
void unit_pool_reset () {
    used.len = 0;
    unused.len = 0;
    for (usize i = 0; i < MAX_UNITS; i++) {
        listUnitAppend(&unused, &pool[i]);
    }
}
ListUnit unit_pool_get_new () {
    if (unused.len != MAX_UNITS) {
        TraceLog(LOG_WARNING, "Provided new pool without resetting the old one!");
    }
    return used;
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
