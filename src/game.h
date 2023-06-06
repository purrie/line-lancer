#ifndef GAME_H_
#define GAME_H_

#include "level.h"

typedef enum {
    INPUT_NONE = 0,
    INPUT_OPEN_BUILDING,
    INPUT_START_SET_PATH,
    INPUT_SET_PATH,
} PlayerState;

typedef struct {
    PlayerState   current_input;
    Building    * selected_building;
    Rectangle     dialog_area;
} GameState;

#endif // GAME_H_
