#include "ui.h"
#include "std.h"
#include "alloc.h"
#include "constants.h"
#include "level.h"
#define CAKE_LAYOUT_IMPLEMENTATION
#include "cake.h"
#include <raymath.h>

void draw_button (
    Rectangle   area,
    char      * text,
    Vector2     cursor,
    UiLayout    label_layout,
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
    Rectangle text_area;
    switch (label_layout) {
        case UI_LAYOUT_LEFT: {
            text_area = cake_carve_to(area, area.width, UI_FONT_SIZE_BUTTON);
            int space = MeasureText(" ", UI_FONT_SIZE_BUTTON);
            text_area.x += space;
        } break;
        case UI_LAYOUT_CENTER: {
            int label_width = MeasureText(text, UI_FONT_SIZE_BUTTON);
            text_area = cake_carve_to(area, label_width, UI_FONT_SIZE_BUTTON);
        } break;
        case UI_LAYOUT_RIGHT: {
            int label_width = MeasureText(text, UI_FONT_SIZE_BUTTON);
            text_area = cake_carve_to(area, area.width, UI_FONT_SIZE_BUTTON);
            text_area.x += area.width - label_width;
            int space = MeasureText(" ", UI_FONT_SIZE_BUTTON);
            text_area.x -= space;
        } break;
    }

    DrawText(text, text_area.x, text_area.y, UI_FONT_SIZE_BUTTON, frame);
}

float ui_margin () {
    return 5.0f;
}
float ui_spacing () {
    return 2.0f;
}

/* In Game Menu **************************************************************/
EmptyDialog empty_dialog (Vector2 position) {
    EmptyDialog result;
    float margin = ui_margin();
    float spacing = ui_spacing();

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    cake_cut_horizontal(&screen, UI_BAR_SIZE, 0);

    result.area = cake_rect(UI_DIALOG_BUILDING_WIDTH + spacing * 2.0f + margin * 2.0f, UI_DIALOG_BUILDING_HEIGHT + margin * 2.0f + spacing);
    result.area = cake_center_rect(result.area, position.x, position.y);
    cake_clamp_inside(&result.area, screen);

    Rectangle butt = cake_margin_all(result.area, margin);
    Rectangle buttons[6];
    cake_split_grid(butt, 3, 2, buttons, spacing);

    result.warrior  = buttons[0];
    result.archer   = buttons[1];
    result.support  = buttons[2];
    result.special  = buttons[3];
    result.resource = buttons[4];

    return result;
}
BuildingDialog building_dialog (Vector2 position) {
    BuildingDialog result;
    float margin  = ui_margin();
    float spacing = ui_spacing();

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    cake_cut_horizontal(&screen, UI_BAR_SIZE, 0);

    result.area = cake_rect(UI_DIALOG_UPGRADE_WIDTH + margin * 2.0f, UI_DIALOG_UPGRADE_HEIGHT + margin * 2.0f + spacing * 2.0f);
    result.area = cake_center_rect(result.area, position.x, position.y);
    cake_clamp_inside(&result.area, screen);

    Rectangle butt = cake_margin_all(result.area, margin);
    Rectangle buttons[3];
    cake_split_horizontal(butt, 3, buttons, spacing);

    result.label    = buttons[0];
    result.upgrade  = buttons[1];
    result.demolish = buttons[2];

    return result;
}

