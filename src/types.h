#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>
#include <stddef.h>
#include <stdint.h>
#include "array.h"
#include "constants.h"

#define STRINGIFY(x) #x
#define STRINGIFY_VALUE(x) STRINGIFY(x)

typedef struct Line Line;
typedef struct Path Path;
typedef struct Building Building;
typedef struct Region Region;
typedef struct Map Map;
typedef struct Area Area;
typedef struct GameState GameState;
typedef struct Assets Assets;
typedef struct Settings Settings;
typedef struct Theme Theme;
typedef struct Unit Unit;
typedef struct PlayerData PlayerData;
typedef struct MagicEffect MagicEffect;
typedef struct Attack Attack;
typedef struct WayPoint WayPoint;
typedef struct FindPoint FindPoint;
typedef struct NavGraph NavGraph;
typedef struct GlobalNavGrid GlobalNavGrid;
typedef struct Particle Particle;
typedef struct SoundEffect SoundEffect;
typedef struct AIRegionScore AIRegionScore;
typedef struct AnimationFrame AnimationFrame;
typedef struct AnimationSet AnimationSet;
typedef struct Animations Animations;

typedef unsigned char uchar;
typedef size_t usize;
typedef long isize;
typedef unsigned short ushort;

typedef enum {
    SUCCESS = 0,
    FAILURE,
    FATAL,
} Result;

typedef enum {
    NO = 0,
    YES,
} Test;

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef bool
#define bool int
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

makeList(Vector2, Vector2);
makeList(ushort, Ushort);
makeList(usize, Usize);
makeList(Line, Line);
makeList(Path, Path);
makeList(Building, Building);
makeList(Region, Region);
makeList(PlayerData, PlayerData);
makeList(Map, Map);
makeList(MagicEffect, MagicEffect);
makeList(Attack, Attack);
makeList(Path*, PathP);
makeList(Particle*, Particle);
makeList(SoundEffect, SFX);
makeList(AIRegionScore, AIRegionScore);
makeList(AIRegionScore*, AIRegionScoreP);
makeList(AnimationFrame, Frame);

makeList(FindPoint, FindPoint);
makeList(WayPoint*, WayPoint);
makeList(NavGraph, NavGraph);
makeList(Unit*, Unit);
makeList(Region*, RegionP);

// @volitile=faction
typedef enum FactionType {
    FACTION_KNIGHTS = 0,
    FACTION_MAGES = 1,
    FACTION_LAST = FACTION_MAGES,
} FactionType;

#define FACTION_COUNT (FACTION_LAST + 1)

// @volitile=unit
typedef enum {
    UNIT_FIGHTER,
    UNIT_ARCHER,
    UNIT_SUPPORT,
    UNIT_SPECIAL,
    UNIT_GUARDIAN,
} UnitType ;

// @volitile=unit
#define UNIT_TYPE_ALL_COUNT (UNIT_GUARDIAN + 1)

// @volitile=unit
typedef enum {
   UNIT_TYPE_FIGHTER = UNIT_FIGHTER,
   UNIT_TYPE_ARCHER = UNIT_ARCHER,
   UNIT_TYPE_SUPPORT = UNIT_SUPPORT,
   UNIT_TYPE_SPECIAL = UNIT_SPECIAL,
} UnitActiveType;

// @volitile=unit
#define UNIT_TYPE_COUNT (UNIT_TYPE_SPECIAL + 1)

typedef enum {
    UNIT_STATE_IDLE = 0,
    UNIT_STATE_MOVING,
    UNIT_STATE_CHASING,
    UNIT_STATE_FIGHTING,
    UNIT_STATE_SUPPORTING,
    UNIT_STATE_GUARDING,
} UnitState;

typedef enum {
    MAGIC_HEALING = 0,
    /* MAGIC_CURSE, */
    MAGIC_WEAKNESS,
    /* MAGIC_STRENGTH, */
    /* MAGIC_HELLFIRE, */
    MAGIC_TYPE_LAST = MAGIC_WEAKNESS,
} MagicType;

typedef enum {
    GRAPH_REGION,
    GRAPH_PATH,
} GraphType;

typedef enum {
    PARTICLE_ARROW = 0,
    PARTICLE_FIREBALL,
    PARTICLE_FIST,
    PARTICLE_PLUS,
    PARTICLE_SLASH,
    PARTICLE_SLASH_2,
    PARTICLE_SYMBOL,
    PARTICLE_THUNDERBOLT,
    PARTICLE_TORNADO,
    PARTICLE_LAST = PARTICLE_TORNADO,
} ParticleType;

