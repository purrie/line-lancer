#ifndef UI_H_
#define UI_H_

#include "game.h"

Rectangle    ui_empty_building_dialog_rect      (Vector2 building_position);
BuildingType ui_get_empty_building_dialog_click (Vector2 building_position);
void         render_ui                          (GameState *const state);

#endif // UI_H_
