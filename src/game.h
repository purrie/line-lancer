#ifndef GAME_H_
#define GAME_H_

#include "types.h"

PlayerData * get_local_player       (GameState *const state);
Result       get_local_player_index (GameState *const state, usize * result);
Color        get_player_color       (usize player_id);

void      simulate_units     (GameState * state);
void      update_resources   (GameState * state);
GameState create_game_state  ();
void      destroy_game_state (GameState state);

#endif // GAME_H_
