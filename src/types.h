#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>
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

typedef enum BuildingType BuildingType;
typedef enum PlayerState PlayerState;
typedef enum MagicType MagicType;
typedef enum UnitType UnitType;
typedef enum UnitState UnitState;
typedef enum Result Result;
typedef enum Test Test;
typedef enum GraphType GraphType;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long usize;
typedef long isize;
typedef unsigned short ushort;

enum Result {
    SUCCESS = 0,
    FAILURE,
    FATAL,
};

enum Test {
    NO = 0,
    YES,
};

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

makeList(FindPoint, FindPoint);
makeList(WayPoint*, WayPoint);
makeList(NavGraph, NavGraph);
makeList(Unit*, Unit);
makeList(Region*, RegionP);

typedef enum FactionType {
    FACTION_KNIGHTS = 0,
    FACTION_MAGES = 1,
    FACTION_LAST = FACTION_MAGES,
} FactionType;

enum UnitType {
    UNIT_FIGHTER,
    UNIT_ARCHER,
    UNIT_SUPPORT,
    UNIT_SPECIAL,
    UNIT_GUARDIAN,
};

enum UnitState {
    UNIT_STATE_IDLE = 0,
    UNIT_STATE_MOVING,
    UNIT_STATE_CHASING,
    UNIT_STATE_FIGHTING,
    UNIT_STATE_SUPPORTING,
    UNIT_STATE_GUARDING,
};

enum MagicType {
    MAGIC_HEALING = 0,
    /* MAGIC_CURSE, */
    MAGIC_WEAKNESS,
    /* MAGIC_STRENGTH, */
    /* MAGIC_HELLFIRE, */
    MAGIC_TYPE_LAST = MAGIC_WEAKNESS,
};

enum GraphType {
    GRAPH_REGION,
    GRAPH_PATH,
};

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

    float     cooldown;
    float     health;
    UnitState state;
    Vector2   position;

    WayPoint * waypoint;
    ListWayPoint pathfind;
    usize current_path;

    ListMagicEffect effects;
    ListAttack incoming_attacks;

    Building * origin;
};

struct Line {
    Vector2 a;
    Vector2 b;
};

enum BuildingType {
    BUILDING_EMPTY = 0,
    BUILDING_FIGHTER,
    BUILDING_ARCHER,
    BUILDING_SUPPORT,
    BUILDING_SPECIAL,
    BUILDING_RESOURCE,
    BUILDING_TYPE_LAST = BUILDING_RESOURCE,
};

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
    ListLine lines;
    Model    model;
    Region * region_a;
    Region * region_b;
    NavGraph nav_graph;
    Map    * map;
};

struct Region {
    FactionType   faction;
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

struct PlayerData {
    usize resource_gold;
    PlayerType type;
    FactionType faction;
    usize seed;
};

typedef enum {
    EXE_MODE_MAIN_MENU,
    EXE_MODE_SINGLE_PLAYER_MAP_SELECT,
    EXE_MODE_IN_GAME,
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

struct Assets {
    ListMap maps;
    Shader water_shader;
    Texture2D particles[PARTICLE_LAST + 1];
    Particle particle_pool[PARTICLES_MAX];
    BuildingSpriteSet buildings[FACTION_LAST + 1];
    Texture2D neutral_castle;
    Texture2D flag;
    Music faction_themes[FACTION_LAST + 1];
    Music main_theme;
    Texture2D water_texture;
    Texture2D ground_texture;
    ListSFX sound_effects;
    // TODO fill assets:
    // units
};

struct Theme {
    Color background;
    Color background_light;
    Color frame;
    Color frame_light;
    Color text;
    Color text_dark;
    Color button;
    Color button_hover;
    Color button_inactive;
    Color button_frame;

    float font_size;
    float margin;
    float spacing;
    float frame_thickness;

    float info_bar_height;
    float info_bar_field_width;

    float dialog_build_width;
    float dialog_build_height;
    float dialog_upgrade_width;
    float dialog_upgrade_height;
};

struct Settings {
    float volume_master;
    float volume_music;
    float volume_sfx;
    float volume_ui;

    Theme theme;
};

enum PlayerState {
    INPUT_NONE = 0,
    INPUT_CLICKED_BUILDING,
    INPUT_CLICKED_PATH,
    INPUT_OPEN_BUILDING,
    INPUT_MOVE_MAP,
};

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
