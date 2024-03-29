#include "ui.h"
#include "input.h"
#include "std.h"
#include "alloc.h"
#include "constants.h"
#include "level.h"
#define CAKE_LAYOUT_IMPLEMENTATION
#define CAKE_RECT Rectangle
#include "cake.h"
#include <stdio.h>
#include <raymath.h>
#include <stdio.h>
#include "audio.h"

void draw_button (
    Rectangle     area,
    const char  * text,
    Vector2       cursor,
    UiLayout      label_layout,
    const Theme * theme
) {
    Rectangle text_area;
    switch (label_layout) {
        case UI_LAYOUT_LEFT: {
            text_area = cake_carve_to(area, area.width, theme->font_size);
            int space = MeasureText(" ", theme->font_size);
            text_area.x += space;
        } break;
        case UI_LAYOUT_CENTER: {
            int label_width = MeasureText(text, theme->font_size);
            text_area = cake_carve_to(area, label_width, theme->font_size);
        } break;
        case UI_LAYOUT_RIGHT: {
            int label_width = MeasureText(text, theme->font_size);
            text_area = cake_carve_to(area, area.width, theme->font_size);
            text_area.x += area.width - label_width;
            int space = MeasureText(" ", theme->font_size);
            text_area.x -= space;
        } break;
    }

    if (CheckCollisionPointRec(cursor, area)) {
        DrawRectangleRec(area, theme->button_hover);
        DrawRectangleLinesEx(area, theme->frame_thickness * 1.5, theme->button_frame);
        DrawText(text, text_area.x, text_area.y, theme->font_size, theme->text);
    }
    else {
        DrawRectangleRec(area, theme->button);
        DrawRectangleLinesEx(area, theme->frame_thickness, theme->button_frame);
        DrawText(text, text_area.x, text_area.y, theme->font_size, theme->text_dark);
    }
}

void draw_building_button (
    Rectangle     area,
    Texture2D     icon,
    const char  * text,
    const char  * cost,
    float         cost_offset,
    Vector2       cursor,
    Test          active,
    const Theme * theme
) {
    Rectangle button = cake_margin_all(area, theme->margin * 2);
    Rectangle image  = cake_cut_vertical(&button, -button.height, theme->spacing);
    button = cake_shrink_to(button, theme->font_size);
    Rectangle label      = cake_cut_vertical(&button, cost_offset, theme->spacing);
    Rectangle label_cost = button;

    if (active && CheckCollisionPointRec(cursor, area)) {
        DrawRectangleRec(area, theme->button_hover);
        DrawRectangleLinesEx(area, theme->frame_thickness, theme->button_frame);
        DrawTexturePro(icon, (Rectangle){0, 0, icon.width, icon.height}, image, Vector2Zero(), 0, WHITE);
        DrawText(text, label.x, label.y, theme->font_size, theme->text);
        DrawText(cost, label_cost.x, label_cost.y, theme->font_size, theme->text);
    }
    else {
        DrawRectangleRec(area, active ? theme->button : theme->button_inactive);
        DrawRectangleLinesEx(area, theme->frame_thickness, theme->button_frame);
        DrawTexturePro(icon, (Rectangle){0, 0, icon.width, icon.height}, image, Vector2Zero(), 0, WHITE);
        DrawText(text, label.x, label.y, theme->font_size, theme->text_dark);
        DrawText(cost, label_cost.x, label_cost.y, theme->font_size, theme->text_dark);
    }
}

/* Drawers *******************************************************************/
void draw_background(Rectangle area, const Theme * theme) {
    DrawRectangleRec(area, theme->background);
    DrawRectangleLinesEx(area, theme->frame_thickness, theme->frame);
}

/* Helper UI *****************************************************************/
void render_interaction (const GameState * state, Vector2 position, usize player) {
    usize phase = FPS * 2;
    unsigned char leftover = 255 - phase;
    usize turning_point = phase / 2;
    usize frame = state->turn % phase;
    if (frame >= turning_point) frame = phase - frame;

    Color col = get_player_color(player);
    col.a = leftover + frame;
    Color col2 = (Color){ 255, 255, 255, 0 };

    DrawCircleGradient(position.x, position.y, PLAYER_SELECTION_RADIUS, col, col2);
}
void render_interaction_hints (const GameState * state) {
    usize player;
    if (get_local_player_index(state, &player)) return;

    Vector2 mouse = mouse_position_inworld(state->camera);
    Region * region;
    if (region_by_position(&state->map, mouse, &region)) {
        return;
    }
    if (region->player_id != player) {
        #ifdef DEBUG
        if (IsKeyUp(KEY_LEFT_SHIFT)) {
            return;
        }
        #else
        return;
        #endif
    }

    for (usize i = 0; i < region->buildings.len; i++) {
        Building * b = &region->buildings.items[i];
        if (CheckCollisionPointCircle(mouse, b->position, PLAYER_SELECTION_RADIUS)) {
            render_interaction(state, b->position, player);
            break;
        }
    }

    for (usize i = 0; i < region->paths.len; i++) {
        Path * path = region->paths.items[i];
        Vector2 point;
        if (path->region_a == region) {
            point = path->lines.items[0].a;
        }
        else {
            point = path->lines.items[path->lines.len - 1].b;
        }
        if (CheckCollisionPointCircle(mouse, point, PLAYER_SELECTION_RADIUS)) {
            render_interaction(state, point, player);
            return;
        }
    }
}
void theme_update (Theme * theme) {
    // TODO make those scale with resolution when necessary
    #if defined(ANDROID)
    int height = GetScreenHeight();
    theme->font_size = height * 0.03f;
    theme->margin = theme->font_size * 0.5f;
    theme->spacing = theme->margin * 0.8f;
    theme->frame_thickness = 2;

    theme->info_bar_field_width = GetScreenWidth() * 0.1f;
    theme->info_bar_height = theme->font_size + theme->margin * 2.0f;

    theme->dialog_build_width = theme->font_size * 20.0f;
    theme->dialog_build_height = theme->font_size * 15.0f;
    theme->dialog_upgrade_width = theme->font_size * 15.0f;
    theme->dialog_upgrade_height = theme->font_size * 15.0f;
    #else
    theme->font_size = 20;
    theme->margin = 5;
    theme->spacing = 4;
    theme->frame_thickness = 2;

    theme->info_bar_field_width = 200;
    theme->info_bar_height = 30;

    theme->dialog_build_width = 350;
    theme->dialog_build_height = 350;
    theme->dialog_upgrade_width = 350;
    theme->dialog_upgrade_height = 350;
    #endif
    SetTextLineSpacing(theme->font_size * 1.15f);
}
Theme theme_setup () {
    Theme theme = {0};
    theme.background = BLACK;
    theme.background_light = (Color){ 32, 32, 32, 255 };
    theme.frame = DARKGRAY;
    theme.frame_light = GRAY;
    theme.text = WHITE;
    theme.text_dark = LIGHTGRAY;
    theme.button = DARKBLUE;
    theme.button_inactive = DARKGRAY;
    theme.button_hover = BLUE;
    theme.button_frame = RAYWHITE;
    theme_update(&theme);
    return theme;
}

