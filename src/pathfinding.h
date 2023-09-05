#ifndef PATHFINDING_H_
#define PATHFINDING_H_

#include <raylib.h>
#include "array.h"
#include "types.h"

typedef enum {
    NAV_CONTEXT_SINGLE,
    NAV_CONTEXT_LIST,
} NavContextCounter;
typedef enum {
    NAV_CONTEXT_FRIENDLY,
    NAV_CONTEXT_HOSTILE,
} NavContextType;

typedef struct {
    NavContextType type;
    NavContextCounter amount;
    usize player_id;
    usize range;
    union {
        Unit * unit_found;
        ListUnit * unit_list;
    };
} NavRangeSearchContext;

typedef enum {
    NAV_TARGET_REGION,
    NAV_TARGET_WAYPOINT,
} NavTargetType;

typedef struct {
    NavTargetType type;
    bool approach_only;
    union {
        Region * region;
        WayPoint * waypoint;
    };
} NavTarget;

/* Inits *********************************************************************/
Result nav_init_global_grid (Map * map);
Result nav_init_path        (Path * path);
Result nav_init_region      (Region * region);
void   nav_deinit_global    (GlobalNavGrid * nav);

/* Lookup *********************************************************************/
Result nav_find_waypoint (NavGraph *const graph, Vector2 point, WayPoint ** nullable_result);
Result nav_range_search  (WayPoint * start, NavRangeSearchContext * context);
Test   nav_find_enemies  (NavGraph * graph, usize player_id, ListUnit * result);
Result nav_gather_points (WayPoint * around, ListWayPoint * result);

/* Pathfinding ***************************************************************/
Result nav_find_path (WayPoint * start, NavTarget target, ListWayPoint * result);

/* Debug *********************************************************************/
void nav_render (NavGraph * graph);

#endif // PATHFINDING_H_
