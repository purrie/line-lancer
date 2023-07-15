#include <raylib.h>
#include <raymath.h>
#include "std.h"
#include "alloc.h"
#include "assets.h"
#include "constants.h"
#include "game.h"
#include "ui.h"
#include "units.h"
#include "level.h"
#include "cake.h"

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 1200;

const Color black = (Color) {18, 18, 18, 255};

Result load_levels (ListMap * maps) {
    FilePathList list;
    list = LoadDirectoryFiles("./assets/maps");

    usize i = 0;

    while (i < list.count) {
        if (WindowShouldClose()) {
            TraceLog(LOG_INFO, "User requested aplication exit");
            goto abort;
        }
        BeginDrawing();
        ClearBackground(black);

        char * label = "Loading Map: ";
        char * map_name = map_name_from_path(list.paths[i], &temp_alloc);
        if (map_name == NULL) {
            goto abort;
        }

        usize label_len = string_length(label);
        usize len = string_length(map_name);
        char * loading_text = temp_alloc(label_len + len + 1);
        copy_memory(loading_text, label, label_len);
        copy_memory(&loading_text[label_len], map_name, len);
        len += label_len;
        loading_text[len] = '\0';
        len = MeasureText(loading_text, 30);

        Rectangle rect = cake_rect(GetScreenWidth(), GetScreenHeight());
        rect = cake_carve_to(rect, len + 30, 50);
        DrawRectangleRec(rect, DARKBLUE);
        rect = cake_carve_to(rect, len, 30);
        DrawText(loading_text, rect.x, rect.y, 30, RAYWHITE);

        if(listMapAppend(maps, (Map){0})) {
            TraceLog(LOG_ERROR, "Failed to append map: #%zu: %s", i, list.paths[i]);
            goto abort;
        }
        if (load_level(&maps->items[i], list.paths[i])) {
            TraceLog(LOG_ERROR, "Failed to load map %s", list.paths[i]);
            maps->len -= 1;
            continue;
        }
        TraceLog(LOG_INFO, "Loaded file: %s", list.paths[i]);

        EndDrawing();
        temp_free();
        i ++;
    }

    UnloadDirectoryFiles(list);
    return SUCCESS;

    abort:
    TraceLog(LOG_FATAL, "Map loading failed at map #%zu %s", list.paths[i]);
    UnloadDirectoryFiles(list);
    return FAILURE;
}
ExecutionMode level_select (GameAssets * assets, GameState * game) {
    int selected_map = -1;

    int action = 0;

    while (action == 0) {
        if (WindowShouldClose()) {
            action = 3;
            break;
        }
        BeginDrawing();
        ClearBackground(black);

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
            // TODO turn this into a generic selector widget
            int sel = render_map_list(map_list, &assets->maps, 0, assets->maps.len);
            if (sel >= 0) {
                selected_map = sel;
            }
        }
        else {
            // TODO make a scroll bar
        }

        screen = cake_squish_to(screen, UI_FONT_SIZE_BUTTON * 1.5f);

        Rectangle cancel = cake_cut_vertical(&screen, 0.5f, 0);
        float width = MeasureText("Cancel", UI_FONT_SIZE_BUTTON) + 10.0f;
        float height = UI_FONT_SIZE_BUTTON * 1.5f;
        cancel = cake_carve_to(cancel, width, height);
        width = MeasureText("Start", UI_FONT_SIZE_BUTTON) + 10.0f;
        Rectangle accept = cake_carve_to(screen, width, height);

        Vector2 cursor = GetMousePosition();
        draw_button(cancel, "Cancel", cursor, 5.0f, DARKBLUE, BLUE, RAYWHITE);
        draw_button(accept, "Start", cursor, 5.0f, DARKBLUE, BLUE, RAYWHITE);

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, cancel)) {
                action = 1;
            }
            if (selected_map >= 0 && CheckCollisionPointRec(cursor, accept)) {
                action = 2;
            }
        }

        EndDrawing();
    }


    switch (action) {
        case 1: return EXE_MODE_MAIN_MENU;
        case 2: {
            if (game_state_prepare (game, &assets->maps.items[selected_map])) {
                return EXE_MODE_SINGLE_PLAYER_MAP_SELECT;
            }
            return EXE_MODE_IN_GAME;
        }
        case 3: return EXE_MODE_EXIT;
        default: return EXE_MODE_SINGLE_PLAYER_MAP_SELECT;
    }
}
ExecutionMode play_mode (GameState * game) {
    // TODO make way of exiting to main menu through UI
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(black);
        game_tick(game);

        BeginMode2D(game->camera);
            render_map_mesh(&game->map);
            render_units(game);
        EndMode2D();

        render_ingame_ui(game);
        EndDrawing();
    }
    game_state_deinit(game);
    return EXE_MODE_MAIN_MENU;
}
ExecutionMode main_menu () {
    Color bg = DARKBLUE;
    Color hover = BLUE;
    Color frame = BLACK;

    ExecutionMode mode = EXE_MODE_MAIN_MENU;

    while (mode == EXE_MODE_MAIN_MENU) {
        if (WindowShouldClose()) {
            mode = EXE_MODE_EXIT;
            break;
        }
        MainMenuLayout layout = main_menu_layout();
        Vector2 cursor = GetMousePosition();

        BeginDrawing();
        ClearBackground(black);

        draw_button(layout.new_game, "New Game", cursor, 5.0f, bg, hover, frame);
        draw_button(layout.quit, "Exit", cursor, 5.0f, bg, hover, frame);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, layout.new_game)) {
                mode = EXE_MODE_SINGLE_PLAYER_MAP_SELECT;
            }

            if (CheckCollisionPointRec(cursor, layout.quit)) {
                mode = EXE_MODE_EXIT;
            }
        }

        EndDrawing();
        temp_free();
    }


    return mode;
}

int main(void) {
    int result = 0;
    SetTraceLogLevel(LOG_INFO);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello!");

    GameAssets game_assets = {0};
    GameState game_state = {0};

    game_assets.maps = listMapInit(6, &MemAlloc, &MemFree);
    ExecutionMode mode = EXE_MODE_MAIN_MENU;

    if (load_levels(&game_assets.maps)) {
        TraceLog(LOG_FATAL, "Failed to load levels");
        goto close;
    }
    if (game_assets.maps.len == 0) {
        TraceLog(LOG_FATAL, "Failed to load any map");
        goto close;
    }

    // TODO load assets

    SetTargetFPS(FPS);
    while (mode != EXE_MODE_EXIT) {
        if (WindowShouldClose()) {
            break;
        }

        switch (mode) {
            case EXE_MODE_IN_GAME: {
                mode = play_mode(&game_state);
            } break;
            case EXE_MODE_MAIN_MENU: {
                mode = main_menu();
            } break;
            case EXE_MODE_SINGLE_PLAYER_MAP_SELECT: {
                mode = level_select(&game_assets, &game_state);
            } break;
            case EXE_MODE_EXIT: {
            };
        }

        temp_free();
    }
    close:

    assets_deinit(&game_assets);

    CloseWindow();
    return result;
}
