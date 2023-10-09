#ifndef UI_H_
#define UI_H_

#include "game.h"

typedef enum {
    BUILDING_ACTION_NONE,
    BUILDING_ACTION_UPGRADE,
    BUILDING_ACTION_DEMOLISH,
} BuildingAction;

typedef struct {
    Rectangle area;
    Rectangle warrior;
    Rectangle archer;
    Rectangle support;
    Rectangle special;
    Rectangle resource;
} EmptyDialog;

typedef struct {
    Rectangle area;
    Rectangle label;
    Rectangle upgrade;
    Rectangle demolish;
} BuildingDialog;

typedef struct {
    Rectangle new_game;
    Rectangle quit;
} MainMenuLayout;

typedef enum {
    UI_LAYOUT_LEFT,
    UI_LAYOUT_CENTER,
    UI_LAYOUT_RIGHT,
} UiLayout;


/* Input *********************************************************************/
Result ui_building_action_click (const GameState * state, Vector2 cursor, BuildingAction * action);
Result ui_building_buy_click    (const GameState * state, Vector2 cursor, BuildingType * result);

/* Layout ********************************************************************/
MainMenuLayout main_menu_layout ();

/* Rendering *****************************************************************/
void          render_ingame_ui (const GameState * state);
ExecutionMode render_main_menu ();

/* Component Rendering *******************************************************/
void render_simple_map_preview (Rectangle area, Map * map, float region_size, float path_thickness);
int  render_map_list           (Rectangle area, ListMap * maps, usize from, usize len);
void draw_button               (Rectangle area, char * text, Vector2 cursor, UiLayout label_layout, Color bg, Color hover, Color frame);

#endif // UI_H_