Result ui_building_action_click (const GameState * state, Vector2 cursor, BuildingAction * action) {
    Vector2 building_pos = GetWorldToScreen2D(state->selected_building->position, state->camera);
    BuildingDialog dialog = building_dialog(building_pos);
    if (CheckCollisionPointRec(cursor, dialog.demolish)) {
        *action = BUILDING_ACTION_DEMOLISH;
    }
    else if (CheckCollisionPointRec(cursor, dialog.upgrade)) {
        *action = BUILDING_ACTION_UPGRADE;
    }
    else {
        *action = BUILDING_ACTION_NONE;
        return FAILURE;
    }
    return SUCCESS;
}
Result ui_building_buy_click (const GameState * state, Vector2 cursor, BuildingType * result) {
    if (state->selected_building->type != BUILDING_EMPTY) {
        return FAILURE;
    }

    Vector2 ui_box = GetWorldToScreen2D(state->selected_building->position, state->camera);
    EmptyDialog dialog = empty_dialog(ui_box);
    if (! CheckCollisionPointRec(cursor, dialog.area)) {
        return FAILURE;
    }

    if (CheckCollisionPointRec(cursor, dialog.warrior)) {
        *result = BUILDING_FIGHTER;
    }
    else if (CheckCollisionPointRec(cursor, dialog.archer)) {
        *result = BUILDING_ARCHER;
    }
    else if (CheckCollisionPointRec(cursor, dialog.support)) {
        *result = BUILDING_SUPPORT;
    }
    else if (CheckCollisionPointRec(cursor, dialog.special)) {
        *result = BUILDING_SPECIAL;
    }
    else if (CheckCollisionPointRec(cursor, dialog.resource)) {
        *result = BUILDING_RESOURCE;
    }
    else {
        return FAILURE;
    }

    return SUCCESS;
}

