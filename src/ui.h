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
    Rectangle fighter;
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
    Rectangle joystick;
    Rectangle zoom_in;
    Rectangle zoom_out;
} CameraJoystick;

typedef struct {
    Rectangle background;
    Rectangle new_game;
    Rectangle tutorial;
    Rectangle manual;
    Rectangle options;
    Rectangle quit;
    Rectangle title;
    Rectangle media_itch;
    Rectangle media_github;
    Rectangle media_coffee;
    Rectangle media_x;
    Rectangle media_youtube;
    Rectangle media_twitch;
    Rectangle media_discord;
} MainMenuLayout;

typedef enum {
    UI_LAYOUT_LEFT,
    UI_LAYOUT_CENTER,
    UI_LAYOUT_RIGHT,
} UiLayout;

/* Drawers *******************************************************************/
void draw_background (Rectangle area, const Theme * theme);
void draw_title      (const Theme * theme);
void label           (Rectangle area, const char * text, float size, UiLayout layout, const Theme * theme);
void draw_image_button (Rectangle area, Texture2D image, Vector2 cursor, const Theme * theme);

/* Input *********************************************************************/
Result ui_building_action_click (BuildingDialog dialog, Vector2 cursor, BuildingAction * action);
Result ui_building_buy_click    (EmptyDialog dialog, Vector2 cursor, BuildingType * result);

/* Layout ********************************************************************/
MainMenuLayout main_menu_layout (const Theme * theme);
void theme_update (Theme * theme);
Theme theme_setup ();
EmptyDialog    empty_dialog         (Vector2 position, const Theme * theme);
BuildingDialog building_dialog      (Vector2 position, const Theme * theme);
Rectangle      flag_button_position ();
CameraJoystick android_camera_control (const Theme * theme);
Range          map_zoom_range       (const GameState * state);

/* Rendering *****************************************************************/
void          render_interaction_hints (const GameState * state);
ExecutionMode render_main_menu         ();

/* Component Rendering *******************************************************/
void render_simple_map_preview      (Rectangle area, Map * map, const Theme * theme);
void render_player_select           (Rectangle area, GameState * state, int selected_map);
int  render_map_list                (Rectangle area, ListMap * maps, usize from, usize len, const Theme * theme);
void draw_button                    (Rectangle area, const char * text, Vector2 cursor, UiLayout label_layout, const Theme * theme);
Test render_settings                (Rectangle area, Settings * settings, const Assets * assets);
void render_winner                  (const GameState * state, usize winner);
InfoBarAction render_resource_bar   (const GameState * state);
void render_upgrade_building_dialog (const GameState * state);
void render_empty_building_dialog   (const GameState * state);
void render_path_button             (const GameState * state);
void render_camera_controls         (const GameState * state);

#endif // UI_H_
