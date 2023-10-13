#include "particle.h"
#include "std.h"
#include "constants.h"
#include <raymath.h>

/* Animation Curves ****************************************************************/
float animation_curve_evaluate (AnimationCurve curve, float point) {
    float npoint = 1.0f - point;
    float result = powf(npoint, 3) * curve.start.y +
        3.0f * powf(npoint, 2.0f) * point * curve.start_handle.y +
        3.0f * npoint * powf(point, 2.0f) * curve.end_handle.y +
        powf(point, 3.0f) * curve.end.y;
    return result;
}
Vector2 animation_curve_position (AnimationCurve curve, float point) {
    float npoint = 1.0f - point;
    float result_x = powf(npoint, 3) * curve.start.x +
        3.0f * powf(npoint, 2.0f) * point * curve.start_handle.x +
        3.0f * npoint * powf(point, 2.0f) * curve.end_handle.x +
        powf(point, 3.0f) * curve.end.x;
    float result_y = powf(npoint, 3) * curve.start.y +
        3.0f * powf(npoint, 2.0f) * point * curve.start_handle.y +
        3.0f * npoint * powf(point, 2.0f) * curve.end_handle.y +
        powf(point, 3.0f) * curve.end.y;
    return (Vector2){ result_x, result_y };
}

AnimationCurve curve_constant (float value) {
    return (AnimationCurve){
        .start = { 0.0f, value },
        .start_handle = { 0.25f, value },
        .end_handle = { 0.75f, value },
        .end = { 1.0f, value }
    };
}
AnimationCurve curve_bell () {
    return (AnimationCurve){
        .start = { 0.0f, 0.0f },
        .start_handle = { 0.25f, 1.0f },
        .end_handle = { 0.75f, 1.0f },
        .end = { 1.0f, 0.0f }
    };
}
AnimationCurve curve_smooth(float start, float end) {
    return (AnimationCurve){
        .start = { 0.0f, start },
        .start_handle = { 0.25f, start },
        .end_handle = { 0.75f, end },
        .end = { 1.0f, end }
    };
}
AnimationCurve curve_linear(float start, float end) {
    return (AnimationCurve){
        .start = { 0.0f, start },
        .start_handle = { 0.25f, start * 0.75f + end * 0.25f },
        .end_handle = { 0.75f, end * 0.75f + start * 0.25f },
        .end = { 1.0f, end }
    };
}
AnimationCurve curve_timeline (float start, float start_handle, float end_handle, float end) {
    return (AnimationCurve){
        .start = { 0.0f, start },
        .start_handle = { 0.25f, start_handle },
        .end_handle = { 0.75f, end_handle },
        .end = { 1.0f, end }
    };
}

