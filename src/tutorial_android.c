#if defined(ANDROID)
#include <math.h>
#include "tutorial.h"
#include "std.h"
#include "string.h"
#include "game.h"
#include "ui.h"
#include "audio.h"
#include "level.h"
#include "units.h"
#include "cake.h"
#include "particle.h"
#include "input.h"

const char * blank_line = " ";

// separated just to make it easier to center the text lines
const char * tutorial_introduction1 = "In this game your goal is to capture all regions from your opponents";
const char * tutorial_introduction2 = "In turn, you have to defend your own regions from being captured";

const char * tutorial_introduction3 = "Tap here to continue";

#define TUT_INTROS(X, a, b) \
    X(tutorial_introduction1, a, b) \
    X(tutorial_introduction2, a, b) \
    X(blank_line, a, b) \
    X(tutorial_introduction3, a, b)

const char * tutorial_camera1 = "You can use the joystick in lower left to move the camera";
const char * tutorial_camera2 = "Buttons around it allow you to zoom in and out as well";

const char * tutorial_camera3 = "You can also press on the screen and drag the camera around freely";

const char * tutorial_camera4 = "Move the camera onto one of the empty pink building spots to continue";

#define TUT_CAMERA(X, a, b) \
    X(tutorial_camera1, a, b) \
    X(tutorial_camera2, a, b) \
    X(blank_line, a, b) \
    X(tutorial_camera3, a, b) \
    X(blank_line, a, b) \
    X(tutorial_camera4, a, b)

const char * tutorial_buildings1 = "To conquer other regions you must first purchase buildings";
const char * tutorial_buildings2 = "Buildings will automatically produce warriors for you";
const char * tutorial_buildings3 = "You can place buildings on those spots empty spots";

const char * tutorial_buildings4 = "Tap here to continue";

#define TUT_BUILDINGS(X, a, b) \
    X(tutorial_buildings1, a, b) \
    X(tutorial_buildings2, a, b) \
    X(tutorial_buildings3, a, b) \
    X(blank_line, a, b) \
    X(tutorial_buildings4, a, b)

const char * tutorial_units1 = "There are 4 types of units in this game.";
const char * tutorial_units2 = "Each has unique streangths and weaknesess.";
const char * tutorial_units3 = "To easily conquer territory it is best to start with fighters";

const char * tutorial_units4 = "Purchase Milita Barracks to continue";

#define TUT_UNITS(X, a, b) \
    X(tutorial_units1, a, b) \
    X(tutorial_units2, a, b) \
    X(tutorial_units3, a, b) \
    X(blank_line, a, b) \
    X(tutorial_units4, a, b)

const char * tutorial_paths1 = "Once your units spawn, they will act independently";
const char * tutorial_paths2 = "They will fight when they encounter enemies";
const char * tutorial_paths3 = "And will walk towards neighboring regions otherwise";

const char * tutorial_paths4 = "You can choose which region your units will move towards";
const char * tutorial_paths5 = "Move the aim cursor onto a path ending and press the flag button";
const char * tutorial_paths6 = "When you press the button on active path, the flag will be taken off";
const char * tutorial_paths7 = "The units will then stay within the region until you direct them elsewhere";

const char * tutorial_paths8 = "Tap here to continue";

#define TUT_PATHS(X, a, b) \
    X(tutorial_paths1, a, b) \
    X(tutorial_paths2, a, b) \
    X(tutorial_paths3, a, b) \
    X(blank_line, a, b) \
    X(tutorial_paths4, a, b) \
    X(tutorial_paths5, a, b) \
    X(tutorial_paths6, a, b) \
    X(tutorial_paths7, a, b) \
    X(blank_line, a, b) \
    X(tutorial_paths8, a, b)

const char * tutorial_upgrade1 = "Purchased buildings can be upgraded up to level 3.";
const char * tutorial_upgrade2 = "Each upgrade improves the capabilities of units produced there.";
const char * tutorial_upgrade3 = "Be sure to upgrade buildings when you deem that advantageous.";

const char * tutorial_upgrade4 = "Tap here to continue";