/* In Game Menu **************************************************************/
EmptyDialog empty_dialog (Vector2 position, const Theme * theme) {
    EmptyDialog result;

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    #if defined(ANDROID)
    position = (Vector2) { screen.width  - theme->dialog_build_width  * 0.5f - theme->info_bar_height,
                           screen.height - theme->dialog_build_height * 0.5f - theme->info_bar_height };
    #endif
    cake_cut_horizontal(&screen, theme->info_bar_height, 0);

    result.area = cake_rect(theme->dialog_build_width + theme->spacing * 2.0f + theme->margin * 2.0f,
                            theme->dialog_build_height + theme->margin * 2.0f + theme->spacing);
    result.area = cake_center_rect(result.area, position.x, position.y);
    cake_clamp_inside(&result.area, screen);

    Rectangle butt = cake_margin_all(result.area, theme->margin);
    Rectangle buttons[5];
    cake_split_horizontal(butt, 5, buttons, theme->spacing);

    result.fighter  = buttons[0];
    result.archer   = buttons[1];
    result.support  = buttons[2];
    result.special  = buttons[3];
    result.resource = buttons[4];

    return result;
}
BuildingDialog building_dialog (Vector2 position, const Theme * theme) {
    BuildingDialog result;

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    #if defined(ANDROID)
    position = (Vector2) { screen.width  - theme->dialog_upgrade_width  * 0.5f - theme->info_bar_height,
                           screen.height - theme->dialog_upgrade_height * 0.5f - theme->info_bar_height };
    #endif
    cake_cut_horizontal(&screen, theme->info_bar_height, 0);

    result.area = cake_rect(theme->dialog_upgrade_width  + theme->margin * 2.0f,
                            theme->dialog_upgrade_height + theme->margin * 2.0f + theme->spacing * 2.0f);
    result.area = cake_center_rect(result.area, position.x, position.y);
    cake_clamp_inside(&result.area, screen);

    Rectangle butt = cake_margin_all(result.area, theme->margin);
    Rectangle buttons[3];
    cake_split_horizontal(butt, 3, buttons, theme->spacing);

    result.label    = buttons[0];
    result.upgrade  = buttons[1];
    result.demolish = buttons[2];

    return result;
}
Range map_zoom_range (const GameState * state) {
    float screen_width = GetScreenWidth();
    float screen_height = GetScreenHeight();
    Vector2 map_size;
    map_size.x = (screen_width  - 20.0f) / (float)state->map.width;
    map_size.y = (screen_height - 20.0f - state->settings->theme.info_bar_height) / (float)state->map.height;
    map_size.x = (map_size.x < map_size.y) ? map_size.x : map_size.y;
    map_size.y = 10.0f;
    return (Range) { map_size.x, map_size.y };
}
Rectangle flag_button_position () {
    Rectangle result = cake_rect(250, 250);
    result.x = GetScreenWidth() - result.width * 1.2f;
    result.y = GetScreenHeight() - result.height * 1.2f;
    return result;
}
CameraJoystick android_camera_control (const Theme * theme) {
    float width = GetScreenWidth();
    float height = GetScreenHeight() - theme->info_bar_height;

    float widget_size = (width > height) ? height : width;
    widget_size = widget_size * 0.3f;

    Rectangle joystick = {
        .x = theme->info_bar_height * 1.5f,
        .y = height - widget_size - theme->info_bar_height * 0.5f,
        .width = widget_size,
        .height = widget_size
    };

    Rectangle zoom_in = {
        .x = joystick.x + joystick.width * 0.25f,
        .y = joystick.y - widget_size * 0.75f,
        .width = widget_size * 0.5f,
        .height = widget_size * 0.5f,
    };

    Rectangle zoom_out = {
        .x = joystick.x + joystick.width * 1.25f,
        .y = joystick.y + widget_size * 0.25f,
        .width = zoom_in.width,
        .height = zoom_in.height,
    };

    return (CameraJoystick) {
        .joystick = joystick,
        .zoom_in = zoom_in,
        .zoom_out = zoom_out,
    };
}