void render_empty_building_dialog (const GameState * state) {
    Vector2 cursor = GetMousePosition();
    Vector2 ui_box = GetWorldToScreen2D(state->selected_building->position, state->camera);
    EmptyDialog dialog = empty_dialog(ui_box);

    {
        Color dialog_bg;

        if (CheckCollisionPointRec(cursor, dialog.area)) {
            dialog_bg = BLUE;
        } else {
            dialog_bg = DARKBLUE;
        }

        DrawRectangleRec(dialog.area, dialog_bg);
    }

    {
        Color color_bg = DARKBLUE;
        Color color_hover = LIGHTGRAY;
        Color color_frame = BLACK;

        draw_button(dialog.warrior, "Fighter", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);
        draw_button(dialog.archer, "Archer", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);
        draw_button(dialog.support, "Support", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);

        draw_button(dialog.special, "Special", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);
        draw_button(dialog.resource, "Cash", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);
    }
}
void render_upgrade_building_dialog (const GameState * state) {
    Vector2 cursor = GetMousePosition();
    Vector2 building_pos = GetWorldToScreen2D(state->selected_building->position, state->camera);
    BuildingDialog dialog = building_dialog(building_pos);

    Color dialog_bg;
    if (CheckCollisionPointRec(cursor, dialog.area)) {
        dialog_bg = BLUE;
    } else {
        dialog_bg = DARKBLUE;
    }

    DrawRectangleRec(dialog.area, dialog_bg);
    Rectangle level = cake_cut_horizontal(&dialog.label, 0.6f, 2.0f);
    char * text;
    switch (state->selected_building->type) {
        case BUILDING_FIGHTER: {
            text = "Fighter";
        } break;
        case BUILDING_ARCHER: {
            text = "Archer";
        } break;
        case BUILDING_SUPPORT: {
            text = "Support";
        } break;
        case BUILDING_SPECIAL: {
            text = "Special";
        } break;
        case BUILDING_RESOURCE: {
            text = "Cash";
        } break;
        case BUILDING_EMPTY: {
            DrawText("Invalid", dialog.label.x, dialog.label.y, 20, WHITE);
        } return;
    }
    char * lvl;
    switch (state->selected_building->upgrades) {
        case 0: {
            lvl = "Level 1";
        } break;
        case 1: {
            lvl = "Level 2";
        } break;
        case 2: {
            lvl = "Level 3";
        } break;
        default: {
            lvl = "Level N/A";
        } break;
    }
    DrawText(text, dialog.label.x, dialog.label.y, 20, WHITE);
    DrawText(lvl, level.x, level.y, 16, WHITE);

    Color color_bg = DARKBLUE;
    Color color_hover = LIGHTGRAY;
    Color color_frame = BLACK;
    draw_button(dialog.upgrade, "Upgrade", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);
    draw_button(dialog.demolish, "Demolish", cursor, UI_LAYOUT_CENTER, color_bg, color_hover, color_frame);
}
void render_resource_bar (const GameState * state) {
    usize player_index;
    if (get_local_player_index(state, &player_index)) {
        player_index = 1;
    }
    PlayerData * player = &state->players.items[player_index];

    float income = get_expected_income(&state->map, player_index);
    float upkeep = get_expected_maintenance_cost(&state->map, player_index);

    Rectangle bar = cake_rect(GetScreenWidth(), UI_BAR_SIZE);
    Rectangle mbar = cake_margin_all(bar, UI_BAR_MARGIN);

    char * gold_label   = "Gold: ";
    char * income_label = "Income: ";
    char * upkeep_label = "Upkeep cost: ";
    char * per_second = "/s ";

    char * gold_value_label = convert_int_to_ascii(player->resource_gold, &temp_alloc);
    if (gold_value_label == NULL) {
        gold_value_label = "lots... ";
    }
    char * income_value_label = convert_float_to_ascii(income, 1, &temp_alloc);
    if (income_value_label == NULL) {
        income_value_label = "too much...";
    }
    char * upkeep_value_label = convert_float_to_ascii(upkeep, 1, &temp_alloc);
    if (upkeep_value_label == NULL) {
        upkeep_value_label = "too much...";
    }

    int gold_label_width = MeasureText(gold_label, UI_FONT_SIZE_BAR);
    int gold_value_width = MeasureText(gold_value_label, UI_FONT_SIZE_BAR);
    int income_label_width = MeasureText(income_label, UI_FONT_SIZE_BAR);
    int income_value_width = MeasureText(income_value_label, UI_FONT_SIZE_BAR);
    int upkeep_label_width = MeasureText(upkeep_label, UI_FONT_SIZE_BAR);
    int upkeep_value_width = MeasureText(upkeep_value_label, UI_FONT_SIZE_BAR);
    int per_second_width = MeasureText(per_second, UI_FONT_SIZE_BAR);

    #define MARGIN_SIZE 50

    int gold_margin = UI_BAR_MARGIN;
    if (gold_value_width < MARGIN_SIZE)
        gold_margin += MARGIN_SIZE - gold_value_width;

    int income_margin = UI_BAR_MARGIN;
    if (income_value_width < MARGIN_SIZE)
        income_margin += MARGIN_SIZE - income_value_width;

    Rectangle rect_gold_label = cake_cut_vertical(&mbar, gold_label_width, 0);
    Rectangle rect_gold  = cake_cut_vertical(&mbar, gold_value_width, gold_margin);

    Rectangle rect_income_label = cake_cut_vertical(&mbar, income_label_width, 0);
    Rectangle rect_income = cake_cut_vertical(&mbar, income_value_width, 0);
    Rectangle rect_income_per_second = cake_cut_vertical(&mbar, per_second_width, income_margin);

    Rectangle rect_upkeep_label = cake_cut_vertical(&mbar, upkeep_label_width, 0);
    Rectangle rect_upkeep = cake_cut_vertical(&mbar, upkeep_value_width, 0);
    Rectangle rect_upkeep_per_second = mbar;

    DrawRectangleRec(bar, DARKGRAY);
    DrawText(gold_label , rect_gold_label.x, rect_gold_label.y, UI_FONT_SIZE_BAR, WHITE);
    DrawText(gold_value_label , rect_gold.x , rect_gold.y , UI_FONT_SIZE_BAR, WHITE);

    DrawText(income_label , rect_income_label.x, rect_income_label.y, UI_FONT_SIZE_BAR, WHITE);
    DrawText(income_value_label , rect_income.x , rect_income.y , UI_FONT_SIZE_BAR, WHITE);
    DrawText(per_second, rect_income_per_second.x, rect_income_per_second.y, UI_FONT_SIZE_BAR, WHITE);

    DrawText(upkeep_label , rect_upkeep_label.x, rect_upkeep_label.y, UI_FONT_SIZE_BAR, WHITE);
    DrawText(upkeep_value_label , rect_upkeep.x , rect_upkeep.y , UI_FONT_SIZE_BAR, WHITE);
    DrawText(per_second, rect_upkeep_per_second.x, rect_upkeep_per_second.y, UI_FONT_SIZE_BAR, WHITE);
}
void render_ingame_ui (const GameState * state) {
    if (state->current_input == INPUT_OPEN_BUILDING) {
        if (state->selected_building->type == BUILDING_EMPTY) {
            render_empty_building_dialog(state);
        }
        else {
            render_upgrade_building_dialog(state);
        }
    }
    render_resource_bar(state);
}

