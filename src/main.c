#include <raylib.h>
#include <raymath.h>
#include <time.h>
#include "std.h"
#include "alloc.h"
#include "assets.h"
#include "constants.h"
#include "game.h"
#include "ui.h"
#include "units.h"
#include "level.h"
#include "particle.h"
#include "cake.h"
#include "audio.h"
#include "tutorial.h"
#include "unit_pool.h"
#include "input.h"
#include "manual.h"

#if defined(ANDROID)
const int WINDOW_WIDTH = 0;
const int WINDOW_HEIGHT = 0;
#else
const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 1200;
#endif

const Color black = (Color) {18, 18, 18, 255};
const Color bg_color = DARKBLUE;
const Color tx_color = RAYWHITE;

ExecutionMode level_select (Assets * assets, GameState * game) {
    game->players = listPlayerDataInit(PLAYERS_MAX, perm_allocator());
    clear_memory(game->players.items, sizeof(PlayerData) * game->players.cap);
    game->players.items[0].type = PLAYER_NEUTRAL;
    game->players.items[1].type = PLAYER_LOCAL;
    for (usize i = 2; i < game->players.cap; i++) {
        game->players.items[i].type = PLAYER_AI;
    }
    int selected_map = -1;
    int action = 0;
    const Theme * theme = &game->settings->theme;

    while (action == 0) {
        if (WindowShouldClose()) {
            action = 3;
            break;
        }
        BeginDrawing();
        draw_title(&game->settings->theme);

        UpdateMusicStream(assets->main_theme);

        Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
        screen = cake_margin_all(screen, 20);
        Rectangle map_list = cake_cut_vertical(&screen, 0.3f, 10);
        Rectangle map_preview = cake_cut_horizontal(&screen, 0.5f, 10);

        Map * selected = NULL;
        if (selected_map >= 0) {
            selected = &assets->maps.items[selected_map];
        }
        render_simple_map_preview(map_preview, selected, theme);

        usize max_len = map_list.height / theme->font_size;
        if (max_len > assets->maps.len) {
            int sel = render_map_list(map_list, &assets->maps, 0, assets->maps.len, theme);
            if (sel >= 0) {
                play_sound(assets, SOUND_UI_CLICK);
                selected_map = sel;
            }
        }
        else {
            // TODO make a scroll bar
        }


        Rectangle buttons = cake_cut_horizontal(&screen, theme->font_size * -1.5f, 20);
        render_player_select(screen, game, selected_map);

        Rectangle cancel = cake_cut_vertical(&buttons, 0.5f, 0);
        float width = MeasureText("Cancel", theme->font_size) + theme->frame_thickness * 2 + theme->margin * 2;
        float height = theme->font_size * theme->frame_thickness * 2 + theme->margin * 2;
        cancel = cake_carve_to(cancel, width, height);
        width = MeasureText("Start", theme->font_size) + theme->frame_thickness * 2 + theme->margin * 2;
        Rectangle accept = cake_carve_to(buttons, width, height);

        Vector2 cursor = GetMousePosition();
        draw_button(cancel, "Cancel", cursor, UI_LAYOUT_CENTER, theme);
        draw_button(accept, "Start", cursor, UI_LAYOUT_CENTER, theme);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, cancel)) {
                play_sound(assets, SOUND_UI_CLICK);
                action = 1;
            }
            if (selected_map >= 0 && CheckCollisionPointRec(cursor, accept)) {
                play_sound(assets, SOUND_UI_CLICK);
                action = 2;
            }
        }

        EndDrawing();
        temp_reset();
    }


    ExecutionMode next;
    switch (action) {
        case 1: next = EXE_MODE_MAIN_MENU; break;
        case 2: {
            if (game_state_prepare (game, &assets->maps.items[selected_map])) {
                next = EXE_MODE_SINGLE_PLAYER_MAP_SELECT;
                break;
            }
            next = EXE_MODE_IN_GAME;
        } break;
        case 3: next = EXE_MODE_EXIT; break;
        default: next = EXE_MODE_SINGLE_PLAYER_MAP_SELECT; break;
    }

    if (next != EXE_MODE_IN_GAME) {
        listPlayerDataDeinit(&game->players);
    }
    return next;
}
ExecutionMode play_mode (GameState * game) {
    usize player;
    if (get_local_player_index(game, &player)) {
        player = 1;
    }

    #if defined(ANDROID)
    Color player_color = get_player_color(player);
    #endif

    Music theme = game->resources->faction_themes[game->players.items[player].faction];
    PlayMusicStream(theme);
    InfoBarAction play_state = INFO_BAR_ACTION_NONE;
    usize winner = 0;
    while (play_state != INFO_BAR_ACTION_QUIT) {
        if (WindowShouldClose()) {
            break;
        }
        BeginDrawing();
        draw_title(&game->settings->theme);

        UpdateMusicStream(theme);
        if (play_state == INFO_BAR_ACTION_NONE) {
            game_tick(game);
            winner = game_winner(game);
        }

        BeginMode2D(game->camera);
            if (play_state == INFO_BAR_ACTION_NONE) {
                render_interaction_hints(game);
            }
            render_map_mesh(game);
            render_units(game);
            particles_render(game->particles_in_use.items, game->particles_in_use.len);
        EndMode2D();

        #if defined(ANDROID)
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
        #else
        if (play_state == INFO_BAR_ACTION_NONE && game->current_input == INPUT_OPEN_BUILDING) {
            if (game->selected_building->type == BUILDING_EMPTY) {
                render_empty_building_dialog(game);
            }
            else {
                render_upgrade_building_dialog(game);
            }
        }
        #endif

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
    game_state_deinit(game);
    StopMusicStream(theme);
    return EXE_MODE_MAIN_MENU;
}
ExecutionMode main_menu (Assets * assets, Settings * settings) {
    ExecutionMode mode = EXE_MODE_MAIN_MENU;

    int options = 0;

    while (mode == EXE_MODE_MAIN_MENU) {
        if (WindowShouldClose()) {
            mode = EXE_MODE_EXIT;
            break;
        }
        UpdateMusicStream(assets->main_theme);
        BeginDrawing();
        draw_title(&settings->theme);

        if (options) {
            Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
            if (render_settings(screen, settings, assets)) {
                options = 0;
                save_settings(settings);
            }
        }
        else {
            MainMenuLayout layout = main_menu_layout(&settings->theme);
            Vector2 cursor = GetMousePosition();


            label (layout.title, "Line Lancer", settings->theme.font_size * 2, UI_LAYOUT_CENTER, &settings->theme);
            draw_background(layout.background, &settings->theme);
            draw_button(layout.new_game, "New Game", cursor, UI_LAYOUT_CENTER, &settings->theme);
            draw_button(layout.tutorial, "Tutorial", cursor, UI_LAYOUT_CENTER, &settings->theme);
            draw_button(layout.manual, "Manual", cursor, UI_LAYOUT_CENTER, &settings->theme);
            draw_button(layout.options, "Settings", cursor, UI_LAYOUT_CENTER, &settings->theme);
            draw_button(layout.quit, "Exit", cursor, UI_LAYOUT_CENTER, &settings->theme);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(cursor, layout.new_game)) {
                    play_sound(assets, SOUND_UI_CLICK);
                    mode = EXE_MODE_SINGLE_PLAYER_MAP_SELECT;
                }
                if (CheckCollisionPointRec(cursor, layout.tutorial)) {
                    play_sound(assets, SOUND_UI_CLICK);
                    mode = EXE_MODE_TUTORIAL;
                }
                if (CheckCollisionPointRec(cursor, layout.manual)) {
                    play_sound(assets, SOUND_UI_CLICK);
                    mode = EXE_MODE_MANUAL;
                }
                if (CheckCollisionPointRec(cursor, layout.options)) {
                    play_sound(assets, SOUND_UI_CLICK);
                    options = 1;
                }
                if (CheckCollisionPointRec(cursor, layout.quit)) {
                    play_sound(assets, SOUND_UI_CLICK);
                    mode = EXE_MODE_EXIT;
                }
            }
        }

        EndDrawing();
        temp_reset();
    }

    return mode;
}