typedef enum {
    ANIMATION_IDLE,
    ANIMATION_WALK,
    ANIMATION_ATTACK,
    ANIMATION_CAST,
} AnimationType;

typedef enum {
    SOUND_HURT_HUMAN,
    SOUND_HURT_HUMAN_OLD,
    SOUND_HURT_KNIGHT,
    SOUND_HURT_GOLEM,
    SOUND_HURT_GREMLIN,
    SOUND_HURT_GENIE,
    SOUND_HURT_CASTLE,

    SOUND_ATTACK_SWORD,
    SOUND_ATTACK_BOW,
    SOUND_ATTACK_HOLY,
    SOUND_ATTACK_KNIGHT,
    SOUND_ATTACK_GOLEM,
    SOUND_ATTACK_FIREBALL,
    SOUND_ATTACK_TORNADO,
    SOUND_ATTACK_THUNDER,

    SOUND_MAGIC_HEALING,
    SOUND_MAGIC_WEAKNESS,

    SOUND_BUILDING_BUILD,
    SOUND_BUILDING_DEMOLISH,
    SOUND_BUILDING_UPGRADE,

    SOUND_FLAG_UP,
    SOUND_FLAG_DOWN,

    SOUND_REGION_CONQUERED,
    SOUND_REGION_LOST,

    SOUND_UI_CLICK,
} SoundEffectType;

struct SoundEffect {
    SoundEffectType kind;
    Sound sound;
};

typedef struct {
    Vector2 start;
    Vector2 start_handle;
    Vector2 end_handle;
    Vector2 end;
} AnimationCurve;

struct Particle {
    float lifetime;
    float time_lived;
    Color color_start;
    Color color_end;
    Vector2 position;
    Vector2 velocity;
    AnimationCurve velocity_curve;
    AnimationCurve rotation_curve;
    AnimationCurve scale_curve;
    AnimationCurve alpha_curve;
    AnimationCurve color_curve;
    Texture2D * sprite;
};

struct WayPoint {
    Vector2    world_position;
    usize      nav_world_pos_x;
    usize      nav_world_pos_y;
    NavGraph * graph;
    Unit     * unit;
    bool       blocked;
};

struct FindPoint {
    FindPoint * from;
    float cost;
    bool  visited;
    bool  queued;
};

struct NavGraph {
    GraphType type;
    usize width;
    usize height;
    usize offset_x;
    usize offset_y;
    ListWayPoint waypoints;
    union {
        Region * region;
        Path * path;
    };
    GlobalNavGrid * global;
};

struct GlobalNavGrid {
    usize width;
    usize height;
    ListWayPoint waypoints;
    ListFindPoint find_buffer;
} ;

struct MagicEffect {
    float     strength;
    float     duration;
    usize     source_player;
    MagicType type;
};

struct Attack {
    usize   damage;
    float   timer;
    float   delay;
    usize   attacker_player_id;
    UnitType attacker_type;
    FactionType attacker_faction;
    Vector2 origin_position;
};

struct Unit {
    UnitType  type;
    ushort    upgrade;

    usize       player_owned;
    FactionType faction;

    float     state_time;
    float     cooldown;
    float     health;
    UnitState state;
    Vector2   position;
    Vector2   facing_direction;

    WayPoint * waypoint;
    ListWayPoint pathfind;
    usize current_path;

    ListMagicEffect effects;
    ListAttack incoming_attacks;
    bool attacked;

    Building * origin;
};

struct AnimationFrame {
    Rectangle source;
    float duration;
};

struct AnimationSet {
    Texture2D sprite_sheet;
    ListFrame frames;

    float idle_duration;
    float walk_duration;
    float attack_duration;
    float cast_duration;

    uint8_t idle_start;
    uint8_t walk_start;
    uint8_t attack_start;
    uint8_t cast_start;
    uint8_t attack_trigger;
    uint8_t cast_trigger;
};

struct Animations {
    AnimationSet sets[FACTION_COUNT][UNIT_TYPE_COUNT][UNIT_LEVELS];
};

struct Line {
    Vector2 a;
    Vector2 b;
};

typedef struct {
    float min;
    float max;
} Range;

// @volitile=buildings
typedef enum {
    BUILDING_EMPTY = 0,
    BUILDING_FIGHTER,
    BUILDING_ARCHER,
    BUILDING_SUPPORT,
    BUILDING_SPECIAL,
    BUILDING_RESOURCE,
} BuildingType;

#define BUILDING_TYPE_LAST BUILDING_RESOURCE
#define BUILDING_TYPE_COUNT (BUILDING_RESOURCE + 1)

struct Building {
    Vector2        position;
    BuildingType   type;
    ushort         upgrades;
    float          spawn_timer;
    usize          units_spawned;
    Region       * region;
    ListWayPoint   spawn_points;
};

