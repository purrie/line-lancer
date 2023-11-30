#ifndef AI_H_
#define AI_H_

#include "types.h"

/* Initialization ************************************************************/
void ai_init    (usize player_id, GameState * state);
void ai_deinit  (PlayerData * player);

/* Events ********************************************************************/
void ai_region_lost (usize player_id, GameState * state, Region * region);
void ai_region_gain (usize player_id, GameState * state, Region * region);

/* Simulation ****************************************************************/
void simulate_ai(GameState * state);

#endif // AI_H_
