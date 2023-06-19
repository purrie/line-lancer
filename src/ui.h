#ifndef UI_H_
#define UI_H_

#include "game.h"

typedef enum {
    BUILDING_ACTION_NONE,
    BUILDING_ACTION_UPGRADE,
    BUILDING_ACTION_DEMOLISH,
} BuildingAction;

Result ui_building_action_click (GameState *const state, Vector2 cursor, BuildingAction * action);
Result ui_building_buy_click    (GameState *const state, Vector2 cursor, BuildingType * result);
void   render_ui                (GameState *const state);

#endif // UI_H_
