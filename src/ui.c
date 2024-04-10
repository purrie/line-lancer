#include "ui.h"
#include "input.h"
#include "std.h"
#include "alloc.h"
#include "constants.h"
#include "level.h"
#define CAKE_LAYOUT_IMPLEMENTATION
#include "cake.h"
#include <stdio.h>
#include <raymath.h>
#include <stdio.h>
#include "audio.h"

/* Drawers *******************************************************************/
void draw_button (
    Rectangle     area,
    const char  * text,
    Vector2       cursor,
    UiLayout      label_layout,
    const Theme * theme
) {
    Rectangle text_area;
    int sides_width = theme->assets->button_info.left + theme->assets->button_info.right;
    switch (label_layout) {
        case UI_LAYOUT_LEFT: {
            text_area = cake_carve_to(area, area.width - sides_width, theme->font_size);
            int space = MeasureText(" ", theme->font_size);
            text_area.x += space;
        } break;
        case UI_LAYOUT_CENTER: {
            int label_width = MeasureText(text, theme->font_size);
            text_area = cake_carve_to(area, label_width, theme->font_size);
        } break;
        case UI_LAYOUT_RIGHT: {
            int label_width = MeasureText(text, theme->font_size);
            text_area = cake_carve_to(area, area.width - sides_width, theme->font_size);
            text_area.x += area.width - label_width;
            int space = MeasureText(" ", theme->font_size);
            text_area.x -= space;
        } break;
    }

    if (CheckCollisionPointRec(cursor, area)) {
        DrawTextureNPatch(theme->assets->button, theme->assets->button_info, area, Vector2Zero(), 0, theme->button_hover);
        DrawText(text, text_area.x, text_area.y, theme->font_size, theme->text);
    }
    else {
        DrawTextureNPatch(theme->assets->button, theme->assets->button_info, area, Vector2Zero(), 0, theme->button);
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
    Rectangle button = cake_margin_all(area, theme->margin + theme->frame_thickness);
    Rectangle image  = cake_cut_vertical(&button, -button.height, theme->spacing);
    button = cake_shrink_to(button, theme->font_size);
    Rectangle label      = cake_cut_vertical(&button, cost_offset, theme->spacing);
    Rectangle label_cost = button;

    Color but_color;
    Color tex_color;
    if (! active) {
        but_color = theme->button_inactive;
        tex_color = theme->text_dark;
    }
    else if (CheckCollisionPointRec(cursor, area)) {
        but_color = theme->button_hover;
        tex_color = theme->text;
    }
    else {
        but_color = theme->button;
        tex_color = theme->text_dark;
    }
    DrawTextureNPatch(theme->assets->button, theme->assets->button_info, area, (Vector2){0}, 0, but_color);
    DrawTexturePro(icon, (Rectangle){0, 0, icon.width, icon.height}, image, Vector2Zero(), 0, WHITE);
    DrawText(text, label.x, label.y, theme->font_size, tex_color);
    DrawText(cost, label_cost.x, label_cost.y, theme->font_size, tex_color);
}
void label (Rectangle area, const char * text, float size, UiLayout layout, const Theme * theme) {
    int sides_width = theme->assets->button_info.left + theme->assets->button_info.right;
    int height = theme->assets->button_info.top + theme->assets->button_info.bottom;
    if ((area.height - height) < size) size = area.height - height;

    Rectangle text_area;
    switch (layout) {
        case UI_LAYOUT_LEFT: {
            text_area = cake_carve_to(area, area.width - sides_width - theme->margin * 2, size);
        } break;
        case UI_LAYOUT_CENTER: {
            int label_width = MeasureText(text, size);
            text_area = cake_carve_to(area, label_width, size);
        } break;
        case UI_LAYOUT_RIGHT: {
            int label_width = MeasureText(text, size);
            text_area = cake_carve_to(area, area.width - sides_width - theme->margin * 2, theme->font_size);
            text_area.x += area.width - label_width;
        } break;
    }

    DrawTextureNPatch(theme->assets->button, theme->assets->button_info, area, Vector2Zero(), 0, WHITE);
    DrawText(text, text_area.x, text_area.y, size, theme->text_dark);
}
void slider (Rectangle area, float thumb, int hover, const Theme * theme) {
    Rectangle slider_area = cake_shrink_to(area, theme->assets->slider.height);
    DrawTextureNPatch(theme->assets->slider, theme->assets->slider_info, slider_area, (Vector2){0}, 0, hover ? theme->button_hover : theme->button);

    Rectangle slider_dot = cake_carve_width(area, theme->assets->slider_thumb.width, thumb);
    slider_dot.x += theme->assets->slider_thumb.width * (thumb - 0.5);

    Rectangle source = { 0, 0, theme->assets->slider_thumb.width, theme->assets->slider_thumb.height };
    DrawTexturePro(theme->assets->slider_thumb, source, slider_dot, (Vector2){0}, 0, hover ? theme->button_hover : theme->button);
}
void draw_background(Rectangle area, const Theme * theme) {
    DrawTextureNPatch(theme->assets->background_box, theme->assets->background_box_info, area, (Vector2){0}, 0, WHITE);
}
void draw_title (const Theme * theme) {
    Texture2D title = theme->assets->title;
    DrawTexturePro(title, (Rectangle){0, 0, title.width, title.height}, (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()}, (Vector2){0}, 0, WHITE);
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
    theme->info_bar_field_width = GetScreenWidth() * 0.1f;
    #else
    theme->font_size = 20;
    theme->info_bar_field_width = 200;
    #endif

    theme->margin = 4;
    theme->spacing = 4;
    theme->frame_thickness = 12;
    theme->dialog_width = 350;
    theme->info_bar_height = theme->font_size + theme->margin * 2 + theme->frame_thickness * 3;
    SetTextLineSpacing(theme->font_size * 1.15f);
}
Theme theme_setup () {
    Theme theme = {0};
    theme.text = DARKBLUE;
    theme.text_dark = BLACK;
    theme.button = (Color) { 230, 230, 230, 255 };
    theme.button_inactive = (Color) { 128, 128, 128, 255 };
    theme.button_hover = (Color) { 255, 255, 255, 255 };
    /* theme.button_frame = RAYWHITE; */
    theme_update(&theme);
    return theme;
}

/* In Game Menu **************************************************************/
EmptyDialog empty_dialog (Vector2 position, const Theme * theme) {
    EmptyDialog result;

    float frame = theme->margin + theme->frame_thickness;
    float line_height = theme->font_size + frame * 2;
    float width  = theme->dialog_width + frame * 2;
    float height = line_height * BUILDING_TYPE_COUNT + theme->spacing * BUILDING_TYPE_LAST + frame * 2;

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    #if defined(ANDROID)
    position = (Vector2) { screen.width  - width * 0.5f - theme->info_bar_height,
                           screen.height - height * 0.5f - theme->info_bar_height };
    #endif
    cake_cut_horizontal(&screen, theme->info_bar_height, 0);

    result.area = cake_rect(width, height);
    result.area = cake_center_rect(result.area, position.x, position.y);
    cake_clamp_inside(&result.area, screen);

    Rectangle butt = cake_margin_all(result.area, theme->margin + theme->frame_thickness);
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

    float frame = theme->margin + theme->frame_thickness;
    float line_height = theme->font_size + frame * 2;
    float width  = theme->dialog_width + frame * 2;
    float height = line_height * BUILDING_TYPE_COUNT + theme->spacing * BUILDING_TYPE_LAST + frame * 2;

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    #if defined(ANDROID)
    position = (Vector2) { screen.width  - width  * 0.5f - theme->info_bar_height,
                           screen.height - height * 0.5f - theme->info_bar_height };
    #endif
    cake_cut_horizontal(&screen, theme->info_bar_height, 0);

    result.area = cake_rect(width, height);
    result.area = cake_center_rect(result.area, position.x, position.y);
    cake_clamp_inside(&result.area, screen);

    Rectangle butt = cake_margin_all(result.area, theme->margin + theme->frame_thickness);
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

    draw_background(dialog.area, theme);
    /* DrawRectangleRec(dialog.area, state->settings->theme.background); */

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

    draw_background(dialog.area, theme);
    const char * text = building_name(building->type, faction, building->upgrades);
    const char * lvl;
    switch (state->selected_building->upgrades) {
        case 0: lvl = "Level 1"; break;
        case 1: lvl = "Level 2"; break;
        case 2: lvl = "Level 3"; break;
        default: lvl = "Level N/A"; break;
    }
    usize max_units = building_max_units(building);
    usize now_units = building->units_spawned;
    char counter[20];
    snprintf(counter, 20, "Units: %zu/%zu", now_units, max_units);

    float title_scale = 1.5f;

    /* DrawRectangleRec(dialog.label, theme->background_light); */
    dialog.label = cake_margin_all(dialog.label, theme->margin);

    Rectangle r_title = cake_cut_horizontal(&dialog.label, theme->font_size * title_scale, theme->spacing * 2);
    Rectangle r_level = cake_cut_horizontal(&dialog.label, theme->font_size, theme->spacing);
    Rectangle r_units = cake_cut_horizontal(&dialog.label, theme->font_size, theme->spacing);

    r_title = cake_carve_width(r_title, MeasureText(text, theme->font_size * title_scale), 0.5f);

    DrawText(text, r_title.x, r_title.y, theme->font_size * title_scale, theme->text_dark);
    DrawText(lvl, r_level.x, r_level.y, theme->font_size, theme->text_dark);
    DrawText(counter, r_units.x, r_units.y, theme->font_size, theme->text_dark);

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

        Rectangle button = cake_margin_all(dialog.upgrade, theme->margin + theme->frame_thickness);
        Rectangle image  = cake_cut_vertical(&button, -button.height, theme->spacing);
        button = cake_shrink_to(button, theme->font_size * 2 + theme->spacing * 2);

        Rectangle label      = cake_cut_horizontal(&button, theme->font_size, theme->spacing * 2);
        Rectangle label_cost = button;

        Color but_color;
        Color tex_color;
        if (! active) {
            but_color = theme->button_inactive;
            tex_color = theme->text_dark;
        }
        else if (CheckCollisionPointRec(cursor, dialog.upgrade)) {
            but_color = theme->button_hover;
            tex_color = theme->text;
        }
        else {
            but_color = theme->button;
            tex_color = theme->text_dark;
        }
        DrawTextureNPatch(theme->assets->button, theme->assets->button_info, dialog.upgrade, (Vector2){0}, 0, but_color);
        DrawTexturePro(icon, (Rectangle){0, 0, icon.width, icon.height}, image, Vector2Zero(), 0, WHITE);
        DrawText(u_label, label.x, label.y, theme->font_size, tex_color);
        DrawText(u_cost, label_cost.x, label_cost.y, theme->font_size, tex_color);
    }
    else {
        DrawTextureNPatch(theme->assets->button, theme->assets->button_info, dialog.upgrade, (Vector2){0}, 0, theme->button_inactive);
        dialog.upgrade = cake_carve_to(dialog.upgrade, MeasureText("Max Level", theme->font_size), theme->font_size);
        DrawText("Max Level", dialog.upgrade.x, dialog.upgrade.y, theme->font_size, theme->text_dark);
    }

    draw_button(dialog.demolish, "Demolish", cursor, UI_LAYOUT_CENTER, theme);
}
#if defined(ANDROID)
void render_path_button (const GameState * state) {
    Rectangle area = flag_button_position();

    draw_background(area, &state->settings->theme);
    area = cake_margin_all(area, state->settings->theme.frame_thickness);
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

    Texture2D joystick = theme->assets->joystick;
    Texture2D zoom_in  = theme->assets->zoom_in;
    Texture2D zoom_out = theme->assets->zoom_out;
    DrawTexturePro(joystick, (Rectangle) { 0, 0, joystick.width, joystick.height }, joy.joystick, (Vector2){0}, 0, WHITE);
    DrawTexturePro(zoom_in, (Rectangle) { 0, 0, zoom_in.width, zoom_in.height }, joy.zoom_in, (Vector2){0}, 0, WHITE);
    DrawTexturePro(zoom_out, (Rectangle) { 0, 0, zoom_out.width, zoom_out.height }, joy.zoom_out, (Vector2){0}, 0, WHITE);
}
#endif
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
            snprintf(text, 256, "You lost! Player %zu is the Winner!", winner);
        }
    }
    int width = MeasureText(text, theme->font_size) + theme->margin * 4 + theme->frame_thickness * 4;

    screen = cake_cut_horizontal(&screen, 0.5, 0);
    screen = cake_carve_to(screen, width, theme->font_size * 4.0f);
    draw_background(screen, theme);
    label (cake_margin_all(screen, theme->margin + theme->frame_thickness), text, theme->font_size, UI_LAYOUT_CENTER, theme);

    winner_color.a = 255;
    base_layers /= 2;
    alpha_step = 255 / base_layers;
    for (usize i = 0; i < base_layers; i++) {
        screen = cake_grow_by(screen, 1, 1);
        DrawRectangleLinesEx(screen, 1, winner_color);
        winner_color.a -= alpha_step;
    }
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

    draw_background(bar, theme);
    bar = cake_margin_all(bar, theme->frame_thickness);

    char * menu_label = "Settings";
    char * quit_label = "Exit to Menu";
    float quit_width = MeasureText(quit_label, theme->font_size) + theme->margin * 2 + theme->frame_thickness * 2;
    float menu_width = MeasureText(menu_label, theme->font_size) + theme->margin * 2 + theme->frame_thickness * 2;
    Rectangle quit = cake_cut_vertical(&bar, -quit_width, theme->spacing);
    Rectangle menu = cake_cut_vertical(&bar, -menu_width, theme->spacing);
    Vector2 cursor = GetMousePosition();
    draw_button(quit, quit_label, cursor, UI_LAYOUT_CENTER, theme);
    draw_button(menu, menu_label, cursor, UI_LAYOUT_CENTER, theme);

    bar = cake_shrink_to(bar, theme->font_size);

    Rectangle rect = cake_cut_vertical(&bar, bar.height, theme->spacing);
    Color player_color = get_player_color(player_index);
    DrawRectangleRec(rect, theme->text_dark);
    DrawRectangleRec(cake_margin_all(rect, 1), player_color);

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
    rect = cake_cut_vertical(&bar, label_width, theme->spacing);
    DrawText(label , rect.x, rect.y, theme->font_size, theme->text_dark);

    if (snprintf(buffer, buffer_size, "Income: %.1f/s ", income) <= 0) {
        label = "too much ...";
    }
    else {
        label = buffer;
    }
    label_width = MeasureText(label, theme->font_size);
    if (label_width < theme->info_bar_field_width)
        label_width = theme->info_bar_field_width;
    rect = cake_cut_vertical(&bar, label_width, theme->spacing);
    DrawText(label , rect.x, rect.y, theme->font_size, theme->text_dark);

    if (snprintf(buffer, buffer_size, "Upkeep: %.1f/s ", upkeep) <= 0) {
        label = "too much ...";
    }
    else {
        label = buffer;
    }
    label_width = MeasureText(label, theme->font_size);
    if (label_width < theme->info_bar_field_width)
        label_width = theme->info_bar_field_width;
    rect = cake_cut_vertical(&bar, label_width, theme->spacing);
    DrawText(label , rect.x, rect.y, theme->font_size, theme->text_dark);

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
MainMenuLayout main_menu_layout (const Theme * theme) {
    MainMenuLayout result = {0};

    Rectangle screen = cake_rect(GetScreenWidth(), GetScreenHeight());
    const uint8_t parts_count = 5;
    Rectangle parts[parts_count];
    cake_split_vertical(screen, 3, parts, 0.0f);

    result.title = cake_carve_height(parts[1], theme->font_size * 2 + theme->frame_thickness * 2 + theme->margin * 2, 0.1f);

    cake_split_horizontal(parts[0], 3, parts, 0.0f);

    result.background = cake_diet_to(parts[1], parts[1].width * 0.5);
    cake_split_horizontal(cake_margin_all(result.background, theme->frame_thickness + theme->margin), parts_count, parts, 0.0f);

    result.new_game = cake_carve_height(parts[0], 50.0f, 0.5f);
    result.tutorial = cake_carve_height(parts[1], 50.0f, 0.5f);
    result.manual   = cake_carve_height(parts[2], 50.0f, 0.5f);
    result.options  = cake_carve_height(parts[3], 50.0f, 0.5f);
    result.quit     = cake_carve_height(parts[4], 50.0f, 0.5f);

    return result;
}
void render_simple_map_preview (Rectangle area, Map * map, const Theme * theme) {
    draw_background(area, theme);
    area = cake_margin_all(area, theme->frame_thickness + theme->margin);

    if (map == NULL) {
        DrawLine(area.x, area.y, area.x + area.width, area.y + area.height, theme->text_dark);
        DrawLine(area.x + area.width, area.y, area.x, area.y + area.height, theme->text_dark);
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
        DrawLineEx(from, to, theme->frame_thickness, theme->text_dark);
    }

    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        Vector2 point = Vector2Multiply(region->castle.position, (Vector2){ scale_w, scale_h });
        point = Vector2Add(point, (Vector2){ area.x, area.y });
        Color col = get_player_color(region->player_id);
        DrawCircleV(point, theme->frame_thickness * 2.0f + 1.0f, theme->text_dark);
        DrawCircleV(point, theme->frame_thickness * 2.0f, col);
    }
}
void render_player_select (Rectangle area, GameState * state, int selected_map) {
    if (selected_map < 0) return;
    const Theme * theme = &state->settings->theme;
    const Map * map = &state->resources->maps.items[selected_map];

    draw_background(area, theme);
    area = cake_margin_all(area, theme->frame_thickness);

    float frame = theme->frame_thickness * 2 + theme->margin * 2;

    Rectangle title = cake_cut_horizontal(&area, theme->font_size + frame, 0);
    title = cake_diet_to(title, MeasureText(map->name, theme->font_size) + frame);
    label(title, map->name, theme->font_size, UI_LAYOUT_CENTER, theme);

    area = cake_margin_all(area, theme->frame_thickness);

    Rectangle * lines = temp_alloc(sizeof(Rectangle) * map->player_count);
    Rectangle * player_dropdowns = temp_alloc(sizeof(Rectangle) * map->player_count);
    Rectangle * faction_dropdowns = temp_alloc(sizeof(Rectangle) * map->player_count);

    for (usize i = 0; i < map->player_count; i++) {
        lines[i] = cake_cut_horizontal(&area, theme->font_size + frame, theme->font_size * 0.5f);
    }

    Vector2 mouse = GetMousePosition();

    char name[64];
    char * player_control = "Controlling: ";
    char * player_faction = "Faction: ";
    int player_control_size = MeasureText(player_control, theme->font_size);
    int player_faction_size = MeasureText(player_faction, theme->font_size);

    static int selected_player = -1;
    static int choosing = -1; // 0 - PC/CPU, 1 - Faction

    Texture2D drop_button = theme->assets->button;
    Texture2D drop_arrow  = theme->assets->drop;
    NPatchInfo drop_patch = theme->assets->button_info;
    NPatchInfo drop_arrow_patch = drop_patch;
    drop_patch.source.width -= drop_patch.right;
    drop_patch.right = 0;
    drop_arrow_patch.source = (Rectangle) {0, 0, drop_arrow.width, drop_arrow.height};
    drop_arrow_patch.left = 0;

    for (usize i = 0; i < map->player_count; i++) {
        PlayerData * player = &state->players.items[i + 1];
        DrawRectangleRec(lines[i], (Color) { 255, 255, 255, 32 });

        snprintf(name, 64, "Player %zu", i + 1);
        Rectangle label = cake_cut_vertical(&lines[i], 0.3f, 5);
        cake_cut_vertical(&label, theme->frame_thickness * 2, 0);
        Rectangle color_box = cake_cut_vertical(&label, theme->font_size, theme->frame_thickness * 2);
        color_box = cake_shrink_to(color_box, color_box.width);
        DrawRectangleRec(color_box, BLACK);
        DrawRectangleRec(cake_margin_all(color_box, 1), get_player_color(i + 1));
        label = cake_shrink_to(label, theme->font_size);
        DrawText(name, label.x, label.y, theme->font_size, theme->text_dark);

        player_dropdowns[i] = cake_cut_vertical(&lines[i], 0.5f, 5);
        faction_dropdowns[i] = lines[i];

        // select who controls this player
        label = cake_cut_vertical(&player_dropdowns[i], player_control_size, 6);
        label = cake_shrink_to(label, theme->font_size);
        DrawText(player_control, label.x, label.y, theme->font_size, theme->text_dark);

        int mouse_over = selected_player >= 0 ? 0 : CheckCollisionPointRec(mouse, player_dropdowns[i]);
        Rectangle select_player = player_dropdowns[i];
        Rectangle drop_area = cake_cut_vertical(&select_player, -select_player.height, 0);
        DrawTextureNPatch(drop_button, drop_patch, select_player, (Vector2){0}, 0, mouse_over ? theme->button_hover : theme->button);
        DrawTextureNPatch(drop_arrow, drop_arrow_patch, drop_area, (Vector2){0}, 0, mouse_over ? theme->button_hover : theme->button);

        char * player_control_label = player->type == PLAYER_LOCAL ? "Player" : "CPU";
        int player_control_label_size = MeasureText(player_control_label, theme->font_size);
        label = cake_carve_to(select_player, player_control_label_size, theme->font_size);
        DrawText(player_control_label, label.x, label.y, theme->font_size, mouse_over ? theme->text : theme->text_dark);

        // select faction of this player
        label = cake_cut_vertical(&faction_dropdowns[i], player_faction_size, 6);
        label = cake_shrink_to(label, theme->font_size);
        DrawText(player_faction, label.x, label.y, theme->font_size, theme->text_dark);

        mouse_over = selected_player >= 0 ? 0 : CheckCollisionPointRec(mouse, faction_dropdowns[i]);
        select_player = faction_dropdowns[i];
        drop_area = cake_cut_vertical(&select_player, -select_player.height, 0);
        DrawTextureNPatch(drop_button, drop_patch, select_player, (Vector2){0}, 0, mouse_over ? theme->button_hover : theme->button);
        DrawTextureNPatch(drop_arrow, drop_arrow_patch, drop_area, (Vector2){0}, 0, mouse_over ? theme->button_hover : theme->button);

        char * faction_name = faction_to_string(state->players.items[i + 1].faction);
        int faction_name_size = MeasureText(faction_name, theme->font_size);
        label = cake_carve_to(select_player, faction_name_size, theme->font_size);
        DrawText(faction_name, label.x, label.y, theme->font_size, mouse_over ? theme->text : theme->text_dark);
    }

    if (selected_player >= 0) {
        if (choosing) {
            Rectangle dropdown = faction_dropdowns[selected_player];
            dropdown.height = ( theme->font_size + frame ) * FACTION_COUNT + theme->font_size * (FACTION_LAST * 0.5f) + theme->frame_thickness * 2;
            draw_background(dropdown, theme);
            dropdown = cake_margin_all(dropdown, theme->frame_thickness);

            int clicked = 0;
            for (usize i = 0; i <= FACTION_LAST; i++) {
                Rectangle rect = cake_cut_horizontal(&dropdown, theme->font_size + frame, theme->font_size * 0.5f);
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

                draw_button(rect, faction_to_string(i), mouse, UI_LAYOUT_CENTER, theme);
                /* rect = cake_carve_to(rect, MeasureText(faction_to_string(i), theme->font_size), theme->font_size); */
                /* DrawText(faction_to_string(i), rect.x, rect.y, theme->font_size, mouse_over ? theme->text : theme->text_dark); */
            }
            if (clicked) {
                selected_player = -1;
                return;
            }
        }
        else {
            Rectangle dropdown = player_dropdowns[selected_player];
            dropdown.height = ( theme->font_size + frame) * 2 + theme->font_size * 0.5f + theme->frame_thickness * 2;
            draw_background(dropdown, theme);
            dropdown = cake_margin_all(dropdown, theme->frame_thickness);
            Rectangle pc_rect = cake_cut_horizontal(&dropdown, theme->font_size + frame, theme->font_size * 0.5f);

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
            draw_button(pc_rect, "Player", mouse, UI_LAYOUT_CENTER, theme);
            draw_button(dropdown, "CPU", mouse, UI_LAYOUT_CENTER, theme);
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
    draw_background(area, theme);

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

    area = cake_margin_all(area, theme->frame_thickness + theme->margin);
    cake_layers(area, len, boxes, theme->font_size + theme->margin + theme->frame_thickness, theme->spacing);
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

    float frame = theme->frame_thickness * 2 + theme->margin * 2;
    float line_height = theme->font_size + frame;

    char * top_label_text = "Settings";
    char * master_label = "Global Volume: ";
    char * music_label = "Music Volume: ";
    char * sfx_label = "SFX Volume: ";
    char * ui_label = "UI Volume: ";
    #ifndef ANDROID
    char * fullscreen = "Fullscreen ";
    #endif

    Rectangle center = cake_carve_to(area, area.width * 0.8, line_height * 10);
    draw_background(center, theme);
    center = cake_margin_all(center, theme->frame_thickness);

    Rectangle close = cake_carve_width(center, theme->font_size * 1.5f, 1.0f);
    close.height = close.width;

    center = cake_margin_all(center, theme->margin);

    Rectangle top_label = cake_cut_horizontal(&center, theme->font_size * 2 + frame, theme->font_size);

    top_label = cake_carve_width(top_label, MeasureText(top_label_text, theme->font_size * 2) + frame, 0.5f);

    Rectangle master_volume_slider = cake_cut_horizontal(&center, line_height, theme->font_size);
    Rectangle music_volume_slider  = cake_cut_horizontal(&center, line_height, theme->font_size);
    Rectangle sfx_volume_slider    = cake_cut_horizontal(&center, line_height, theme->font_size);
    Rectangle ui_volume_slider     = cake_cut_horizontal(&center, line_height, theme->font_size);
    #ifndef ANDROID
    Rectangle fullscreen_check     = cake_cut_horizontal(&center, line_height, theme->font_size);
    #endif

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
        labels_width += frame;
    }

    Rectangle master_volume_label = cake_cut_vertical(&master_volume_slider , labels_width, theme->spacing);
    Rectangle music_volume_label  = cake_cut_vertical(&music_volume_slider  , labels_width, theme->spacing);
    Rectangle sfx_volume_label    = cake_cut_vertical(&sfx_volume_slider    , labels_width, theme->spacing);
    Rectangle ui_volume_label     = cake_cut_vertical(&ui_volume_slider     , labels_width, theme->spacing);
    #ifndef ANDROID
    Rectangle fullscreen_label    = cake_cut_vertical(&fullscreen_check     , labels_width, theme->spacing);
    fullscreen_check.width = fullscreen_check.height;
    #endif

    Vector2 cursor = GetMousePosition();
    int over_master = CheckCollisionPointRec(cursor, master_volume_slider);
    int over_music  = CheckCollisionPointRec(cursor, music_volume_slider);
    int over_sfx    = CheckCollisionPointRec(cursor, sfx_volume_slider);
    int over_ui     = CheckCollisionPointRec(cursor, ui_volume_slider);
    int over_close  = CheckCollisionPointRec(cursor, close);

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

    label(top_label, top_label_text, theme->font_size * 2, UI_LAYOUT_CENTER, theme);

    label(master_volume_label , master_label, theme->font_size, UI_LAYOUT_CENTER, theme);
    label(music_volume_label  , music_label , theme->font_size, UI_LAYOUT_CENTER, theme);
    label(sfx_volume_label    , sfx_label   , theme->font_size, UI_LAYOUT_CENTER, theme);
    label(ui_volume_label     , ui_label    , theme->font_size, UI_LAYOUT_CENTER, theme);
    #ifndef ANDROID
    label(fullscreen_label    , fullscreen  , theme->font_size, UI_LAYOUT_CENTER, theme);
    #endif

    slider(master_volume_slider, settings->volume_master, over_master, theme);
    slider(music_volume_slider, settings->volume_music, over_music, theme);
    slider(sfx_volume_slider, settings->volume_sfx, over_sfx, theme);
    slider(ui_volume_slider, settings->volume_ui, over_ui, theme);

    #ifndef ANDROID
    draw_button(fullscreen_check, IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) ? "X" : "", cursor, UI_LAYOUT_CENTER, theme);
    #endif
    Texture2D close_texture;
    Rectangle close_rectangle;
    Color     close_color;
    if (over_close) {
        close_texture   = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? assets->ui.close_press : assets->ui.close;
        close_rectangle = (Rectangle) {0, 0, close_texture.width, close_texture.height};
        close_color     = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? theme->button : theme->button_hover;
    }
    else {
        close_texture   = assets->ui.close;
        close_rectangle = (Rectangle) {0, 0, close_texture.width, close_texture.height};
        close_color     = theme->button;
    }
    DrawTexturePro(close_texture, close_rectangle, close, (Vector2){0}, 0, close_color);
    return NO;
}
