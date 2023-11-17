#ifndef UI_H_
#define UI_H_

#include "game.h"

typedef enum {
    BUILDING_ACTION_NONE,
    BUILDING_ACTION_UPGRADE,
    BUILDING_ACTION_DEMOLISH,
} BuildingAction;

typedef enum {
    INFO_BAR_ACTION_NONE,
    INFO_BAR_ACTION_SETTINGS,
    INFO_BAR_ACTION_QUIT,
} InfoBarAction;

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
    Rectangle options;
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
void theme_update (Theme * theme);
Theme theme_setup ();

/* Rendering *****************************************************************/
void          render_interaction_hints (const GameState * state);
ExecutionMode render_main_menu         ();

/* Component Rendering *******************************************************/
void render_simple_map_preview      (Rectangle area, Map * map, const Theme * theme);
void render_player_select           (Rectangle area, GameState * state, int selected_map);
int  render_map_list                (Rectangle area, ListMap * maps, usize from, usize len, const Theme * theme);
void draw_button                    (Rectangle area, char * text, Vector2 cursor, UiLayout label_layout, const Theme * theme);
Test render_settings                (Rectangle area, Settings * settings, const Assets * assets);
InfoBarAction render_resource_bar   (const GameState * state);
void render_upgrade_building_dialog (const GameState * state);
void render_empty_building_dialog   (const GameState * state);

#endif // UI_H_
