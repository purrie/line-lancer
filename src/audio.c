#include "audio.h"
#include <raylib.h>
#include <raymath.h>

char * sound_kind_name (SoundEffectType kind) {
    switch (kind) {
    case SOUND_HURT_HUMAN: return "SOUND_HURT_HUMAN";
    case SOUND_HURT_HUMAN_OLD: return "SOUND_HURT_HUMAN_OLD";
    case SOUND_HURT_KNIGHT: return "SOUND_HURT_KNIGHT";
    case SOUND_HURT_GOLEM: return "SOUND_HURT_GOLEM";
    case SOUND_HURT_GREMLIN: return "SOUND_HURT_GREMLIN";
    case SOUND_HURT_GENIE: return "SOUND_HURT_GENIE";
    case SOUND_HURT_CASTLE: return "SOUND_HURT_CASLTE";

    case SOUND_ATTACK_SWORD: return "SOUND_ATTACK_SWORD";
    case SOUND_ATTACK_BOW: return "SOUND_ATTACK_BOW";
    case SOUND_ATTACK_HOLY: return "SOUND_ATTACK_HOLY";
    case SOUND_ATTACK_KNIGHT: return "SOUND_ATTACK_KNIGHT";
    case SOUND_ATTACK_GOLEM: return "SOUND_ATTACK_GOLEM";
    case SOUND_ATTACK_FIREBALL: return "SOUND_ATTACK_FIREBALL";
    case SOUND_ATTACK_TORNADO: return "SOUND_ATTACK_TORNADO";
    case SOUND_ATTACK_THUNDER: return "SOUND_ATTACK_THUNDER";

    case SOUND_MAGIC_HEALING: return "SOUND_MAGIC_HEALING";
    case SOUND_MAGIC_WEAKNESS: return "SOUND_MAGIC_WEAKNESS";

    case SOUND_BUILDING_BUILD: return "SOUND_BUILDING_BUILD";
    case SOUND_BUILDING_DEMOLISH: return "SOUND_BUILDING_DEMOLISH";
    case SOUND_BUILDING_UPGRADE: return "SOUND_BUILDING_UPGRADE";

    case SOUND_FLAG_UP: return "SOUND_FLAG_UP";
    case SOUND_FLAG_DOWN: return "SOUND_FLAG_DOWN";

    case SOUND_REGION_CONQUERED: return "SOUND_REGION_CONQUERED";
    case SOUND_REGION_LOST: return "SOUND_REGION_LOST";

    case SOUND_UI_CLICK: return "SOUND_UI_CLICK";
    }
    return "UNHANDLED SOUND EFFECT NAME";
}

Sound LoadSoundAlias(Sound);
void UnloadSoundAlias(Sound);

