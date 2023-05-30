#include <stdio.h>
#include <raylib.h>
#include "level.h"
#include "assets.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main(void) {
    OptionalMap m = load_level("./assets/maps/tiled.json");
    if (m.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to load map");
        return 1;
    }
    if (m.value.regions.len != 2) {
        TraceLog(LOG_ERROR, "Expected 2 regions");
        return 1;
    }

    map_resize(&m.value, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 10);
    map_transl(&m.value, 5, 5);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello!");

    Vector2 a = { .x = 1.0f, .y = 1.0f };
    Vector2 b = { .x = 100.0f, .y = 100.0f };

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(RAYWHITE);
        /* DrawText("Hello Ray!", 100, 100, 20, BLACK); */

        for (usize i = 0; i < m.value.regions.len; i++) {
            ListLine * lines = &m.value.regions.items[i].area.lines;
            for (usize l = 0; l < lines->len; l++) {
                DrawLineV(lines->items[l].a, lines->items[l].b, BLUE);
            }
            ListBuilding * buildings = &m.value.regions.items[i].buildings;
            for (usize b = 0; b < buildings->len; b++) {
                DrawCircleV(buildings->items[b].position, 20.0f, BLUE);
            }
            Vector2 castle = m.value.regions.items[i].castle.position;
            /* TraceLog(LOG_INFO, "Castle pos = [%.2f, %.2f]", castle.x, castle.y); */
            DrawCircleV(castle, 30.0f, RED);
        }
        for (usize i = 0; i < m.value.paths.len; i++) {
            ListLine * lines = &m.value.paths.items[i].lines;
            for (usize l = 0; l < lines->len; l++) {
                DrawLineV(lines->items[l].a, lines->items[l].b, ORANGE);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    if (m.has_value) {
        map_deinit(&m.value);
    }
    return 0;
}