#define TUT_UPGRADES(X, a, b) \
    X(tutorial_upgrade1, a, b) \
    X(tutorial_upgrade2, a, b) \
    X(tutorial_upgrade3, a, b) \
    X(blank_line, a, b) \
    X(tutorial_upgrade4, a, b)

const char * tutorial_resources1 = "Each time a warrior spawns, it costs a little bit of resources.";
const char * tutorial_resources2 = "You can see current amount of resources you have up here in the HUD.";
const char * tutorial_resources3 = "You can also see how much resources you gain per second.";
const char * tutorial_resources4 = "And next to that, you can see how much it costs to recruit from all your buildings.";

const char * tutorial_resources5 = "Your income comes in batches every couple seconds";
const char * tutorial_resources6 = "while resources are taken only when new units spawn.";
const char * tutorial_resources7 = "So use those numbers as an indicator of fluidity of your economy";
const char * tutorial_resources8 = "rather than exact state of it.";

const char * tutorial_resources9 = "Each region you own gives you a little income.";
const char * tutorial_resources10 = "You can also build farms to increase your income further.";

const char * tutorial_resources11 = "Tap here to continue";

#define TUT_RESOURCES(X, a, b) \
    X(tutorial_resources1, a, b) \
    X(tutorial_resources2, a, b) \
    X(tutorial_resources3, a, b) \
    X(tutorial_resources4, a, b) \
    X(blank_line, a, b) \
    X(tutorial_resources5, a, b) \
    X(tutorial_resources6, a, b) \
    X(tutorial_resources7, a, b) \
    X(tutorial_resources8, a, b) \
    X(blank_line, a, b) \
    X(tutorial_resources9, a, b) \
    X(tutorial_resources10, a, b) \
    X(blank_line, a, b) \
    X(tutorial_resources11, a, b)

const char * tutorial_outro1 = "That covers most of what you need to get started.";
const char * tutorial_outro2 = "Experiment and try various strategies out";
const char * tutorial_outro3 = "Good luck and have fun!";

const char * tutorial_outro4 = "Tap here to play";

#define TUT_OUTROS(X, a, b) \
    X(tutorial_outro1, a, b) \
    X(tutorial_outro2, a, b) \
    X(tutorial_outro3, a, b) \
    X(blank_line, a, b) \
    X(tutorial_outro4, a, b)

#define UPDATE_WIDTH(txt, theme, max_width) { int w = MeasureText(txt, theme->font_size); if (w > max_width) max_width = w; }
#define UPDATE_HEIGHT(txt, theme, max_height) { max_height += theme->font_size + theme->spacing; }
#define DRAW_TEXT(txt, theme, rec) { \
    int w = MeasureText(txt, theme->font_size); \
    if (w > rec.width) { \
        rec = cake_grow_to(rec, w, theme->font_size); \
    } \
    else if (w < rec.width) { \
        rec = cake_diet_to(rec, w); \
    } \
    DrawText(txt, rec.x, rec.y, theme->font_size, theme->text); \
    rec.y += theme->font_size + theme->spacing; \
}

typedef enum {
    TUTORIAL_INTRODUCTION,
    TUTORIAL_CAMERA,
    TUTORIAL_BUILDINGS,
    TUTORIAL_UNITS,
    TUTORIAL_PATHS,
    TUTORIAL_UPGRADING,
    TUTORIAL_RESOURCES,
    TUTORIAL_OUTRO,
    TUTORIAL_DONE,
} TutorialStage;

TutorialStage tutorial_stage = TUTORIAL_INTRODUCTION;

Rectangle make_window (float width, float height, const Theme * theme) {
    float scale = theme->margin * 10.0f;
    return cake_rect(width + scale, height + scale);
}
Rectangle make_text_view (Rectangle area, const Theme * theme) {
    return cake_margin_all(area, theme->margin * 5.0f);
}

