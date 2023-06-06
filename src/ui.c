#include "ui.h"

void draw_button(
    char      * text,
    Rectangle   area,
    Vector2     cursor,
    float       padding,
    Color       bg,
    Color       hover,
    Color       frame
) {
    if (CheckCollisionPointRec(cursor, area)) {
        DrawRectangleRec(area, hover);
        DrawRectangleLinesEx(area, 2.0f, frame);
    }
    else {
        DrawRectangleRec(area, bg);
        DrawRectangleLinesEx(area, 1.0f, frame);
    }

    DrawText(text, area.x + padding, area.y + padding, 10, frame);
}

float ui_button_size() {
    return 50.0f;
}
float ui_margin() {
    return 5.0f;
}
float ui_spacing() {
    return 2.0f;
}

Rectangle ui_empty_building_dialog_rect(Vector2 building_position) {
    int button_per_row = BUILDING_TYPE_COUNT / 2 + BUILDING_TYPE_COUNT % 2;
    float button_size = ui_button_size();
    float margin  = ui_margin();
    float spacing = ui_spacing();
    float width   = button_size * button_per_row + margin * 2.0f + spacing * (button_per_row - 1);
    float height  = button_size * 2.0f + margin * 2.0f + spacing;

    Rectangle dialog_rect = {
        .x = building_position.x - width * 0.5f,
        .y = building_position.y - height * 0.5f,
        .width = width,
        .height = height,
    };
    return dialog_rect;
}

Rectangle ui_empty_building_starting_button_rect(Rectangle dialog) {
    float margin = ui_margin();
    float button_size = ui_button_size();

    Rectangle button_rect = {
        .x = dialog.x + margin,
        .y = dialog.y + margin,
        .width = button_size,
        .height = button_size,
    };
    return button_rect;
}

void render_empty_building_dialog(GameState *const state) {

    Vector2 cursor = GetMousePosition();
    Rectangle dialog_rect = ui_empty_building_dialog_rect(state->selected_building->position);
    float margin = ui_margin();
    float button_size = ui_button_size();
    float spacing = ui_spacing();

    {
        Color dialog_bg;

        if (CheckCollisionPointRec(cursor, dialog_rect)) {
            dialog_bg = BLUE;
        } else {
            dialog_bg = DARKBLUE;
        }

        DrawRectangleRec(dialog_rect, dialog_bg);
    }

    {
        Rectangle button_rect = ui_empty_building_starting_button_rect(dialog_rect);
        Color color_bg = DARKBLUE;
        Color color_hover = LIGHTGRAY;
        Color color_frame = BLACK;

        draw_button("Fighter", button_rect, cursor, margin, color_bg, color_hover, color_frame);
        button_rect.x += button_size + spacing;
        draw_button("Archer", button_rect, cursor, margin, color_bg, color_hover, color_frame);
        button_rect.x += button_size + spacing;
        draw_button("Support", button_rect, cursor, margin, color_bg, color_hover, color_frame);

        button_rect.x = dialog_rect.x + margin;
        button_rect.y += button_size + spacing;
        draw_button("Special", button_rect, cursor, margin, color_bg, color_hover, color_frame);
        button_rect.x += button_size + spacing;
        draw_button("Cash", button_rect, cursor, margin, color_bg, color_hover, color_frame);
    }
}

BuildingType ui_get_empty_building_dialog_click(Vector2 building_position) {
    Rectangle dialog = ui_empty_building_dialog_rect(building_position);
    Rectangle button = ui_empty_building_starting_button_rect(dialog);
    Vector2 cursor = GetMousePosition();
    float stride = ui_button_size() + ui_spacing();

    if (CheckCollisionPointRec(cursor, button)) {
        return BUILDING_FIGHTER;
    }
    button.x += stride;
    if (CheckCollisionPointRec(cursor, button)) {
        return BUILDING_ARCHER;
    }
    button.x += stride;
    if (CheckCollisionPointRec(cursor, button)) {
        return BUILDING_SUPPORT;
    }
    button = ui_empty_building_starting_button_rect(dialog);
    button.y += stride;
    if (CheckCollisionPointRec(cursor, button)) {
        return BUILDING_SUPPORT;
    }
    button.x += stride;
    if (CheckCollisionPointRec(cursor, button)) {
        return BUILDING_RESOURCE;
    }

    return BUILDING_EMPTY;
}

void render_upgrade_building_dialog(GameState *const state) {

}

void render_ui(GameState *const state) {
    if (state->current_input == INPUT_OPEN_BUILDING) {
        if (state->selected_building->type == BUILDING_EMPTY) {
            render_empty_building_dialog(state);
        }
        else {
            render_upgrade_building_dialog(state);
        }
    }
}
