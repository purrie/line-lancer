#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define DEBUG
/* #define DEBUG_AI */

#define FPS 60

// TODO ensure map loading takes this into account
#define PLAYERS_MAX 16

#define UNIT_SIZE 6.0f
#define UNIT_MAX_RANGE 6

#define PARTICLES_MAX 512

#define NAV_GRID_SIZE 12

#define REGION_INCOME_INTERVAL 10
#define REGION_INCOME 3

#define BUILDING_MAX_UPGRADES 2
#define BUILDING_MAX_LEVEL 3

#define PATH_THICKNESS (NAV_GRID_SIZE * 3)

#define LAYER_BACKGROUND -0.4f
#define LAYER_MAP -0.3f
#define LAYER_MAP_OUTLINE -0.25f
#define LAYER_PATH -0.2f
#define LAYER_BUILDING -0.1f
#define MAP_BEVEL 5

/* #define RENDER_NAV_GRID */
#define RENDER_PATHS
/* #define RENDER_PATHS_DEBUG */

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

#endif // CONSTANTS_H_