void draw_tutorial_intro (GameState * game) {
    int max_width = 0;
    int max_height = 0;
    const Theme * theme = &game->settings->theme;
    TUT_INTROS(UPDATE_WIDTH, theme, max_width)
    TUT_INTROS(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);
    screen = cake_center_rect(screen, GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), screen)) {
        tutorial_stage = TUTORIAL_CAMERA;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);

    TUT_INTROS(DRAW_TEXT, theme, screen)
}
void draw_tutorial_camera (GameState * game) {
    const Theme * theme = &game->settings->theme;
    usize player;
    if (get_local_player_index(game, &player)) {
        TraceLog(LOG_FATAL, "Missing player from tutorial!");
    }
    Region * starting_region = NULL;
    for (usize i = 0; i < game->map.regions.len; i++) {
        Region * region = &game->map.regions.items[i];
        if (region->player_id == player) {
            starting_region = region;
            break;
        }
    }
    if (starting_region == NULL) {
        TraceLog(LOG_FATAL, "Player is missing starting region!");
    }
    {
        Vector2 spot;
        usize idx = (usize)( get_time() * 0.5f) % starting_region->buildings.len;
        spot = starting_region->buildings.items[idx].position;
        spot = GetWorldToScreen2D(spot, game->camera);
        spot.y -= 10.0f + sinf(get_time() * 5.0f) * 5.0f;
        Color color = theme->text;
        DrawLineEx(spot, Vector2Subtract(spot, (Vector2){ 0.0f, 60.0f }), 10.0f,  color);
        DrawLineEx(spot, Vector2Subtract(spot, (Vector2){ 20.0f, 30.0f }), 8.0f, color);
        DrawLineEx(spot, Vector2Subtract(spot, (Vector2){ -20.0f, 30.0f }), 8.0f, color);
        DrawCircleV(spot, 3.0f, color);
    }

    int max_width = 0;
    int max_height = 0;
    TUT_CAMERA(UPDATE_WIDTH, theme, max_width)
    TUT_CAMERA(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);
    screen = cake_center_rect(screen, GetScreenWidth() - max_width * 0.75f, GetScreenHeight() - max_height);

    if (game->current_input == INPUT_CLICKED_BUILDING) {
        tutorial_stage = TUTORIAL_BUILDINGS;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);

    TUT_CAMERA(DRAW_TEXT, theme, screen)
}
void draw_tutorial_buildings (GameState * game) {
    const Theme * theme = &game->settings->theme;

    int max_width = 0;
    int max_height = 0;
    TUT_BUILDINGS(UPDATE_WIDTH, theme, max_width)
    TUT_BUILDINGS(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);

    screen = cake_center_rect(screen, GetScreenWidth() - max_width * 0.75f, max_height + theme->info_bar_height);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), screen)) {
        tutorial_stage = TUTORIAL_UNITS;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);

    TUT_BUILDINGS(DRAW_TEXT, theme, screen)
}
void draw_tutorial_units (GameState * game) {
    usize player;
    if (get_local_player_index(game, &player)) {
        TraceLog(LOG_FATAL, "Missing player from tutorial!");
    }
    Region * starting_region = NULL;
    for (usize i = 0; i < game->map.regions.len; i++) {
        Region * region = &game->map.regions.items[i];
        if (region->player_id == player) {
            starting_region = region;
            break;
        }
    }
    if (starting_region == NULL) {
        TraceLog(LOG_FATAL, "Player is missing starting region!");
    }
    for (usize i = 0; i < starting_region->buildings.len; i++) {
        Building * building = &starting_region->buildings.items[i];
        if (building->type == BUILDING_EMPTY) continue;
        if (building->type == BUILDING_FIGHTER) tutorial_stage = TUTORIAL_PATHS;
        else {
            usize cost = building_buy_cost(building->type);
            game->players.items[player].resource_gold += cost;
            demolish_building(building);
        }
    }

    const Theme * theme = &game->settings->theme;
    float max_width = 0;
    float max_height = 0;
    TUT_UNITS(UPDATE_WIDTH, theme, max_width)
    TUT_UNITS(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);

    screen = cake_center_rect(screen, GetScreenWidth() - max_width * 0.75f, max_height + theme->info_bar_height);

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);
    TUT_UNITS(DRAW_TEXT, theme, screen)
}
void draw_tutorial_paths (GameState * game) {
    const Theme * theme = &game->settings->theme;
    float max_width = theme->margin * 2.0f;
    float max_height = theme->margin * 2.0f;
    TUT_PATHS(UPDATE_WIDTH, theme, max_width)
    TUT_PATHS(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);

    screen = cake_center_rect(screen, GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), screen)) {
        tutorial_stage = TUTORIAL_UPGRADING;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);
    TUT_PATHS(DRAW_TEXT, theme, screen)
}
void draw_tutorial_upgrades (GameState * game) {
    const Theme * theme = &game->settings->theme;
    float max_width = theme->margin * 2.0f;
    float max_height = theme->margin * 2.0f;
    TUT_UPGRADES(UPDATE_WIDTH, theme, max_width)
    TUT_UPGRADES(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);

    screen = cake_center_rect(screen, GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), screen)) {
        tutorial_stage = TUTORIAL_RESOURCES;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);
    TUT_UPGRADES(DRAW_TEXT, theme, screen)
}
void draw_tutorial_resources (GameState * game) {
    const Theme * theme = &game->settings->theme;
    float max_width = theme->margin * 2.0f;
    float max_height = theme->margin * 2.0f;
    TUT_RESOURCES(UPDATE_WIDTH, theme, max_width)
    TUT_RESOURCES(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);

    screen = cake_move_rect(screen, theme->info_bar_height * 1.5f, theme->info_bar_height * 1.5f);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), screen)) {
        tutorial_stage = TUTORIAL_OUTRO;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);
    TUT_RESOURCES(DRAW_TEXT, theme, screen)
}
void draw_tutorial_outro (GameState * game) {
    const Theme * theme = &game->settings->theme;
    float max_width = theme->margin * 2.0f;
    float max_height = theme->margin * 2.0f;
    TUT_OUTROS(UPDATE_WIDTH, theme, max_width)
    TUT_OUTROS(UPDATE_HEIGHT, theme, max_height)
    Rectangle screen = make_window(max_width, max_height, theme);

    screen = cake_center_rect(screen, GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), screen)) {
        tutorial_stage = TUTORIAL_DONE;
    }

    draw_background(screen, theme);
    screen = make_text_view(screen, theme);
    TUT_OUTROS(DRAW_TEXT, theme, screen)
}

