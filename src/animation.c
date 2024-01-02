#include "animation.h"
#include "game.h"
#include "math.h"
#include <raymath.h>

void render_debug_unit (const GameState * game, const Unit * unit) {
    EndShaderMode();
    Color player = get_player_color(unit->player_owned);
    Color unit_type;
    switch (unit->type) {
        case UNIT_FIGHTER: unit_type = RED; break;
        case UNIT_ARCHER: unit_type = YELLOW; break;
        case UNIT_SUPPORT: unit_type = BLUE; break;
        case UNIT_SPECIAL: unit_type = PURPLE; break;
        default: unit_type = DARKBROWN; break;
    }

    DrawCircleV(unit->position, 7.0f, player);
    DrawCircleV(unit->position, 5.0f, BLACK);
    DrawCircleV(unit->position, 4.0f, unit_type);
    const char * label;
    switch (unit->upgrade) {
        case 0: label = "1"; break;
        case 1: label = "2"; break;
        case 2: label = "3"; break;
        default: label = "N/A"; break;
    }
    DrawText(label, unit->position.x - 1, unit->position.y - 4, 6, BLACK);

    BeginShaderMode(game->resources->outline_shader);
}

void animate_unit (const GameState * game, const Unit * unit) {
    if (unit->type >= UNIT_TYPE_COUNT) {
        TraceLog(LOG_ERROR, "Tried to animate units that can't be animated");
        return;
    }
    AnimationSet set = game->resources->animations.sets[unit->faction][unit->type][unit->upgrade];
    if (set.sprite_sheet.format == 0) {
        // we lack sprite sheet, fallback to debug rendering for now
        render_debug_unit(game, unit);
        return;
    }

    float time_left = unit->state_time;
    uint8_t index;
    switch (unit->state) {
        case UNIT_STATE_IDLE: {
            index = set.idle_start;
            while (time_left > set.idle_duration) time_left -= set.idle_duration;
        } break;
        case UNIT_STATE_CHASING:
        case UNIT_STATE_MOVING: {
            index = set.walk_start;
            while (time_left > set.walk_duration) time_left -= set.walk_duration;
        } break;
        case UNIT_STATE_FIGHTING: {
            if (time_left >= set.attack_duration) {
                index = set.idle_start;
                time_left -= set.attack_duration;
                while (time_left > set.idle_duration) time_left -= set.idle_duration;
            }
            else {
                index = set.attack_start;
            }
        } break;
        case UNIT_STATE_SUPPORTING: {
            if (time_left >= set.cast_duration) {
                index = set.idle_start;
                time_left -= set.cast_duration;
                while (time_left > set.idle_duration) time_left -= set.idle_duration;
            }
            else {
                index = set.cast_start;
            }
        } break;
        case UNIT_STATE_GUARDING:
            TraceLog(LOG_ERROR, "Can't animate guardians");
            return;
    }
    while (time_left > set.frames.items[index].duration) {
        time_left -= set.frames.items[index].duration;
        index++;
    }
    Rectangle source = set.frames.items[index].source;

    Rectangle target = {
        .x = unit->position.x,
        .y = unit->position.y,
        .width = NAV_GRID_SIZE,
        .height = NAV_GRID_SIZE,
    };
    Vector2 origin = { target.width * 0.5f, target.height * 0.5f };

    float angle = Vector2AngleHorizon(unit->facing_direction) * RAD2DEG;

    if (angle < 0)  angle = -angle;
    if (angle > 90) source.width = -source.width;

    Color outline = get_player_color(unit->player_owned);

    DrawTexturePro(set.sprite_sheet, source, target, origin, 0, outline);
}
