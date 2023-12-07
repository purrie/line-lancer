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

Result animate_loop (ListFrame animation, float time, Rectangle * result) {
    if (animation.len == 0) {
        return FAILURE;
    }
    float animation_length = 0;
    for (usize i = 0; i < animation.len; i++) {
        animation_length += animation.items[i].duration;
    }
    float animation_time = time;
    while (animation_time > animation_length) animation_time -= animation_length;

    float counter = 0;
    for (usize i = 0; i < animation.len; i++) {
        counter += animation.items[i].duration;
        if (counter >= animation_time) {
            *result = animation.items[i].source;
            return SUCCESS;
        }
    }
    TraceLog(LOG_ERROR, "Couldn't loop animation correctly");
    return FAILURE;
}
Result animate_single (ListFrame animation, float time, Rectangle * result) {
    if (animation.len == 0) {
        return FAILURE;
    }
    float animation_time = time;

    float counter = 0;
    for (usize i = 0; i < animation.len; i++) {
        counter += animation.items[i].duration;
        if (counter >= animation_time) {
            *result = animation.items[i].source;
            return SUCCESS;
        }
    }
    *result = animation.items[animation.len - 1].source;
    return SUCCESS;
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

    Rectangle source;
    switch (unit->state) {
        case UNIT_STATE_IDLE:
            if (animate_loop(set.idle, unit->state_time, &source)) {
                TraceLog(LOG_DEBUG, "Failed to animate unit's idle state");
                render_debug_unit(game, unit);
                return;
            }
            break;
        case UNIT_STATE_CHASING:
        case UNIT_STATE_MOVING:
            if (animate_loop(set.walk, unit->state_time, &source)) {
                TraceLog(LOG_DEBUG, "Failed to animate unit's walk state");
                render_debug_unit(game, unit);
                return;
            }
            break;
        case UNIT_STATE_FIGHTING:
            if (animate_single(set.attack, unit->state_time, &source)) {
                TraceLog(LOG_DEBUG, "Failed to animate unit's attack");
                render_debug_unit(game, unit);
                return;
            }
            break;
        case UNIT_STATE_SUPPORTING:
            if (animate_single(set.cast, unit->state_time, &source)) {
                TraceLog(LOG_DEBUG, "Failed to animate unit's spellcasting");
                render_debug_unit(game, unit);
                return;
            }
            break;
        case UNIT_STATE_GUARDING:
            TraceLog(LOG_ERROR, "Can't animate guardians");
            return;
    }

    Rectangle target = {
        .x = unit->position.x,
        .y = unit->position.y,
        .width = NAV_GRID_SIZE,
        .height = NAV_GRID_SIZE,
    };
    Vector2 origin = { target.width * 0.5f, target.height * 0.5f };

    float angle = Vector2AngleHorizon(unit->facing_direction) * RAD2DEG;
    if (angle < 0) angle = -angle;
    if (angle > 90) {
        source.x += source.width;
        source.width = -source.width;
    }
    Color outline = get_player_color(unit->player_owned);

    DrawTexturePro(set.sprite_sheet, source, target, origin, 0, outline);
}