bool draw_tutorial_window (GameState * game) {
    switch (tutorial_stage) {
        case TUTORIAL_INTRODUCTION: draw_tutorial_intro     (game); return false;
        case TUTORIAL_CAMERA:       draw_tutorial_camera    (game); return false;
        case TUTORIAL_BUILDINGS:    draw_tutorial_buildings (game); return false;
        case TUTORIAL_UPGRADING:    draw_tutorial_upgrades  (game); return false;
        case TUTORIAL_UNITS:        draw_tutorial_units     (game); return false;
        case TUTORIAL_PATHS:        draw_tutorial_paths     (game); return false;
        case TUTORIAL_RESOURCES:    draw_tutorial_resources (game); return false;
        case TUTORIAL_OUTRO:        draw_tutorial_outro     (game); return tutorial_stage == TUTORIAL_DONE;
        default:
        case TUTORIAL_DONE: return true;
    }
}

ExecutionMode tutorial_mode (Assets * assets, GameState * game) {
    game->players = listPlayerDataInit(PLAYERS_MAX, perm_allocator());
    clear_memory(game->players.items, sizeof(PlayerData) * game->players.cap);
    game->players.items[0].type = PLAYER_NEUTRAL;
    game->players.items[1].type = PLAYER_LOCAL;
    game->players.items[1].faction = FACTION_KNIGHTS;
    game->players.items[2].type = PLAYER_AI;
    game->players.items[2].faction = FACTION_MAGES;

    tutorial_stage = TUTORIAL_INTRODUCTION;

    Map * map = NULL;
    for (usize i = 0; i < assets->maps.len; i++) {
        if (strcmp(assets->maps.items[i].name, "Lanes") == 0) {
            map = &assets->maps.items[i];
            break;
        }
    }
    if (map == NULL) {
        TraceLog(LOG_ERROR, "Couldn't find tutorial map");
        listPlayerDataDeinit(&game->players);
        return EXE_MODE_MAIN_MENU;
    }

    if (game_state_prepare(game, map)) {
        listPlayerDataDeinit(&game->players);
        return EXE_MODE_MAIN_MENU;
    }

    usize ai_region = game->map.regions.len;
    for (usize i = 0; i < game->map.regions.len; i++) {
        Region * region = &game->map.regions.items[i];
        if (region->player_id == 2) {
            ai_region = i;
            region->player_id = 0;
            break;
        }
    }
    if (ai_region >= game->map.regions.len) {
        TraceLog(LOG_ERROR, "Failed to find AI region in tutorial map");
        goto end;
    }
    for (usize i = 0; i < game->map.regions.len; i++) {
        Region * region = &game->map.regions.items[i];
        if (region->player_id == 1) {
            game->camera.target = region->castle.position;
            break;
        }
    }

    Color player_color = get_player_color(1);
    Music music = assets->faction_themes[FACTION_KNIGHTS];
    PlayMusicStream(music);
    InfoBarAction play_state = INFO_BAR_ACTION_NONE;
    usize winner = 0;

    while (play_state != INFO_BAR_ACTION_QUIT) {
        if (WindowShouldClose()) {
            break;
        }
        BeginDrawing();
        ClearBackground(BLACK);
        UpdateMusicStream(music);
        if (play_state == INFO_BAR_ACTION_NONE) {
            game_tick(game);
            if (tutorial_stage == TUTORIAL_DONE) winner = game_winner(game);
        }

        BeginMode2D(game->camera);
            if (play_state == INFO_BAR_ACTION_NONE) {
                render_interaction_hints(game);
            }
            render_map_mesh(game);
            render_units(game);
            particles_render(game->particles_in_use.items, game->particles_in_use.len);
        EndMode2D();

        if (play_state == INFO_BAR_ACTION_NONE) {
            if (game->current_input != INPUT_MOVE_MAP) {

                if (game->current_input == INPUT_CLICKED_PATH) {
                    render_path_button(game);
                }
                else if (game->current_input == INPUT_CLICKED_BUILDING) {
                    if (game->selected_building->type == BUILDING_EMPTY) {
                        render_empty_building_dialog(game);
                    }
                    else {
                        render_upgrade_building_dialog(game);
                    }
                }

                render_camera_controls(game);
            }
            Vector2 cursor = mouse_position_pointer();
            const Theme * ui_theme = &game->settings->theme;
            Vector2 top_left  = { cursor.x - ui_theme->info_bar_height, cursor.y - ui_theme->info_bar_height };
            Vector2 top_right = { cursor.x + ui_theme->info_bar_height, cursor.y - ui_theme->info_bar_height };
            Vector2 bot_left  = { cursor.x - ui_theme->info_bar_height, cursor.y + ui_theme->info_bar_height };
            Vector2 bot_right = { cursor.x + ui_theme->info_bar_height, cursor.y + ui_theme->info_bar_height };

            DrawLineEx(top_left, bot_right, ui_theme->frame_thickness, player_color);
            DrawLineEx(bot_left, top_right, ui_theme->frame_thickness, player_color);
        }

        if (tutorial_stage != TUTORIAL_DONE) {
            if (draw_tutorial_window(game)) {
                game->map.regions.items[ai_region].player_id = 2;
            }
        }

        if (winner) {
            render_winner(game, winner);
        }

        if (play_state == INFO_BAR_ACTION_SETTINGS) {
            Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
            if (render_settings(screen, (Settings*)game->settings, game->resources)) {
                play_sound(game->resources, SOUND_UI_CLICK);
                play_state = INFO_BAR_ACTION_NONE;
            }
        }

        switch (render_resource_bar(game)) {
            case INFO_BAR_ACTION_NONE: break;
            case INFO_BAR_ACTION_QUIT: {
                play_sound(game->resources, SOUND_UI_CLICK);
                play_state = INFO_BAR_ACTION_QUIT;
            } break;
            case INFO_BAR_ACTION_SETTINGS: {
                play_sound(game->resources, SOUND_UI_CLICK);
                if (play_state == INFO_BAR_ACTION_SETTINGS) {
                    play_state = INFO_BAR_ACTION_NONE;
                }
                else {
                    play_state = INFO_BAR_ACTION_SETTINGS;
                }
            } break;
        }
        EndDrawing();
        temp_reset();
    }

    StopMusicStream(music);
    end:
    game_state_deinit(game);
    return EXE_MODE_MAIN_MENU;
}

#endif
