#ifndef UNITS_H_
#define UNITS_H_

#include <raylib.h>
#include "level.h"

Unit * unit_from_building (Building *const building);
void   clear_unit_list    (ListUnit * list);
void   render_units       (ListUnit *const units);
float  get_unit_attack    (Unit *const unit);
usize  get_unit_range     (Unit *const unit);
Node * get_next_node      (Unit *const unit);
Unit * get_enemy_in_range (Unit *const unit);
usize  destroy_unit       (ListUnit * units, Unit * unit);

#endif // UNITS_H_
