#include "particle.h"
#include "constants.h"
#include <raymath.h>

/* Animations ****************************************************************/
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

/* Particles *****************************************************************/
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
void particles_render (const Particle ** particles, usize len) {
    for (usize i = 0; i < len; i++) {
        const Particle * particle = particles[i];
        float t = particle->time_lived / particle->lifetime;

        float rotation = animation_curve_evaluate(particle->rotation_curve, t);
        float scale = animation_curve_evaluate(particle->scale_curve, t);

        float t_color = animation_curve_evaluate(particle->color_curve, t);
        Color color = particle->color_end;
        color.a = (char)( t_color * 255 );
        color = ColorAlphaBlend(color, particle->color_start, WHITE);
        color.a = (char)( 255 * animation_curve_evaluate(particle->alpha_curve, t) );

        if (particle->sprite) {
            DrawTextureEx(*particle->sprite, particle->position, rotation, scale, color);
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