/* Main Menu *****************************************************************/
MainMenuLayout main_menu_layout () {
    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    Rectangle parts[3];
    cake_split_vertical(screen, 3, parts, 0.0f);
    cake_split_horizontal(parts[1], 3, parts, 0.0f);
    cake_split_horizontal(parts[1], 2, parts, 0.0f);

    MainMenuLayout result = {
        .new_game = cake_carve_to(parts[0], 150.0f, 50.0f),
        .quit     = cake_carve_to(parts[1], 150.0f, 50.0f),
    };
    return result;
}
void render_simple_map_preview (Rectangle area, Map * map, float region_size, float path_thickness) {
    DrawRectangleRec(area, BLACK);
    DrawRectangleLinesEx(area, UI_BORDER_SIZE, DARKGRAY);

    if (map == NULL) {
        DrawLine(area.x, area.y, area.x + area.width, area.y + area.height, RAYWHITE);
        DrawLine(area.x + area.width, area.y, area.x, area.y + area.height, RAYWHITE);
        return;
    }

    float scale_w = area.width / (float)(map->width);
    float scale_h = area.height / (float)(map->height);

    for (usize p = 0; p < map->paths.len; p++) {
        Path * path = &map->paths.items[p];
        Region * a = map_get_region_at(map, path->lines.items[0].a);
        Region * b = map_get_region_at(map, path->lines.items[path->lines.len - 1].b);
        if (a == NULL || b == NULL) {
            TraceLog(LOG_ERROR, "Failed to get path endpoint for the prevew");
            continue;
        }
        Vector2 from = a->castle.position;
        Vector2 to   = b->castle.position;
        from = Vector2Multiply (from, (Vector2){ scale_w, scale_h });
        to   = Vector2Multiply (to,   (Vector2){ scale_w, scale_h });
        from = Vector2Add      (from, (Vector2){ area.x, area.y });
        to   = Vector2Add      (to,   (Vector2){ area.x, area.y });
        DrawLineEx(from, to, path_thickness, ORANGE);
    }

    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        Vector2 point = Vector2Multiply(region->castle.position, (Vector2){ scale_w, scale_h });
        point = Vector2Add(point, (Vector2){ area.x, area.y });
        Color col = get_player_color(region->player_id);
        DrawCircleV(point, region_size, col);
    }
}
int render_map_list (Rectangle area, ListMap * maps, usize from, usize len) {
    int selected = -1;
    DrawRectangleRec(area, BLACK);
    DrawRectangleLinesEx(area, UI_BORDER_SIZE, DARKGRAY);

    if (len == 0) {
        TraceLog(LOG_WARNING, "No map entries to render");
        return selected;
    }
    if ((from + len) > maps->len) {
        TraceLog(LOG_ERROR, "Attempting to render more maps than there's in the list");
        return selected;
    }
    Vector2 cursor = GetMousePosition();

    Rectangle * boxes = temp_alloc(sizeof(Rectangle) * len);
    if (boxes == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for map list");
        return selected;
    }

    area = cake_margin_all(area, UI_BORDER_SIZE + 2);
    cake_layers(area, len, boxes, UI_FONT_SIZE_BUTTON + ui_margin(), ui_spacing());
    for (usize i = 0; i < len; i++) {
        usize index = i + from;
        draw_button(boxes[i], maps->items[index].name, cursor, UI_LAYOUT_LEFT, DARKBLUE, BLUE, RAYWHITE);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, boxes[i])) {
                selected = index;
            }
        }
    }
    return selected;
}