Result ui_building_action_click (BuildingDialog dialog, Vector2 cursor, BuildingAction * action) {
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
Result ui_building_buy_click (EmptyDialog dialog, Vector2 cursor, BuildingType * result) {
    if (! CheckCollisionPointRec(cursor, dialog.area)) {
        return FAILURE;
    }

    if (CheckCollisionPointRec(cursor, dialog.fighter)) {
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
    const Theme * theme = &state->settings->theme;
    Vector2 ui_box = GetWorldToScreen2D(state->selected_building->position, state->camera);
    EmptyDialog dialog = empty_dialog(ui_box, &state->settings->theme);

    usize player_id;
    if (get_local_player_index(state, &player_id)) {
        #if defined(RELEASE)
        return;
        #else
        player_id = 1;
        #endif
    }
    PlayerData * player = &state->players.items[player_id];
    FactionType faction = player->faction;

    DrawRectangleRec(dialog.area, state->settings->theme.background);

    Texture2D icon_fighter  = state->resources->buildings[faction].fighter[0];
    Texture2D icon_archer   = state->resources->buildings[faction].archer[0];
    Texture2D icon_support  = state->resources->buildings[faction].support[0];
    Texture2D icon_special  = state->resources->buildings[faction].special[0];
    Texture2D icon_resource = state->resources->buildings[faction].resource[0];

    const char * name_fighter  = building_name(BUILDING_FIGHTER  , faction, 0);
    const char * name_archer   = building_name(BUILDING_ARCHER   , faction, 0);
    const char * name_support  = building_name(BUILDING_SUPPORT  , faction, 0);
    const char * name_special  = building_name(BUILDING_SPECIAL  , faction, 0);
    const char * name_resource = building_name(BUILDING_RESOURCE , faction, 0);
    float max_width = MeasureText(name_fighter, theme->font_size);
    {
        float width;
        #define UPDATE_WIDTH(x) width = MeasureText(x, theme->font_size); \
        if (width > max_width) max_width = width;

        UPDATE_WIDTH(name_archer)
        UPDATE_WIDTH(name_support)
        UPDATE_WIDTH(name_special)
        UPDATE_WIDTH(name_resource)
        #undef UPDATE_WIDTH
    }
    max_width = max_width * 1.1f;

    usize cost_fighter  = building_buy_cost(BUILDING_FIGHTER);
    usize cost_archer   = building_buy_cost(BUILDING_ARCHER);
    usize cost_support  = building_buy_cost(BUILDING_SUPPORT);
    usize cost_special  = building_buy_cost(BUILDING_SPECIAL);
    usize cost_resource = building_buy_cost(BUILDING_RESOURCE);

    const usize buffer_size = 10;
    char cost_label_fighter  [buffer_size];
    char cost_label_archer   [buffer_size];
    char cost_label_support  [buffer_size];
    char cost_label_special  [buffer_size];
    char cost_label_resource [buffer_size];

    snprintf(cost_label_fighter  , buffer_size, "Cost: %zu", cost_fighter);
    snprintf(cost_label_archer   , buffer_size, "Cost: %zu", cost_archer);
    snprintf(cost_label_support  , buffer_size, "Cost: %zu", cost_support);
    snprintf(cost_label_special  , buffer_size, "Cost: %zu", cost_special);
    snprintf(cost_label_resource , buffer_size, "Cost: %zu", cost_resource);

    Vector2 cursor = GetMousePosition();

    draw_building_button(
        dialog.fighter,
        icon_fighter,
        name_fighter,
        cost_label_fighter,
        max_width,
        cursor,
        cost_fighter <= player->resource_gold,
        theme
    );
    draw_building_button(
        dialog.archer,
        icon_archer,
        name_archer,
        cost_label_archer,
        max_width,
        cursor,
        cost_archer <= player->resource_gold,
        theme
    );
    draw_building_button(
        dialog.support,
        icon_support,
        name_support,
        cost_label_support,
        max_width,
        cursor,
        cost_support <= player->resource_gold,
        theme
    );
    draw_building_button(
        dialog.special,
        icon_special,
        name_special,
        cost_label_special,
        max_width,
        cursor,
        cost_special <= player->resource_gold,
        theme
    );
    draw_building_button(
        dialog.resource,
        icon_resource,
        name_resource,
        cost_label_resource,
        max_width,
        cursor,
        cost_resource <= player->resource_gold,
        theme
    );
}
void render_upgrade_building_dialog (const GameState * state) {
    Vector2 building_pos = GetWorldToScreen2D(state->selected_building->position, state->camera);

    const Theme * theme = &state->settings->theme;
    BuildingDialog dialog = building_dialog(building_pos, theme);
    usize player_id;
    if (get_local_player_index(state, &player_id)) {
        player_id = 1;
    }
    PlayerData * player = &state->players.items[player_id];
    FactionType faction = player->faction;
    Building * building = state->selected_building;

    DrawRectangleRec(dialog.area, theme->background);
    const char * text = building_name(building->type, faction, building->upgrades);
    const char * lvl;
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
    usize max_units = building_max_units(building);
    usize now_units = building->units_spawned;
    char counter[20];
    snprintf(counter, 20, "Units: %zu/%zu", now_units, max_units);

    float title_scale = 1.5f;

    DrawRectangleRec(dialog.label, theme->background_light);
    dialog.label = cake_margin_all(dialog.label, theme->margin);

    Rectangle r_title = cake_cut_horizontal(&dialog.label, theme->font_size * title_scale, theme->spacing * 2);
    Rectangle r_level = cake_cut_horizontal(&dialog.label, theme->font_size, theme->spacing);
    Rectangle r_units = cake_cut_horizontal(&dialog.label, theme->font_size, theme->spacing);

    r_title = cake_carve_width(r_title, MeasureText(text, theme->font_size * title_scale), 0.5f);

    DrawText(text, r_title.x, r_title.y, theme->font_size * title_scale, theme->text);
    DrawText(lvl, r_level.x, r_level.y, theme->font_size, theme->text);
    DrawText(counter, r_units.x, r_units.y, theme->font_size, theme->text);

    Vector2 cursor = GetMousePosition();

    if (building->upgrades < BUILDING_MAX_UPGRADES) {
        const char * u_label = building_name(building->type, faction, building->upgrades + 1);
        usize cost = building_upgrade_cost(building);
        char u_cost[10];
        if (building->upgrades >= BUILDING_MAX_UPGRADES) {
            u_cost[0] = 0;
        }
        else {
            snprintf(u_cost, 10, "Cost: %zu", cost);
        }
        Texture2D icon = building_image(state->resources, faction, building->type, building->upgrades + 1);
        Test active = cost <= player->resource_gold;

        Rectangle button = cake_margin_all(dialog.upgrade, theme->margin * 2);
        Rectangle image  = cake_cut_vertical(&button, -button.height, theme->spacing);
        button = cake_shrink_to(button, theme->font_size * 2 + theme->spacing * 2);

        Rectangle label      = cake_cut_horizontal(&button, theme->font_size, theme->spacing * 2);
        Rectangle label_cost = button;

        if (active && CheckCollisionPointRec(cursor, dialog.upgrade)) {
            DrawRectangleRec(dialog.upgrade, theme->button_hover);
            DrawRectangleLinesEx(dialog.upgrade, theme->frame_thickness, theme->button_frame);
            DrawTexturePro(icon, (Rectangle){0, 0, icon.width, icon.height}, image, Vector2Zero(), 0, WHITE);
            DrawText(u_label, label.x, label.y, theme->font_size, theme->text);
            DrawText(u_cost, label_cost.x, label_cost.y, theme->font_size, theme->text);
        }
        else {
            DrawRectangleRec(dialog.upgrade, active ? theme->button : theme->button_inactive);
            DrawRectangleLinesEx(dialog.upgrade, theme->frame_thickness, theme->button_frame);
            DrawTexturePro(icon, (Rectangle){0, 0, icon.width, icon.height}, image, Vector2Zero(), 0, WHITE);
            DrawText(u_label, label.x, label.y, theme->font_size, theme->text_dark);
            DrawText(u_cost, label_cost.x, label_cost.y, theme->font_size, theme->text_dark);
        }
    }
    else {
        DrawRectangleRec(dialog.upgrade, theme->button_inactive);
        DrawRectangleLinesEx(dialog.upgrade, theme->frame_thickness, theme->button_frame);
        dialog.upgrade = cake_carve_to(dialog.upgrade, MeasureText("Max Level", theme->font_size), theme->font_size);
        DrawText("Max Level", dialog.upgrade.x, dialog.upgrade.y, theme->font_size, theme->text_dark);
    }

    draw_button(dialog.demolish, "Demolish", cursor, UI_LAYOUT_CENTER, theme);
}
void render_path_button (const GameState * state) {
    Rectangle area = flag_button_position();

    DrawRectangleRec(area, state->settings->theme.button);
    DrawRectangleLinesEx(area, state->settings->theme.frame_thickness, state->settings->theme.button_frame);
    usize player;
    if (get_local_player_index(state, &player)) {
        player = 1;
    }
    Texture2D flag = state->resources->flag;
    DrawTexturePro(flag, (Rectangle) { 0, 0, flag.width, flag.height }, area, Vector2Zero(), 0, get_player_color(player));
}
void render_camera_controls (const GameState * state) {
    const Theme * theme = &state->settings->theme;

    CameraJoystick joy = android_camera_control(theme);

    Vector2 joy_center = { joy.joystick.x + joy.joystick.width  * 0.5f,
                           joy.joystick.y + joy.joystick.height * 0.5f };

    Vector2 plus_center = { joy.zoom_in.x + joy.zoom_in.width * 0.5f,
                            joy.zoom_in.y + joy.zoom_in.height * 0.5f };

    Vector2 minus_center = { joy.zoom_out.x + joy.zoom_out.width * 0.5f,
                             joy.zoom_out.y + joy.zoom_out.height * 0.5f };

    joy.joystick.width *= 0.5f;
    joy.joystick.height *= 0.5f;
    joy.zoom_in.width *= 0.5f;
    joy.zoom_in.height *= 0.5f;
    joy.zoom_out.width *= 0.5f;
    joy.zoom_out.height *= 0.5f;

    DrawCircleV(joy_center, joy.joystick.width, theme->button);
    DrawCircleLinesV(joy_center, joy.joystick.width, theme->button_frame);

    DrawCircleV(minus_center, joy.zoom_out.width, theme->button);
    DrawCircleLinesV(minus_center, joy.zoom_out.width, theme->button_frame);
    DrawCircleV(plus_center, joy.zoom_in.width, theme->button);
    DrawCircleLinesV(plus_center, joy.zoom_in.width, theme->button_frame);

    Vector2 plus_left = { plus_center.x - joy.zoom_in.width * 0.5f, plus_center.y };
    Vector2 plus_rigt = { plus_center.x + joy.zoom_in.width * 0.5f, plus_center.y };
    Vector2 plus_top = { plus_center.x, plus_center.y - joy.zoom_in.height * 0.5f };
    Vector2 plus_bot = { plus_center.x, plus_center.y + joy.zoom_in.height * 0.5f };

    Vector2 minus_left = { minus_center.x - joy.zoom_out.width * 0.5f, minus_center.y };
    Vector2 minus_rigt = { minus_center.x + joy.zoom_out.width * 0.5f, minus_center.y };

    DrawLineEx(plus_left  , plus_rigt  , theme->frame_thickness, theme->text);
    DrawLineEx(plus_top   , plus_bot   , theme->frame_thickness, theme->text);
    DrawLineEx(minus_left , minus_rigt , theme->frame_thickness, theme->text);

    Vector2 arrow_left = { joy_center.x - joy.joystick.width * 0.75f, joy_center.y };
    Vector2 arrow_rigt = { joy_center.x + joy.joystick.width * 0.75f, joy_center.y };
    Vector2 arrow_top = { joy_center.x, joy_center.y - joy.joystick.height * 0.75f };
    Vector2 arrow_bot = { joy_center.x, joy_center.y + joy.joystick.height * 0.75f };

    Vector2 arrow_top_l = { arrow_top.x - joy.joystick.width * 0.15f, arrow_top.y + joy.joystick.height * 0.15f };
    Vector2 arrow_top_r = { arrow_top.x + joy.joystick.width * 0.15f, arrow_top.y + joy.joystick.height * 0.15f };
    Vector2 arrow_bot_l = { arrow_bot.x - joy.joystick.width * 0.15f, arrow_bot.y - joy.joystick.height * 0.15f };
    Vector2 arrow_bot_r = { arrow_bot.x + joy.joystick.width * 0.15f, arrow_bot.y - joy.joystick.height * 0.15f };
    Vector2 arrow_left_t = { arrow_left.x + joy.joystick.width * 0.15f, arrow_left.y - joy.joystick.height * 0.15f };
    Vector2 arrow_left_b = { arrow_left.x + joy.joystick.width * 0.15f, arrow_left.y + joy.joystick.height * 0.15f };
    Vector2 arrow_rigt_t = { arrow_rigt.x - joy.joystick.width * 0.15f, arrow_rigt.y - joy.joystick.height * 0.15f };
    Vector2 arrow_rigt_b = { arrow_rigt.x - joy.joystick.width * 0.15f, arrow_rigt.y + joy.joystick.height * 0.15f };

    DrawLineEx(arrow_left, arrow_rigt, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_top, arrow_bot, theme->frame_thickness, theme->text);

    DrawLineEx(arrow_top, arrow_top_l, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_top, arrow_top_r, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_bot, arrow_bot_l, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_bot, arrow_bot_r, theme->frame_thickness, theme->text);

    DrawLineEx(arrow_left, arrow_left_t, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_left, arrow_left_b, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_rigt, arrow_rigt_t, theme->frame_thickness, theme->text);
    DrawLineEx(arrow_rigt, arrow_rigt_b, theme->frame_thickness, theme->text);
}
void render_winner (const GameState * state, usize winner) {
    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    cake_cut_horizontal(&screen, state->settings->theme.info_bar_height, 0);

    Color winner_color = get_player_color(winner);
    usize base_layers = 20 + (usize)(sinf(get_time()) * 10);
    char alpha_step = 255 / base_layers;

    for (usize i = 0; i < base_layers; i++) {
        DrawRectangleLinesEx(screen, 1.0, winner_color);
        winner_color.a -= alpha_step;
        screen = cake_margin_all(screen, 1.0);
    }

    const Theme * theme = &state->settings->theme;

    char text[256];

    usize player;
    if (get_local_player_index(state, &player)) {
        snprintf(text, 256, "Player %zu won the game!", winner);
    }
    else {
        if (player == winner) {
            snprintf(text, 256, "You won!");
        }
        else {
            snprintf(text, 256, "You lost! Winner is Player %zu!", winner);
        }
    }
    int width = MeasureText(text, theme->font_size);

    screen = cake_cut_horizontal(&screen, 0.5, 0);
    screen = cake_carve_to(screen, width * 1.5f, theme->font_size * 4.0f);
    DrawRectangleRec(screen, theme->background);
    DrawRectangleLinesEx(screen, theme->frame_thickness, theme->frame);

    winner_color.a = 255;
    base_layers /= 2;
    alpha_step = 255 / base_layers;
    for (usize i = 0; i < base_layers; i++) {
        screen = cake_grow_by(screen, 1, 1);
        DrawRectangleLinesEx(screen, 1, winner_color);
        winner_color.a -= alpha_step;
    }
    screen = cake_carve_to(screen, width, theme->font_size);

    DrawText(text, screen.x, screen.y, theme->font_size, theme->text);
}
InfoBarAction render_resource_bar (const GameState * state) {
    usize player_index;
    if (get_local_player_index(state, &player_index)) {
        player_index = 1;
    }
    PlayerData * player = &state->players.items[player_index];

    float income = get_expected_income(&state->map, player_index);
    float upkeep = get_expected_maintenance_cost(&state->map, player_index);

    const Theme * theme = &state->settings->theme;
    Rectangle bar = cake_rect(GetScreenWidth(), theme->info_bar_height);
    Rectangle mbar = cake_margin_all(bar, theme->margin);

    DrawRectangleRec(bar, theme->background_light);
    DrawRectangleLinesEx(bar, theme->frame_thickness, get_player_color(player_index));

    const usize buffer_size = 256;
    char buffer[buffer_size];

    const char * fmt;
    switch (player->faction) {
        case FACTION_KNIGHTS: fmt = "Gold: %zu"; break;
        case FACTION_MAGES:   fmt = "Mana: %zu"; break;
        default: TraceLog(LOG_FATAL, "Invalid player faction"); return 0;
    }
    char * label;
    if (snprintf(buffer, buffer_size, fmt, player->resource_gold) <= 0) {
        label = "too much...";
    }
    else {
        label = buffer;
    }
    int label_width = MeasureText(label, theme->font_size);
    if (label_width < theme->info_bar_field_width)
        label_width = theme->info_bar_field_width;
    Rectangle rect = cake_cut_vertical(&mbar, label_width, theme->spacing);
    DrawText(label , rect.x, rect.y, theme->font_size, theme->text);

    if (snprintf(buffer, buffer_size, "Income: %.1f/s ", income) <= 0) {
        label = "too much ...";
    }
    else {
        label = buffer;
    }
    label_width = MeasureText(label, theme->font_size);
    if (label_width < theme->info_bar_field_width)
        label_width = theme->info_bar_field_width;
    rect = cake_cut_vertical(&mbar, label_width, theme->spacing);
    DrawText(label , rect.x, rect.y, theme->font_size, theme->text);

    if (snprintf(buffer, buffer_size, "Upkeep: %.1f/s ", upkeep) <= 0) {
        label = "too much ...";
    }
    else {
        label = buffer;
    }
    label_width = MeasureText(label, theme->font_size);
    if (label_width < theme->info_bar_field_width)
        label_width = theme->info_bar_field_width;
    rect = cake_cut_vertical(&mbar, label_width, theme->spacing);
    DrawText(label , rect.x, rect.y, theme->font_size, theme->text);

    char * menu_label = "Settings";
    char * quit_label = "Exit to Menu";
    Rectangle quit = cake_cut_vertical(&mbar, -MeasureText(quit_label, theme->font_size) * 1.2f, theme->spacing);
    Rectangle menu = cake_cut_vertical(&mbar, -MeasureText(menu_label, theme->font_size) * 1.2f, theme->spacing);
    Vector2 cursor = GetMousePosition();
    draw_button(quit, quit_label, cursor, UI_LAYOUT_CENTER, theme);
    draw_button(menu, menu_label, cursor, UI_LAYOUT_CENTER, theme);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(cursor, menu)) {
            return INFO_BAR_ACTION_SETTINGS;
        }
        if (CheckCollisionPointRec(cursor, quit)) {
            return INFO_BAR_ACTION_QUIT;
        }
    }
    return INFO_BAR_ACTION_NONE;
}

