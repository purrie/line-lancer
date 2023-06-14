#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include "level.h"
#include "assets.h"
#include "input.h"
#include "ui.h"
#include "units.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main(void) {
    SetTraceLogLevel(LOG_INFO);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello!");

    Map map = {0};
    if (load_level("./assets/maps/tiled.json", &map)) {
        TraceLog(LOG_ERROR, "Failed to load map");
        goto end;
    }

    Camera2D cam = setup_camera(&map);
    set_cursor_to_camera_scale(&cam);

    GameState state = {0};
    state.current_map = &map;
    state.units = listUnitInit(120, &MemAlloc, &MemFree);

    while (!WindowShouldClose()) {
        update_input_state(&map, &state);
        simulate_units(&state);

        BeginDrawing();
        BeginMode2D(cam);

        ClearBackground(RAYWHITE);

        render_map_mesh(&map);
        render_units(&state.units);
        render_ui(&state);

        EndMode2D();
        EndDrawing();
    }

    clear_unit_list(&state.units);
    listUnitDeinit(&state.units);

    end:
    level_unload(&map);
    CloseWindow();
    return 0;
}
