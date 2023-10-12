#ifndef PARTICLE_H_
#define PARTICLE_H_

#include "types.h"

/* Animations ****************************************************************/
float animation_curve_evaluate (AnimationCurve curve, float point);

/* Particles *****************************************************************/
void particles_blood (GameState * state, Unit * unit, Attack attack);
void particles_render (Particle ** particles, usize len);
void particles_render_attacks (const GameState * state, Unit * attacked);
void particles_advance (Particle ** particles, usize len, float delta_time);
void particles_clean (GameState * state);


#endif // PARTICLE_H_