/* Main Menu *****************************************************************/
MainMenuLayout main_menu_layout () {
    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    const uint8_t parts_count = 5;
    Rectangle parts[parts_count];
    cake_split_vertical(screen, 3, parts, 0.0f);
    cake_split_horizontal(parts[1], 3, parts, 0.0f);

    cake_split_horizontal(parts[1], parts_count, parts, 0.0f);

    MainMenuLayout result = {
        .new_game = cake_carve_to(parts[0], 250.0f, 50.0f),
        .tutorial = cake_carve_to(parts[1], 250.0f, 50.0f),
        .manual   = cake_carve_to(parts[2], 250.0f, 50.0f),
        .options  = cake_carve_to(parts[3], 250.0f, 50.0f),
        .quit     = cake_carve_to(parts[4], 250.0f, 50.0f),
    };
    return result;
}
void render_simple_map_preview (Rectangle area, Map * map, const Theme * theme) {
    DrawRectangleRec(area, theme->background);
    DrawRectangleLinesEx(area, theme->frame_thickness, theme->frame);

    if (map == NULL) {
        DrawLine(area.x, area.y, area.x + area.width, area.y + area.height, theme->frame);
        DrawLine(area.x + area.width, area.y, area.x, area.y + area.height, theme->frame);
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
        DrawLineEx(from, to, theme->frame_thickness, theme->frame_light);
    }

    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        Vector2 point = Vector2Multiply(region->castle.position, (Vector2){ scale_w, scale_h });
        point = Vector2Add(point, (Vector2){ area.x, area.y });
        Color col = get_player_color(region->player_id);
        DrawCircleV(point, theme->frame_thickness * 3.0f + 1.0f, theme->frame_light);
        DrawCircleV(point, theme->frame_thickness * 3.0f, col);
    }
}
void render_player_select (Rectangle area, GameState * state, int selected_map) {
    if (selected_map < 0) return;
    const Theme * theme = &state->settings->theme;
    const Map * map = &state->resources->maps.items[selected_map];

    DrawRectangleRec(area, theme->background);
    DrawRectangleLinesEx(area, theme->frame_thickness, theme->frame);
    area = cake_margin_all(area, theme->frame_thickness * 3);

    Rectangle title = cake_cut_horizontal(&area, theme->font_size, theme->font_size * 0.5f);
    title = cake_diet_to(title, MeasureText(map->name, theme->font_size));
    DrawText(map->name, title.x, title.y, theme->font_size, theme->text);

    Rectangle * lines = temp_alloc(sizeof(Rectangle) * map->player_count);
    Rectangle * player_dropdowns = temp_alloc(sizeof(Rectangle) * map->player_count);
    Rectangle * faction_dropdowns = temp_alloc(sizeof(Rectangle) * map->player_count);

    for (usize i = 0; i < map->player_count; i++) {
        lines[i] = cake_cut_horizontal(&area, theme->font_size * 1.5f, theme->font_size * 0.5f);
    }

    Vector2 mouse = GetMousePosition();

    char name[64];
    char * player_control = "Controlling: ";
    char * player_faction = "Faction: ";
    char * button_label = "v";
    int player_control_size = MeasureText(player_control, theme->font_size);
    int player_faction_size = MeasureText(player_faction, theme->font_size);
    int button_label_size = MeasureText(button_label, theme->font_size);

    static int selected_player = -1;
    static int choosing = -1; // 0 - PC/CPU, 1 - Faction

    for (usize i = 0; i < map->player_count; i++) {
        PlayerData * player = &state->players.items[i + 1];
        DrawRectangleRec(lines[i], theme->background_light);

        snprintf(name, 64, "Player %zu", i + 1);
        Rectangle label = cake_cut_vertical(&lines[i], 0.3f, 5);
        cake_cut_vertical(&label, theme->frame_thickness * 2, 0);
        Rectangle color_box = cake_cut_vertical(&label, theme->font_size, theme->frame_thickness * 2);
        color_box = cake_shrink_to(color_box, color_box.width);
        DrawRectangleRec(color_box, get_player_color(i + 1));
        label = cake_shrink_to(label, theme->font_size);
        DrawText(name, label.x, label.y, theme->font_size, theme->text);

        player_dropdowns[i] = cake_cut_vertical(&lines[i], 0.5f, 5);
        faction_dropdowns[i] = lines[i];

        // select who controls this player
        label = cake_cut_vertical(&player_dropdowns[i], player_control_size, 6);
        label = cake_shrink_to(label, theme->font_size);
        DrawText(player_control, label.x, label.y, theme->font_size, theme->text);

        player_dropdowns[i] = cake_margin_all(player_dropdowns[i], theme->frame_thickness);

        int mouse_over = selected_player >= 0 ? 0 : CheckCollisionPointRec(mouse, player_dropdowns[i]);
        DrawRectangleRec(player_dropdowns[i], theme->background);
        DrawRectangleLinesEx(player_dropdowns[i], theme->frame_thickness, mouse_over ? theme->frame_light : theme->frame);

        label = player_dropdowns[i];
        Rectangle button = cake_cut_vertical(&label, -label.height, 0);
        DrawRectangleRec(button, mouse_over ? theme->frame_light : theme->frame);
        button = cake_carve_to(button, button_label_size, theme->font_size);
        DrawText(button_label, button.x, button.y, theme->font_size, theme->text);

        char * player_control_label = player->type == PLAYER_LOCAL ? "Player" : "CPU";
        int player_control_label_size = MeasureText(player_control_label, theme->font_size);
        label = cake_carve_to(label, player_control_label_size, theme->font_size);
        DrawText(player_control_label, label.x, label.y, theme->font_size, theme->text);

        // select faction of this player
        label = cake_cut_vertical(&faction_dropdowns[i], player_faction_size, 6);
        label = cake_shrink_to(label, theme->font_size);
        DrawText(player_faction, label.x, label.y, theme->font_size, theme->text);

        faction_dropdowns[i] = cake_margin_all(faction_dropdowns[i], theme->frame_thickness);

        mouse_over = selected_player >= 0 ? 0 : CheckCollisionPointRec(mouse, faction_dropdowns[i]);
        DrawRectangleRec(faction_dropdowns[i], theme->background);
        DrawRectangleLinesEx(faction_dropdowns[i], theme->frame_thickness, mouse_over ? theme->frame_light : theme->frame);

        label = faction_dropdowns[i];
        button = cake_cut_vertical(&label, -label.height, 0);
        DrawRectangleRec(button, mouse_over ? theme->frame_light : theme->frame);
        button = cake_carve_to(button, button_label_size, theme->font_size);
        DrawText(button_label, button.x, button.y, theme->font_size, theme->text);

        char * faction_name = faction_to_string(state->players.items[i + 1].faction);
        int faction_name_size = MeasureText(faction_name, theme->font_size);
        label = cake_carve_to(label, faction_name_size, theme->font_size);
        DrawText(faction_name, label.x, label.y, theme->font_size, theme->text);
    }

    if (selected_player >= 0) {
        if (choosing) {
            Rectangle dropdown = faction_dropdowns[selected_player];
            dropdown.height = theme->font_size * (FACTION_LAST + 1) + theme->font_size * (FACTION_LAST * 0.5f) + theme->frame_thickness * 4;
            DrawRectangleRec(dropdown, theme->background);
            DrawRectangleLinesEx(dropdown, theme->frame_thickness, CheckCollisionPointRec(mouse, dropdown) ? theme->frame_light : theme->frame);
            dropdown = cake_margin_all(dropdown, theme->frame_thickness * 2);

            int clicked = 0;
            for (usize i = 0; i <= FACTION_LAST; i++) {
                Rectangle rect = cake_cut_horizontal(&dropdown, theme->font_size, theme->font_size * 0.5f);
                int mouse_over = CheckCollisionPointRec(mouse, rect);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (mouse_over) {
                        play_sound(state->resources, SOUND_UI_CLICK);
                        state->players.items[selected_player + 1].faction = i;
                        selected_player = -1;
                        return;
                    }
                    clicked = 1;
                }

                rect = cake_carve_to(rect, MeasureText(faction_to_string(i), theme->font_size), theme->font_size);
                DrawText(faction_to_string(i), rect.x, rect.y, theme->font_size, mouse_over ? theme->text : theme->text_dark);
            }
            if (clicked) {
                selected_player = -1;
                return;
            }
        }
        else {
            Rectangle dropdown = player_dropdowns[selected_player];
            dropdown.height = theme->font_size * 2 + theme->font_size * 0.5f + theme->frame_thickness * 4;
            DrawRectangleRec(dropdown, theme->background);
            DrawRectangleLinesEx(dropdown, theme->frame_thickness, CheckCollisionPointRec(mouse, dropdown) ? theme->frame_light : theme->frame);
            dropdown = cake_margin_all(dropdown, theme->frame_thickness * 2);
            Rectangle pc_rect = cake_cut_horizontal(&dropdown, theme->font_size, theme->font_size * 0.5f);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mouse, pc_rect)) {
                    play_sound(state->resources, SOUND_UI_CLICK);
                    state->players.items[selected_player + 1].type = PLAYER_LOCAL;
                    for (usize i = 0; i < map->player_count; i++) {
                        if (i == (usize)selected_player) continue;
                        if (PLAYER_LOCAL == state->players.items[i + 1].type) {
                            state->players.items[i + 1].type = PLAYER_AI;
                        }
                    }
                }
                else if (CheckCollisionPointRec(mouse, dropdown)) {
                    play_sound(state->resources, SOUND_UI_CLICK);
                    state->players.items[selected_player + 1].type = PLAYER_AI;
                }
                selected_player = -1;
                return;
            }
            int mouse_over = CheckCollisionPointRec(mouse, pc_rect);
            pc_rect = cake_carve_to(pc_rect, MeasureText("Player", theme->font_size), theme->font_size);
            DrawText("Player", pc_rect.x, pc_rect.y, theme->font_size, mouse_over ? theme->text : theme->text_dark);

            mouse_over = CheckCollisionPointRec(mouse, dropdown);
            dropdown = cake_carve_to(dropdown, MeasureText("CPU", theme->font_size), theme->font_size);
            DrawText("CPU", dropdown.x, dropdown.y, theme->font_size, mouse_over ? theme->text : theme->text_dark);
        }
    }
    else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (usize i = 0; i < map->player_count; i++) {
            if (CheckCollisionPointRec(mouse, player_dropdowns[i])) {
                play_sound(state->resources, SOUND_UI_CLICK);
                selected_player = i;
                choosing = 0;
                return;
            }
            if (CheckCollisionPointRec(mouse, faction_dropdowns[i])) {
                play_sound(state->resources, SOUND_UI_CLICK);
                selected_player = i;
                choosing = 1;
                return;
            }
        }
        selected_player = -1;
    }
}
int render_map_list (Rectangle area, ListMap * maps, usize from, usize len, const Theme * theme) {
    int selected = -1;
    DrawRectangleRec(area, theme->background);
    DrawRectangleLinesEx(area, theme->frame_thickness, theme->frame);

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

    area = cake_margin_all(area, theme->frame_thickness + 2);
    cake_layers(area, len, boxes, theme->font_size + theme->margin, theme->spacing);
    for (usize i = 0; i < len; i++) {
        usize index = i + from;
        draw_button(boxes[i], maps->items[index].name, cursor, UI_LAYOUT_LEFT, theme);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(cursor, boxes[i])) {
                selected = index;
            }
        }
    }
    return selected;
}
Test render_settings (Rectangle area, Settings * settings, const Assets * assets) {
    const Theme * theme = &settings->theme;

    Rectangle center = cake_carve_to(area, area.width * 0.8, theme->font_size * 15);
    DrawRectangleRec(center, theme->background);
    DrawRectangleLinesEx(center, theme->frame_thickness, theme->frame);
    Rectangle close = cake_carve_width(center, theme->font_size * 1.5f, 1.0f);
    close.height = close.width;
    center = cake_margin(center, theme->frame_thickness * 3, theme->frame_thickness * 3, theme->font_size * 2, theme->font_size * 2);

    Rectangle top_label = cake_cut_horizontal(&center, theme->font_size * 2, theme->font_size);

    Rectangle master_volume_slider = cake_cut_horizontal(&center, theme->font_size, theme->font_size);
    Rectangle music_volume_slider  = cake_cut_horizontal(&center, theme->font_size, theme->font_size);
    Rectangle sfx_volume_slider    = cake_cut_horizontal(&center, theme->font_size, theme->font_size);
    Rectangle ui_volume_slider     = cake_cut_horizontal(&center, theme->font_size, theme->font_size);
    #ifndef ANDROID
    Rectangle fullscreen_check     = cake_cut_horizontal(&center, theme->font_size, theme->font_size);
    #endif

    char * top_label_text = "Settings";
    char * master_label = "Global Volume: ";
    char * music_label = "Music Volume: ";
    char * sfx_label = "SFX Volume: ";
    char * ui_label = "UI Volume: ";
    #ifndef ANDROID
    char * fullscreen = "Fullscreen ";
    #endif

    top_label = cake_carve_width(top_label, MeasureText(top_label_text, theme->font_size), 0.5f);

    float labels_width = MeasureText(master_label, theme->font_size);

    {
        #define UPDATE_WIDTH(x) width = MeasureText(x, theme->font_size); \
        if (width > labels_width) labels_width = width;

        float width;
        UPDATE_WIDTH(music_label)
        UPDATE_WIDTH(sfx_label)
        UPDATE_WIDTH(ui_label)
        #ifndef ANDROID
        UPDATE_WIDTH(fullscreen)
        #endif
        #undef UPDATE_WIDTH
    }

    Rectangle master_volume_label = cake_cut_vertical(&master_volume_slider , labels_width, theme->spacing);
    Rectangle music_volume_label  = cake_cut_vertical(&music_volume_slider  , labels_width, theme->spacing);
    Rectangle sfx_volume_label    = cake_cut_vertical(&sfx_volume_slider    , labels_width, theme->spacing);
    Rectangle ui_volume_label     = cake_cut_vertical(&ui_volume_slider     , labels_width, theme->spacing);
    #ifndef ANDROID
    Rectangle fullscreen_label    = cake_cut_vertical(&fullscreen_check     , labels_width, theme->spacing);
    fullscreen_check.width = fullscreen_check.height;
    #endif

    float GetMasterVolume();
    float master_volume = GetMasterVolume();
    Rectangle master_dot = cake_carve_width(master_volume_slider , theme->font_size, master_volume);
    Rectangle music_dot  = cake_carve_width(music_volume_slider  , theme->font_size, settings->volume_music);
    Rectangle sfx_dot    = cake_carve_width(sfx_volume_slider    , theme->font_size, settings->volume_sfx);
    Rectangle ui_dot     = cake_carve_width(ui_volume_slider     , theme->font_size, settings->volume_ui);

    Vector2 cursor = GetMousePosition();
    int over_master = CheckCollisionPointRec(cursor, master_volume_slider);
    int over_music = CheckCollisionPointRec(cursor, music_volume_slider);
    int over_sfx = CheckCollisionPointRec(cursor, sfx_volume_slider);
    int over_ui = CheckCollisionPointRec(cursor, ui_volume_slider);

    static bool input_enabled = false;
    if (input_enabled == false) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            input_enabled = true;
        }
    }
    else {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            #ifndef ANDROID
            if (CheckCollisionPointRec(cursor, fullscreen_check)) {
                play_sound(assets, SOUND_UI_CLICK);
                if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
                    SetWindowState(FLAG_WINDOW_RESIZABLE);
                    settings->fullscreen = false;
                }
                else {
                    ClearWindowState(FLAG_WINDOW_RESIZABLE);
                    settings->fullscreen = true;
                }
                ToggleBorderlessWindowed();
            }
            #endif
            if (CheckCollisionPointRec(cursor, close)) {
                play_sound(assets, SOUND_UI_CLICK);
                input_enabled = false;
                return YES;
            }
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (over_master) {
                float local_pos = ( cursor.x - master_volume_slider.x ) / master_volume_slider.width;
                settings->volume_master = local_pos;
                SetMasterVolume(local_pos);
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    int sound = GetRandomValue(0, SOUND_UI_CLICK);
                    play_sound(assets, sound);
                }
            }
            else if (over_music) {
                float local_pos = ( cursor.x - music_volume_slider.x ) / music_volume_slider.width;
                settings->volume_music = local_pos;
                for (usize i = 0; i <= FACTION_LAST; i++) {
                    SetMusicVolume(assets->faction_themes[i], local_pos);
                }
                SetMusicVolume(assets->main_theme, local_pos);
            }
            else if (over_sfx) {
                float local_pos = ( cursor.x - sfx_volume_slider.x ) / sfx_volume_slider.width;
                settings->volume_sfx = local_pos;
                const ListSFX * sounds = &assets->sound_effects;
                for (usize i = 0; i < sounds->len; i++) {
                    if (sounds->items[i].kind != SOUND_UI_CLICK) {
                        SetSoundVolume(sounds->items[i].sound, local_pos);
                    }
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    int sound = GetRandomValue(0, SOUND_UI_CLICK-1);
                    play_sound(assets, sound);
                }
            }
            else if (over_ui) {
                float local_pos = ( cursor.x - ui_volume_slider.x ) / ui_volume_slider.width;
                settings->volume_ui = local_pos;
                const ListSFX * sounds = &assets->sound_effects;
                for (usize i = 0; i < sounds->len; i++) {
                    if (sounds->items[i].kind == SOUND_UI_CLICK) {
                        SetSoundVolume(sounds->items[i].sound, local_pos);
                    }
                }
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                    play_sound(assets, SOUND_UI_CLICK);
                }
            }
        }
    }

    DrawText(top_label_text, top_label.x, top_label.y, theme->font_size * 2.0f, theme->text);
    DrawText(master_label, master_volume_label.x, master_volume_label.y, theme->font_size, theme->text);
    DrawText(music_label, music_volume_label.x, music_volume_label.y, theme->font_size, theme->text);
    DrawText(sfx_label, sfx_volume_label.x, sfx_volume_label.y, theme->font_size, theme->text);
    DrawText(ui_label, ui_volume_label.x, ui_volume_label.y, theme->font_size, theme->text);
    #ifndef ANDROID
    DrawText(fullscreen, fullscreen_label.x, fullscreen_label.y, theme->font_size, theme->text);
    #endif

    // This code could be abstracted into slider function but unless it's needed elsewhere I'm not going to do it.
    DrawRectangleRec(master_volume_slider, theme->background_light);
    DrawRectangleRec(music_volume_slider, theme->background_light);
    DrawRectangleRec(sfx_volume_slider, theme->background_light);
    DrawRectangleRec(ui_volume_slider, theme->background_light);

    DrawRectangleLinesEx(master_volume_slider , theme->frame_thickness, over_master ? theme->frame_light : theme->frame);
    DrawRectangleLinesEx(music_volume_slider  , theme->frame_thickness, over_music  ? theme->frame_light : theme->frame);
    DrawRectangleLinesEx(sfx_volume_slider    , theme->frame_thickness, over_sfx    ? theme->frame_light : theme->frame);
    DrawRectangleLinesEx(ui_volume_slider     , theme->frame_thickness, over_ui     ? theme->frame_light : theme->frame);

    DrawRectangleRec(master_dot , over_master ? theme->button_hover : theme->button);
    DrawRectangleRec(music_dot  , over_music  ? theme->button_hover : theme->button);
    DrawRectangleRec(sfx_dot    , over_sfx    ? theme->button_hover : theme->button);
    DrawRectangleRec(ui_dot     , over_ui     ? theme->button_hover : theme->button);

    DrawRectangleLinesEx(master_dot , theme->frame_thickness, theme->button_frame);
    DrawRectangleLinesEx(music_dot  , theme->frame_thickness, theme->button_frame);
    DrawRectangleLinesEx(sfx_dot    , theme->frame_thickness, theme->button_frame);
    DrawRectangleLinesEx(ui_dot     , theme->frame_thickness, theme->button_frame);

    #ifndef ANDROID
    draw_button(fullscreen_check, IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) ? "X" : "", cursor, UI_LAYOUT_CENTER, theme);
    #endif
    draw_button(close, "X", cursor, UI_LAYOUT_CENTER, theme);
    return NO;
}
