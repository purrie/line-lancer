#ifndef GAME_H_
#define GAME_H_

#include "types.h"

PlayerData * get_local_player       (const GameState * state);
Result       get_local_player_index (const GameState * state, usize * result);
Color        get_player_color       (usize player_id);

void      game_tick          (GameState * state);
usize     game_winner        (GameState * game);
Result    game_state_prepare (GameState * result, const Map * prefab);
void      game_state_deinit  (GameState * state);

#endif // GAME_H_
