#ifndef TYPES_H_
#define TYPES_H_

#include <raylib.h>
#include "array.h"

#define STRINGIFY(x) #x
#define STRINGIFY_VALUE(x) STRINGIFY(x)

typedef struct Node Node;
typedef struct Bridge Bridge;
typedef struct PathBridge PathBridge;
typedef struct Line Line;
typedef struct Path Path;
typedef struct PathEntry PathEntry;
typedef struct Building Building;
typedef struct Castle Castle;
typedef struct Region Region;
typedef struct Map Map;
typedef struct Area Area;
typedef struct GameState GameState;
typedef struct Unit Unit;
typedef struct PlayerData PlayerData;

typedef enum Movement Movement;
typedef enum BuildingType BuildingType;
typedef enum PlayerState PlayerState;
typedef enum UnitType UnitType;
typedef enum UnitState UnitState;
typedef enum Result Result;
typedef enum Test Test;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long int usize;
typedef long int size;
typedef unsigned short ushort;

typedef void * ( * Allocator )(uint size);
typedef void ( * Deallocator )(void * ptr);

enum Result {
    SUCCESS = 0,
    FAILURE,
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
makeList(PathEntry, PathEntry);
makeList(Building, Building);
makeList(Region, Region);
makeList(PathBridge, PathBridge);
makeList(Bridge, Bridge);
makeList(PlayerData, PlayerData);

makeList(Unit*, Unit);
makeList(Region*, RegionP);

typedef enum FactionType {
    FACTION_KNIGHTS,
    FACTION_MAGES,
} FactionType;

enum UnitType {
    UNIT_FIGHTER,
    UNIT_ARCHER,
    UNIT_SUPPORT,
    UNIT_SPECIAL,
    UNIT_GUARDIAN,
};

enum UnitState {
    UNIT_STATE_MOVING = 0,
    UNIT_STATE_FIGHTING,
    UNIT_STATE_GUARDING,
};

enum Movement {
    MOVEMENT_DIR_FORWARD = 0,
    MOVEMENT_DIR_BACKWARD,
    MOVEMENT_INVALID,
};

struct Unit {
    float       health;
    Vector2     position;
    UnitType    type;
    ushort      upgrade;
    UnitState   state;
    Node      * location;
    Movement    move_direction;
    usize       player_owned;
    FactionType faction;
};

struct Node {
    Node    * previous;
    Node    * next;
    Unit    * unit;
    Bridge  * bridge;
    Vector2   position;
};

struct Bridge {
    Node * start;
    Node * end;
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
    BUILDING_TYPE_COUNT,
};

struct Building {
    Vector2        position;
    BuildingType   type;
    Model          model;
    ushort         upgrades;
    float          spawn_timer;
    ListBridge     spawn_paths;
    usize          active_spawn;
    Region       * region;
};

struct Castle {
    Vector2  position;
    Model    model;
    Node     guardian_spot;
    Region * region;
    Unit     guardian;
};

struct Area {
    ListLine lines;
    Model    model;
};

struct Path {
    ListLine lines;
    Model    model;
    Bridge   bridge;
    Region * region_a;
    Region * region_b;
};

struct PathBridge {
    Path   * to;
    Bridge * bridge;
};

struct PathEntry {
    Path           * path;
    ListPathBridge   redirects;
    usize            active_redirect;
    Bridge           castle_path;
};

struct Region {
    Area          area;
    usize         player_id;
    Castle        castle;
    ListBuilding  buildings;
    ListPathEntry paths;
    FactionType   faction;
};

struct Map {
    usize      width;
    usize      height;
    uchar      player_count;
    ListRegion regions;
    ListPath   paths;
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
    Map            * current_map;
    ListUnit         units;
    ListPlayerData   players;
    usize            turn;
    Camera2D         camera;
};

typedef struct {
    uchar * start;
    usize   len;
} StringSlice;

StringSlice make_slice(uchar * from, usize start, usize end);

#endif // TYPES_H_
