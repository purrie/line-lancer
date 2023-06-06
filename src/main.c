#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include "level.h"
#include "assets.h"
#include "input.h"
#include "ui.h"

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 1200;

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello!");

    OptionalMap m = load_level("./assets/maps/tiled.json");
    if (m.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to load map");
        goto end;
    }

    Camera2D cam = setup_camera(&m.value);
    set_cursor_to_camera_scale(&cam);

    GameState state = {0};

    while (!WindowShouldClose()) {
        update_input_state(&m.value, &state);

        BeginDrawing();
        BeginMode2D(cam);

        ClearBackground(RAYWHITE);

        render_map_mesh(&m.value);
        render_ui(&state);

        EndMode2D();
        EndDrawing();
    }

    end:
    if (m.has_value) {
        level_unload(&m.value);
    }
    CloseWindow();
    return 0;
}
