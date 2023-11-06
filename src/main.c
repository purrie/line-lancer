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

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 1200;

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

    while (action == 0) {
        if (WindowShouldClose()) {
            action = 3;
            break;
        }
        BeginDrawing();
        ClearBackground(black);

        UpdateMusicStream(assets->main_theme);

        Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
        screen = cake_margin_all(screen, 20);
        Rectangle map_list = cake_cut_vertical(&screen, 0.3f, 10);
        Rectangle map_preview = cake_cut_horizontal(&screen, 0.5f, 10);

        Map * selected = NULL;
        if (selected_map >= 0) {
            selected = &assets->maps.items[selected_map];
        }
        render_simple_map_preview(map_preview, selected, 5.0f, 2.0f);

        usize max_len = map_list.height / UI_FONT_SIZE_BUTTON;
        if (max_len > assets->maps.len) {
            int sel = render_map_list(map_list, &assets->maps, 0, assets->maps.len);
            if (sel >= 0) {
                selected_map = sel;
            }
        }
        else {
            // TODO make a scroll bar
        }


        Rectangle buttons = cake_cut_horizontal(&screen, UI_FONT_SIZE_BUTTON * -1.5f, 20);
        render_player_select(screen, game, selected_map);

        Rectangle cancel = cake_cut_vertical(&buttons, 0.5f, 0);
        float width = MeasureText("Cancel", UI_FONT_SIZE_BUTTON) + 10.0f;
        float height = UI_FONT_SIZE_BUTTON * 1.5f;
        cancel = cake_carve_to(cancel, width, height);
        width = MeasureText("Start", UI_FONT_SIZE_BUTTON) + 10.0f;
        Rectangle accept = cake_carve_to(buttons, width, height);

        Vector2 cursor = GetMousePosition();
        draw_button(cancel, "Cancel", cursor, UI_LAYOUT_CENTER, DARKBLUE, BLUE, RAYWHITE);
        draw_button(accept, "Start", cursor, UI_LAYOUT_CENTER, DARKBLUE, BLUE, RAYWHITE);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, cancel)) {
                action = 1;
            }
            if (selected_map >= 0 && CheckCollisionPointRec(cursor, accept)) {
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
    // TODO make way of exiting to main menu through UI
    usize player;
    if (get_local_player_index(game, &player)) {
        player = 1;
    }
    Music theme = game->resources->faction_themes[game->players.items[player].faction];
    PlayMusicStream(theme);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(black);
        UpdateMusicStream(theme);
        game_tick(game);

        BeginMode2D(game->camera);
            render_interaction_hints(game);
            render_map_mesh(game);
            render_units(game);
            particles_render(game->particles_in_use.items, game->particles_in_use.len);
        EndMode2D();

        render_ingame_ui(game);
        EndDrawing();
        temp_reset();
    }
    game_state_deinit(game);
    StopMusicStream(theme);
    return EXE_MODE_MAIN_MENU;
}
ExecutionMode main_menu (Assets * assets) {
    Color bg = DARKBLUE;
    Color hover = BLUE;
    Color frame = BLACK;

    ExecutionMode mode = EXE_MODE_MAIN_MENU;

    while (mode == EXE_MODE_MAIN_MENU) {
        if (WindowShouldClose()) {
            mode = EXE_MODE_EXIT;
            break;
        }
        UpdateMusicStream(assets->main_theme);

        MainMenuLayout layout = main_menu_layout();
        Vector2 cursor = GetMousePosition();

        BeginDrawing();
        ClearBackground(black);

        draw_button(layout.new_game, "New Game", cursor, UI_LAYOUT_CENTER, bg, hover, frame);
        draw_button(layout.quit, "Exit", cursor, UI_LAYOUT_CENTER, bg, hover, frame);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, layout.new_game)) {
                mode = EXE_MODE_SINGLE_PLAYER_MAP_SELECT;
            }

            if (CheckCollisionPointRec(cursor, layout.quit)) {
                mode = EXE_MODE_EXIT;
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

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello!");

    InitAudioDevice();

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

    SetTargetFPS(FPS);
    PlayMusicStream(game_assets.main_theme);
    while (mode != EXE_MODE_EXIT) {
        if (WindowShouldClose()) {
            break;
        }

        game_state.resources = &game_assets;
        switch (mode) {
            case EXE_MODE_IN_GAME: {
                StopMusicStream(game_assets.main_theme);
                mode = play_mode(&game_state);
                if (EXE_MODE_MAIN_MENU == mode || EXE_MODE_SINGLE_PLAYER_MAP_SELECT == mode) {
                    PlayMusicStream(game_assets.main_theme);
                }
            } break;
            case EXE_MODE_MAIN_MENU: {
                mode = main_menu(&game_assets);
            } break;
            case EXE_MODE_SINGLE_PLAYER_MAP_SELECT: {
                mode = level_select(&game_assets, &game_state);
            } break;
            case EXE_MODE_EXIT: {
            };
        }

        temp_reset();
    }
    close:

    assets_deinit(&game_assets);

    CloseAudioDevice();
    CloseWindow();
    return result;
}
