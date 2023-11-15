#ifndef AUDIO_H_
#define AUDIO_H_

#include "types.h"

/* Direct sound handling *****************************************************/
void play_sound           (const Assets * assets, SoundEffectType sound);
void play_sound_inworld   (const GameState * game, SoundEffectType kind, Vector2 position);
void play_sound_concurent (GameState * game, SoundEffectType kind, Vector2 position);
void stop_sounds          (ListSFX * sounds);
void clean_sounds         (GameState * sounds);

/* Complex sound handling ****************************************************/
void play_unit_attack_sound       (GameState * game, const Unit * unit);
void play_unit_hurt_sound         (GameState * game, const Unit * unit);

#endif // AUDIO_H_