int main(void) {
    int result = 0;
    SetTraceLogLevel(LOG_INFO);

    SetRandomSeed(time(0));

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Line Lancer");
    InitAudioDevice();
    SetWindowMinSize(1200, 720);

    #if !defined(DEBUG)
    SetTraceLogLevel(LOG_ERROR);
    #endif

    {
        BeginDrawing();
        ClearBackground(black);
        Rectangle rect = (Rectangle) { 0, 0, GetScreenWidth(), GetScreenHeight() };
        rect = cake_diet_by(rect, 0.5f);
        rect = cake_squish_by(rect, 0.5f);
        DrawRectangleRec(rect, bg_color);
        char * loading = "Loading...";
        float len = MeasureText(loading, 30);
        rect = cake_carve_to(rect, len, 30);
        DrawText(loading, rect.x, rect.y, 30, tx_color);
        EndDrawing();
    }

    Assets game_assets = {0};
    GameState game_state = {0};
    Settings game_settings = {0};

    game_assets.maps = listMapInit(6, perm_allocator());
    ExecutionMode mode = EXE_MODE_MAIN_MENU;

    if (load_levels(&game_assets.maps)) {
        TraceLog(LOG_FATAL, "Failed to load levels");
        goto close;
    }
    if (load_graphics(&game_assets)) {
        TraceLog(LOG_FATAL, "Failed to load graphics");
        goto close;
    }
    if (load_music(&game_assets)) {
        TraceLog(LOG_FATAL, "Failed to load music");
        goto close;
    }
    if (load_sound_effects(&game_assets)) {
        TraceLog(LOG_FATAL, "Failed to load sound effects");
        goto close;
    }
    if (load_settings(&game_settings)) {
        TraceLog(LOG_FATAL, "Failed to load game settings");
        goto close;
    }
    if (load_animations(&game_assets)) {
        TraceLog(LOG_FATAL, "Failed to load animations");
        goto close;
    }
    game_settings.theme.assets = &game_assets.ui;

    apply_sound_settings(&game_assets, &game_settings);
    #if !defined(ANDROID)
    if (game_settings.fullscreen) {
        ClearWindowState(FLAG_WINDOW_RESIZABLE);
        ToggleBorderlessWindowed();
    }
    else {
        SetWindowState(FLAG_WINDOW_RESIZABLE);
    }
    #endif
    unit_pool_init();

    SetTargetFPS(FPS);
    PlayMusicStream(game_assets.main_theme);
    while (mode != EXE_MODE_EXIT) {
        if (WindowShouldClose()) {
            break;
        }

        game_state.resources = &game_assets;
        game_state.settings = &game_settings;
        switch (mode) {
            case EXE_MODE_IN_GAME: {
                StopMusicStream(game_assets.main_theme);
                mode = play_mode(&game_state);
                if (EXE_MODE_MAIN_MENU == mode || EXE_MODE_SINGLE_PLAYER_MAP_SELECT == mode) {
                    PlayMusicStream(game_assets.main_theme);
                }
            } break;
            case EXE_MODE_MAIN_MENU: {
                mode = main_menu(&game_assets, &game_settings);
            } break;
            case EXE_MODE_MANUAL: {
                mode = manual_mode(&game_assets, &game_settings.theme);
            } break;
            case EXE_MODE_SINGLE_PLAYER_MAP_SELECT: {
                mode = level_select(&game_assets, &game_state);
            } break;
            case EXE_MODE_TUTORIAL: {
                mode = tutorial_mode(&game_assets, &game_state);
            } break;
            case EXE_MODE_EXIT: {};
        }

        temp_reset();
    }

    close:
    CloseAudioDevice();
    save_settings(&game_settings);
    unit_pool_deinit();
    assets_deinit(&game_assets);

    CloseWindow();
    return result;
}
