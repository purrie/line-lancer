#include <stdio.h>
#include <raylib.h>
#include <raymath.h>
#include "alloc.h"
#include "assets.h"
#include "constants.h"
#include "game.h"
#include "level.h"
#include "input.h"
#include "ui.h"
#include "units.h"

const int WINDOW_WIDTH = 1400;
const int WINDOW_HEIGHT = 1200;

int main(void) {
    SetTraceLogLevel(LOG_INFO);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello!");

    Map map = {0};
    if (load_level("./assets/maps/hash.json", &map)) {
        TraceLog(LOG_ERROR, "Failed to load map");
        goto error;
    }

    SetTargetFPS(FPS);

    Camera2D cam = setup_camera(&map);
    set_cursor_to_camera_scale(&cam);

    GameState state = create_game_state(&map);

    while (!WindowShouldClose()) {
        state.turn ++;
        update_input_state(&map, &state);
        update_resources(&state);
        simulate_units(&state);

        BeginDrawing();
        BeginMode2D(cam);

        ClearBackground(BLACK);

        render_map_mesh(&map);
        render_units(&state);
        render_ui(&state);

        EndMode2D();
        EndDrawing();
        temp_free();
    }

    clear_unit_list(&state.units);
    listUnitDeinit(&state.units);

    destroy_game_state(state);
    CloseWindow();
    return 0;

    error:
    level_unload(&map);
    CloseWindow();
    return 1;
}