void play_sound (const Assets * assets, SoundEffectType kind) {
    Sound sound = {0};
    for (usize i = 0; i < assets->sound_effects.len; i++) {
        SoundEffect * effect = &assets->sound_effects.items[i];
        if (effect->kind == kind) {
            sound = effect->sound;
            break;
        }
    }
    if (sound.frameCount == 0) {
        TraceLog(LOG_WARNING, "Couldn't find sound: %s", sound_kind_name(kind));
        return;
    }
    float pitch = GetRandomValue(80, 120) * 0.01;
    SetSoundPitch(sound, pitch);
    PlaySound(sound);
}
void play_sound_inworld (const GameState * game, SoundEffectType kind, Vector2 position) {
    Vector2 screen_pos = GetWorldToScreen2D(position, game->camera);
    Vector2 screen = { GetScreenWidth(), GetScreenHeight() };
    Vector2 center = Vector2Scale(screen, 0.5);
    float len = Vector2Length(center);
    float dist = Vector2Distance(center, screen_pos);
    float volume = dist / len ;
    volume = 1.0f - volume * volume;
    float zoom = game->camera.zoom / 10.0;
    volume *= zoom;
    if (volume <= 0.0) return;

    const Assets * assets = game->resources;
    Sound sound = {0};
    for (usize i = 0; i < assets->sound_effects.len; i++) {
        SoundEffect * effect = &assets->sound_effects.items[i];
        if (effect->kind == kind) {
            sound = effect->sound;
            break;
        }
    }
    if (sound.frameCount == 0) {
        TraceLog(LOG_WARNING, "Couldn't find sound: %s", sound_kind_name(kind));
        return;
    }
    SetSoundVolume(sound, volume);

    float pan = ( center.x - screen_pos.x ) / len;
    pan = pan * 0.5f + 0.5f;
    SetSoundPan(sound, pan);

    float pitch = GetRandomValue(80, 120) * 0.01;
    SetSoundPitch(sound, pitch);
    PlaySound(sound);
}
void play_sound_concurent (GameState * game, SoundEffectType kind, Vector2 position) {
    Vector2 screen_pos = GetWorldToScreen2D(position, game->camera);
    Vector2 screen = { GetScreenWidth(), GetScreenHeight() };
    Vector2 center = Vector2Scale(screen, 0.5);
    float len = Vector2Length(center);
    float dist = Vector2Distance(center, screen_pos);
    float volume = dist / len ;
    volume = 1.0f - volume * volume;
    float zoom = game->camera.zoom / 10.0;
    volume *= zoom;
    if (volume <= 0.0) return;

    SoundEffect sound = {0};
    for (usize i = 0; i < game->disabled_sounds.len; i++) {
        if (game->disabled_sounds.items[i].kind == kind) {
            sound = game->disabled_sounds.items[i];
            listSFXRemove(&game->disabled_sounds, i);
            break;
        }
    }
    if (sound.sound.frameCount == 0) {
        const Assets * assets = game->resources;
        for (usize i = 0; i < assets->sound_effects.len; i++) {
            SoundEffect * effect = &assets->sound_effects.items[i];
            if (effect->kind == kind) {
                sound = (SoundEffect){ kind, LoadSoundAlias(effect->sound) };
                break;
            }
        }
    }
    if (sound.sound.frameCount == 0) {
        TraceLog(LOG_WARNING, "Couldn't find sound: %s", sound_kind_name(kind));
        return;
    }
    if (listSFXAppend(&game->active_sounds, sound)) {
        TraceLog(LOG_ERROR, "Failed to play sound concurently: %s", sound_kind_name(kind));
        UnloadSoundAlias(sound.sound);
        return;
    }

    SetSoundVolume(sound.sound, volume);

    float pan = ( center.x - screen_pos.x ) / len;
    pan = pan * 0.5f + 0.5f;
    SetSoundPan(sound.sound, pan);

    float pitch = GetRandomValue(80, 120) * 0.01;
    SetSoundPitch(sound.sound, pitch);
    PlaySound(sound.sound);
}
void stop_sounds (ListSFX * sounds) {
    for (usize i = 0; i < sounds->len; i++) {
        if (IsSoundPlaying(sounds->items[i].sound)) {
            StopSound(sounds->items[i].sound);
        }
        UnloadSoundAlias(sounds->items[i].sound);
    }
    sounds->len = 0;
}
void clean_sounds (GameState * state) {
    ListSFX * sounds = &state->active_sounds;
    ListSFX * rest = &state->disabled_sounds;

    usize idx = sounds->len;
    while (idx --> 0) {
        SoundEffect sound = sounds->items[idx];
        if (! IsSoundPlaying(sound.sound)) {
            listSFXRemove(sounds, idx);
            listSFXAppend(rest, sound);
        }
    }
}

void play_unit_hurt_sound (GameState * game, const Unit * unit) {
    switch (unit->type) {
        case UNIT_FIGHTER: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_HURT_HUMAN, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_HURT_GOLEM, unit->position);
                } break;
            }
        } break;
        case UNIT_ARCHER: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_HURT_HUMAN, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_HURT_HUMAN_OLD, unit->position);
                } break;
            }
        } break;
        case UNIT_SUPPORT: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_HURT_HUMAN_OLD, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_HURT_GREMLIN, unit->position);
                } break;
            }
        } break;
        case UNIT_SPECIAL: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_HURT_KNIGHT, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_HURT_GENIE, unit->position);
                } break;
            }
        } break;
        case UNIT_GUARDIAN: {
            play_sound_concurent(game, SOUND_HURT_CASTLE, unit->position);
        } break;
    }
}
void play_unit_attack_sound (GameState * game, const Unit * unit) {
    switch (unit->type) {
        case UNIT_FIGHTER: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_ATTACK_SWORD, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_ATTACK_GOLEM, unit->position);
                } break;
            }
        } break;
        case UNIT_ARCHER: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_ATTACK_BOW, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_ATTACK_FIREBALL, unit->position);
                } break;
            }
        } break;
        case UNIT_SUPPORT: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_ATTACK_HOLY, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_ATTACK_TORNADO, unit->position);
                } break;
            }
        } break;
        case UNIT_SPECIAL: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_ATTACK_KNIGHT, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_ATTACK_THUNDER, unit->position);
                } break;
            }
        } break;
        case UNIT_GUARDIAN: {
            switch (unit->faction) {
                case FACTION_KNIGHTS: {
                    play_sound_concurent(game, SOUND_ATTACK_BOW, unit->position);
                } break;
                case FACTION_MAGES: {
                    play_sound_concurent(game, SOUND_ATTACK_FIREBALL, unit->position);
                } break;
            }
        } break;
    }
}
