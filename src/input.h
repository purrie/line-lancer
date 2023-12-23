#ifndef INPUT_H_
#define INPUT_H_

#include "game.h"

// Returns whatever the state has changed
void update_input_state (GameState * state);
Vector2 mouse_position_inworld (Camera2D view);
Vector2 mouse_position_pointer ();

#endif // INPUT_H_
