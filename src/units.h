#ifndef UNITS_H_
#define UNITS_H_

#include <raylib.h>
#include "level.h"

/* Info **********************************************************************/
Test is_unit_at_main_path  (Unit *const unit, ListPath *const paths);
Test is_unit_at_path_end   (Unit *const unit);
Test is_unit_at_own_region (Unit *const unit, Map *const map);
Test can_move_forward      (Unit *const unit);

/* Spawing *******************************************************************/
Unit * unit_from_building  (Building *const building);
void   setup_unit_guardian (Region * region);

/* Despawning ****************************************************************/
void   clear_unit_list    (ListUnit * list);
usize  destroy_unit       (ListUnit * units, Unit * unit);

/* Movement ******************************************************************/
Test     can_unit_progress (Unit *const unit);
Result   move_unit_forward (Unit * unit);
Result   move_unit_towards (Unit * unit, Node * node);
Movement move_direction    (Node *const from, Node *const to);

/* Combat ********************************************************************/
float  get_unit_attack    (Unit *const unit);
usize  get_unit_range     (Unit *const unit);
Unit * get_enemy_in_range (Unit *const unit);

/* Rendering *****************************************************************/
void   render_units       (GameState *const state);

#endif // UNITS_H_