/* Particles *****************************************************************/
void particles_blood (GameState * state, Unit * attacked, Attack attack) {
    usize amount;
    switch (attacked->type) {
        case UNIT_GUARDIAN: {
            amount = attacked->health > 0.0f ? GetRandomValue(2, 8) : GetRandomValue(4, 20);
        } break;
        default: {
            amount = attacked->health > 0.0f ? GetRandomValue(3, 10) : GetRandomValue(10, 20);
        } break;
    }
    Vector2 direction;
    switch (attack.attacker_faction) {
        case FACTION_KNIGHTS: switch (attack.attacker_type) {
            case UNIT_ARCHER:
            case UNIT_GUARDIAN: goto arc_unit;
            default: goto straight_unit;
        }
        case FACTION_MAGES: goto straight_unit;
    }

    if (0) {
        straight_unit:
        direction = Vector2Normalize(Vector2Subtract(attack.origin_position, attacked->position));
    }
    if (0) {
        arc_unit:
        direction = (Vector2){ attack.origin_position.x > attacked->position.x ? 0.3f : -0.3f, -0.7f };
    }

    for (usize i = 0; i < amount; i++) {
        if (state->particles_available.len == 0) {
            TraceLog(LOG_WARNING, "Ran out of particles");
            break;
        }
        state->particles_available.len --;
        Particle * particle = state->particles_available.items[state->particles_available.len];
        clear_memory(particle, sizeof(Particle));

        particle->lifetime = GetRandomValue(20, 60) * 0.01f;

        switch (attacked->type) {
            case UNIT_GUARDIAN: switch (attacked->faction) {
                case FACTION_KNIGHTS: {
                    unsigned char col = GetRandomValue(132, 164);
                    particle->color_start = (Color){ col, col, col, 255 };
                    particle->color_end = (Color){ 92, 92, 92, 255 };
                } break;
                case FACTION_MAGES: {
                    unsigned char col = GetRandomValue(132, 164);
                    particle->color_start = (Color){ GetRandomValue(152, 192), col, col, 255 };
                    particle->color_end = (Color){ 92, 92, 92, 255 };
                } break;
            } break;
            default: {
                particle->color_start = (Color){ GetRandomValue(192, 255), 32, 16, 255 };
                particle->color_end = (Color){ 128, 64, 32, 255 };
            } break;
        }
        particle->color_curve = curve_timeline(0.0f, 0.0f, 0.2f, 1.0f);
        particle->alpha_curve = curve_constant(1.0f);

        particle->position = Vector2Add(attacked->position, (Vector2){ GetRandomValue(-2, 2), GetRandomValue(-2, 2)});

        float angle = attacked->health > 0.0f ? GetRandomValue(-20, 20) : GetRandomValue(0, 360);
        particle->velocity = Vector2Rotate(direction, angle * DEG2RAD);
        particle->velocity_curve = curve_timeline(1.0f, 1.0f, 0.5f, 0.0f);

        float rotation = GetRandomValue(-80, 80);
        particle->rotation_curve = curve_linear(0.0f, rotation);

        particle->scale_curve = curve_timeline(2.0f, 1.5f, 1.5f, 1.0f);
        listParticleAppend(&state->particles_in_use, particle);
    }
}
void particles_magic (GameState * state, Unit * caster, Unit * target) {
    // caster particle
    {
        if (state->particles_available.len == 0) return;
        state->particles_available.len --;
        Particle * particle = state->particles_available.items[state->particles_available.len];
        clear_memory(particle, sizeof(Particle));

        particle->lifetime = 1.0f;
        particle->color_start = particle->color_end = WHITE;
        particle->color_curve = curve_constant(0.0f);
        particle->alpha_curve = curve_bell();
        particle->rotation_curve = curve_constant(0.0f);
        particle->scale_curve = curve_smooth(2.0f, 8.0f);
        particle->position = caster->position;
        particle->velocity = (Vector2){ 0.0f, -0.1f};
        particle->velocity_curve = curve_constant(1.0f);

        switch(caster->faction) {
            case FACTION_KNIGHTS: {
                particle->sprite = (Texture2D*)&state->resources->particles[PARTICLE_PLUS];
            } break;
            case FACTION_MAGES: {
                particle->sprite = (Texture2D*)&state->resources->particles[PARTICLE_TORNADO];
            } break;
        }
        listParticleAppend(&state->particles_in_use, particle);
    }

    usize amount;
    switch (caster->faction) {
        case FACTION_KNIGHTS: amount = GetRandomValue(3, 5); break;
        case FACTION_MAGES: amount = GetRandomValue(3, 5); break;
    }

    // magic effect particle on the target
    while (amount --> 0) {
        if (state->particles_available.len == 0) break;
        state->particles_available.len --;
        Particle * particle = state->particles_available.items[state->particles_available.len];
        clear_memory(particle, sizeof(Particle));

        particle->lifetime = 1.0f;
        particle->color_start = particle->color_end = WHITE;
        particle->color_curve = curve_constant(0.0f);
        particle->alpha_curve = curve_bell();
        particle->rotation_curve = curve_constant(0.0f);
        particle->scale_curve = curve_smooth(2.0f, 4.0f);

        switch(caster->faction) {
            case FACTION_KNIGHTS: {
                particle->position = (Vector2){ GetRandomValue(-UNIT_SIZE * 50, UNIT_SIZE * 50) * 0.01f, GetRandomValue(-UNIT_SIZE * 75, -UNIT_SIZE * 100) * 0.01f };
                particle->position = Vector2Add(particle->position, target->position);
                particle->velocity = (Vector2){ 0.0f, 0.2f};
                particle->velocity_curve = curve_smooth(0.0f, 1.0f);
                particle->sprite = (Texture2D*)&state->resources->particles[PARTICLE_PLUS];
            } break;
            case FACTION_MAGES: {
                particle->position = (Vector2){ GetRandomValue(-UNIT_SIZE * 10, UNIT_SIZE * 10) * 0.01f, GetRandomValue(UNIT_SIZE * 25, UNIT_SIZE * 50) * 0.01f };
                particle->position = Vector2Add(particle->position, target->position);
                particle->velocity = (Vector2){ GetRandomValue(-10, 10) * 0.1f, GetRandomValue(-20, -10) * 0.1f };
                particle->velocity_curve = curve_constant(1.0f);
                particle->sprite = (Texture2D*)&state->resources->particles[PARTICLE_TORNADO];
            } break;
        }

        listParticleAppend(&state->particles_in_use, particle);
    }
}
void particles_clean (GameState * state) {
    if (state->particles_in_use.len == 0) return;

    usize i = state->particles_in_use.len;
    while (i --> 0) {
        Particle * particle = state->particles_in_use.items[i];
        if (particle->lifetime < particle->time_lived) {
            listParticleRemove(&state->particles_in_use, i);
            listParticleAppend(&state->particles_available, particle);
        }
    }
}
void particles_advance (Particle ** particles, usize len, float delta_time) {
    for (usize i = 0; i < len; i++) {
        Particle * particle = particles[i];

        particle->time_lived += delta_time;
        if (particle->time_lived > particle->lifetime) continue;

        float t = particle->time_lived / particle->lifetime;
        float velocity = animation_curve_evaluate(particle->velocity_curve, t);
        if (! FloatEquals(velocity, 0.0f)) {
            Vector2 diff = Vector2Scale(particle->velocity, velocity);
            particle->position = Vector2Add(particle->position, diff);
        }
    }
}
void particles_render (Particle ** particles, usize len) {
    for (usize i = 0; i < len; i++) {
        const Particle * particle = particles[i];
        float t = particle->time_lived / particle->lifetime;

        float rotation = animation_curve_evaluate(particle->rotation_curve, t);
        float scale = animation_curve_evaluate(particle->scale_curve, t);

        float t_color = animation_curve_evaluate(particle->color_curve, t);
        Color color = particle->color_end;
        color.a = (unsigned char)( t_color * 255 );
        color = ColorAlphaBlend(particle->color_start, color, WHITE);
        color.a = (unsigned char)( 255 * animation_curve_evaluate(particle->alpha_curve, t) );

        if (particle->sprite) {
            Rectangle sprite_rect = (Rectangle){ 0, 0, particle->sprite->width, particle->sprite->height };
            Vector2 origin = (Vector2){ 0.5f * scale, 0.5f * scale };
            Rectangle world_rect = (Rectangle) { particle->position.x, particle->position.y, scale, scale };
            DrawTexturePro(*particle->sprite, sprite_rect, world_rect, origin, rotation, color);
        }
        else {
            Rectangle rect = {
                .x = particle->position.x,
                .y = particle->position.y,
                .width = scale,
                .height = scale,
            };
            DrawRectanglePro(rect, (Vector2){0}, rotation, color);
        }
    }
}
void particles_render_attacks (const GameState * state, Unit * attacked) {
    if (attacked->incoming_attacks.len == 0)
        return;

    for (usize i = 0; i < attacked->incoming_attacks.len; i++) {
        Attack * attack = &attacked->incoming_attacks.items[i];
        float t = attack->timer / attack->delay;

        Vector2 attack_position;
        float attack_rotation;
        ParticleType attack_type;

        switch (attack->attacker_faction) {
            case FACTION_KNIGHTS: {
                switch (attack->attacker_type) {
                    case UNIT_FIGHTER: attack_type = PARTICLE_SLASH; goto straight;
                    case UNIT_ARCHER:
                    case UNIT_GUARDIAN: attack_type = PARTICLE_ARROW; goto arc;
                    case UNIT_SUPPORT: attack_type = PARTICLE_SYMBOL; goto straight;
                    case UNIT_SPECIAL: attack_type = PARTICLE_SLASH_2; goto straight;
                }
            } break;
            case FACTION_MAGES: {
                switch (attack->attacker_type) {
                    case UNIT_FIGHTER: attack_type = PARTICLE_FIST; goto straight;
                    case UNIT_ARCHER:
                    case UNIT_GUARDIAN: attack_type = PARTICLE_FIREBALL; goto straight;
                    case UNIT_SUPPORT: attack_type = PARTICLE_TORNADO; goto straight;
                    case UNIT_SPECIAL: attack_type = PARTICLE_THUNDERBOLT; goto straight;
                }
            } break;
        }

        if (0) {
            arc:
            attack_position = Vector2Lerp(attack->origin_position, attacked->position, 0.5);
            AnimationCurve curve = {
                .start = attack->origin_position,
                .start_handle = Vector2Add(attack_position, (Vector2){ 0.0f, -UNIT_SIZE * 2.0f }),
                .end_handle = Vector2Add(attacked->position, (Vector2){0.0f, -UNIT_SIZE * 2.0f }),
                .end = attacked->position,
            };
            attack_position = animation_curve_position(curve, t);
            attack_rotation = Vector2Angle(
                (Vector2){ attacked->position.x < attack->origin_position.x ? 1.0f : -1.0f , 0.0f },
                Vector2Normalize(Vector2Subtract(animation_curve_position(curve, t + GetFrameTime()), attack_position))
            );
        }
        if (0) {
            straight:
            attack_position = Vector2Lerp(attack->origin_position, attacked->position, t);
            attack_rotation = Vector2Angle((Vector2) { 1, 0 }, Vector2Subtract(attacked->position, attack->origin_position));
        }
        attack_rotation *= RAD2DEG;

        Rectangle rec = (Rectangle){ attack_position.x - 2, attack_position.y - 2, 4, 4};
        Texture2D sprite = state->resources->particles[attack_type];

        DrawTexturePro(sprite, (Rectangle){ 0, 0, sprite.width, sprite.height }, rec, (Vector2){2,2}, attack_rotation, WHITE);
    }
}
void particles_render_effects (const GameState * state, Unit * unit) {
    usize i = unit->effects.len;
    while (i --> 0) {
        MagicEffect * effect = &unit->effects.items[i];
        isize frame = (isize)(state->turn) % 100;

        switch (effect->type) {
            case MAGIC_HEALING: {
                isize actual_frame = frame >= 50 ? 100 - frame : frame;
                float scale = 3.0f + actual_frame * 0.025f;
                float half_scale = scale * 0.5f;
                Rectangle target = (Rectangle){unit->position.x, unit->position.y, scale, scale};

                Texture2D sprite = state->resources->particles[PARTICLE_PLUS];
                Rectangle source = (Rectangle){0, 0, sprite.width, sprite.height};
                DrawTexturePro(sprite, source, target, (Vector2){half_scale, half_scale}, 0.0f, WHITE);
            } break;
            case MAGIC_WEAKNESS: {
                isize actual_frame = frame >= 50 ? 100 - frame : frame;
                float horizontal = ( actual_frame - 25 ) * 0.1f;
                float vertical = (actual_frame >= 25 ? 50 - actual_frame : actual_frame) * (frame >= 50 ? 0.1f : -0.1f);

                Texture2D sprite = state->resources->particles[PARTICLE_TORNADO];
                float size = UNIT_SIZE * 0.5f;
                float half_size = size * 0.5f;
                Rectangle target = (Rectangle){horizontal + unit->position.x, vertical + unit->position.y, size, size};
                Rectangle source = (Rectangle){0, 0, sprite.width, sprite.height};
                DrawTexturePro(sprite, source, target, (Vector2){half_size, half_size}, 0.0f, WHITE);
            } break;
            default: break;
            // TODO others not implemented yet until they're in the game
        }

    }
}
