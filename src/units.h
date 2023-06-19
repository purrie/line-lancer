#ifndef UNITS_H_
#define UNITS_H_

#include <raylib.h>
#include "level.h"

Test   can_unit_progress  (Unit *const unit);
Unit * unit_from_building (Building *const building);
void   unit_guardian      (Region * region);
void   clear_unit_list    (ListUnit * list);
float  get_unit_attack    (Unit *const unit);
usize  get_unit_range     (Unit *const unit);
Unit * get_enemy_in_range (Unit *const unit);
usize  destroy_unit       (ListUnit * units, Unit * unit);
Result move_unit_forward  (Unit * unit);
void   render_units       (GameState *const state);

#endif // UNITS_H_
