#ifndef PARTICLE_H_
#define PARTICLE_H_

#include "types.h"

/* Animations ****************************************************************/
float animation_curve_evaluate (AnimationCurve curve, float point);

/* Particles *****************************************************************/
void particles_render (const Particle ** particles, usize len);
void particles_render_attacks (const GameState * state, Unit * attacked);
void particles_advance (Particle ** particles, usize len, float delta_time);


#endif // PARTICLE_H_
