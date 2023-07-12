#ifndef GAME_H_
#define GAME_H_

#include "types.h"

PlayerData * get_local_player       (GameState *const state);
Result       get_local_player_index (GameState *const state, usize * result);
Color        get_player_color       (usize player_id);

void      game_tick          (GameState * state);
Result    game_state_prepare (GameState * result, Map *const prefab);
void      game_state_deinit  (GameState * state);

#endif // GAME_H_
