#ifndef UNITS_H_
#define UNITS_H_

#include <raylib.h>
#include "level.h"
#include "pathfinding.h"

/* Info **********************************************************************/
Test is_unit_at_own_region (const Unit * unit);
Test unit_reached_waypoint (const Unit * unit);
Test unit_has_path         (const Unit * unit);
Test unit_has_effect       (const Unit * unit, MagicType type, MagicEffect * found);
Test unit_should_repath    (const Unit * unit);

/* Spawing *******************************************************************/
Unit * unit_from_building  (const Building * building);
Result setup_unit_guardian (Region * region);

/* Despawning ****************************************************************/
void clear_unit_list (ListUnit * list);
void unit_deinit     (Unit * unit);

/* Movement ******************************************************************/
Result unit_progress_path  (Unit * unit);
Result unit_calculate_path (Unit * unit);

/* Combat ********************************************************************/
float  get_unit_attack_damage (const Unit * unit);
usize  get_unit_range         (const Unit * unit);
float  get_unit_health        (UnitType type, FactionType faction, unsigned int upgrades);
float  get_unit_wounds        (const Unit * unit);
Result get_unit_support_power (const Unit * unit, MagicEffect * effect);
float  get_unit_cooldown      (const Unit * unit);
float  get_unit_attack_delay  (const Unit * unit);
float  get_unit_speed         (const Unit * unit);
Unit * get_enemy_in_range     (const Unit * unit);
Unit * get_enemy_in_sight     (const Unit * unit);
Result get_enemies_in_range   (const Unit * unit, ListUnit * result);
Result get_allies_in_range    (const Unit * unit, ListUnit * result);
void   unit_kill              (GameState * state, Unit * unit);

/* Rendering *****************************************************************/
void   render_units       (const GameState * state);

#endif // UNITS_H_
