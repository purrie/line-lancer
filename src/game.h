#ifndef GAME_H_
#define GAME_H_

#include "level.h"
#include "units.h"

typedef enum {
    INPUT_NONE = 0,
    INPUT_OPEN_BUILDING,
    INPUT_START_SET_PATH,
    INPUT_SET_PATH,
} PlayerState;

typedef struct {
    PlayerState   current_input;
    Building    * selected_building;
    Map         * current_map;
    ListUnit      units;
} GameState;

void simulate_units(GameState * state);

#endif // GAME_H_