struct Area {
    ListLine lines;
    Model    model;
    Model    outline;
};

struct Path {
    usize path_id;
    ListLine lines;
    Model    model;
    Region * region_a;
    Region * region_b;
    NavGraph nav_graph;
    Map    * map;
};

struct Region {
    FactionType   faction;
    usize         region_id;
    usize         player_id;
    Area          area;
    Unit          castle;
    ListBuilding  buildings;
    ListPathP     paths;
    usize         active_path;
    NavGraph      nav_graph;
    Map         * map;
};

struct Map {
    char        * name;
    usize         width;
    usize         height;
    uchar         player_count;
    ListRegion    regions;
    ListPath      paths;
    GlobalNavGrid nav_grid;
    Model         background;
};

typedef enum PlayerType {
    PLAYER_NEUTRAL,
    PLAYER_LOCAL,
    PLAYER_AI,
} PlayerType;

struct AIRegionScore {
    Region * region;
    usize random_focus_bonus;
    usize frontline_distance;
    bool enemies_present;
    bool frontline;
    bool gather_troops;
};
typedef struct {
    float fighter;
    float archer;
    float support;
    float special;
    float resources;
} AIDesiredBuildings;

typedef struct {
    usize seed;
    AIDesiredBuildings buildings;
    ListAIRegionScore regions;
    bool aggressive;
    bool new_conquest;
} AIData;

struct PlayerData {
    usize resource_gold;
    PlayerType type;
    FactionType faction;
    AIData * ai;
};

typedef enum {
    EXE_MODE_MAIN_MENU,
    EXE_MODE_SINGLE_PLAYER_MAP_SELECT,
    EXE_MODE_IN_GAME,
    EXE_MODE_TUTORIAL,
    EXE_MODE_MANUAL,
    EXE_MODE_EXIT,
} ExecutionMode;

typedef struct {
    Texture2D castle;
    Texture2D fighter[3];
    Texture2D archer[3];
    Texture2D support[3];
    Texture2D special[3];
    Texture2D resource[3];
} BuildingSpriteSet;

typedef struct {
    Texture2D  background_box;
    NPatchInfo background_box_info;
    Texture2D  button;
    Texture2D  button_press;
    NPatchInfo button_info;
    Texture2D  close;
    Texture2D  close_press;
    Texture2D  slider;
    Texture2D  slider_thumb;
    NPatchInfo slider_info;
    Texture2D  drop;
    #if defined(ANDROID)
    Texture2D  joystick;
    Texture2D  zoom_in;
    Texture2D  zoom_out;
    #endif
} UiAssets;

struct Assets {
    ListMap maps;
    Shader water_shader;
    Shader outline_shader;
    Texture2D particles[PARTICLE_LAST + 1];
    Particle particle_pool[PARTICLES_MAX];
    BuildingSpriteSet buildings[FACTION_LAST + 1];
    Texture2D neutral_castle;
    Texture2D flag;
    Music faction_themes[FACTION_LAST + 1];
    Music main_theme;
    Texture2D water_texture;
    Texture2D ground_texture;
    Texture2D bridge_texture;
    Texture2D empty_building;
    ListSFX sound_effects;
    Animations animations;
    UiAssets ui;
};

struct Theme {
    Color text;
    Color text_dark;
    Color button;
    Color button_hover;
    Color button_inactive;

    float font_size;
    float margin;
    float spacing;
    float frame_thickness;

    float info_bar_height;
    float info_bar_field_width;

    float dialog_width;

    const UiAssets * assets;
};

struct Settings {
    float volume_master;
    float volume_music;
    float volume_sfx;
    float volume_ui;
    bool fullscreen;

    Theme theme;
};

typedef enum {
    INPUT_NONE = 0,
    INPUT_CLICKED_BUILDING,
    INPUT_CLICKED_PATH,
    INPUT_OPEN_BUILDING,
    INPUT_MOVE_MAP,
} PlayerState;

struct GameState {
    PlayerState      current_input;
    Vector2          selected_point;
    Building       * selected_building;
    Path           * selected_path;
    Region         * selected_region;
    Map              map;
    ListUnit         units;
    ListPlayerData   players;
    ListParticle     particles_in_use;
    ListParticle     particles_available;
    usize            turn;
    Camera2D         camera;
    ListSFX          active_sounds;
    ListSFX          disabled_sounds;
    const Assets   * resources;
    const Settings * settings;
};

/* Utils *********************************************************************/
char * faction_to_string (FactionType faction);
char * building_type_to_string (BuildingType type);
#endif // TYPES_H_
