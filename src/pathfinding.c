#include "constants.h"
#include "pathfinding.h"
#include "std.h"
#include "level.h"
#include "math.h"
#include "alloc.h"
#include <raymath.h>

implementList(WayPoint*, WayPoint)
implementList(NavGraph, NavGraph)

/* Uitls *********************************************************************/
Result nav_position_global_world (const GlobalNavGrid * nav, usize x, usize y, Vector2 * position) {
    if (x >= nav->width || y >= nav->height) {
        return FAILURE;
    }
    position->x = x * NAV_GRID_SIZE + NAV_GRID_SIZE;
    position->y = y * NAV_GRID_SIZE + NAV_GRID_SIZE;
    return SUCCESS;
}
Result nav_position_world_global (const GlobalNavGrid * nav, Vector2 position, usize * out_x, usize * out_y) {
    Vector2 pos = { position.x + NAV_GRID_SIZE, position.y + NAV_GRID_SIZE };
    if (pos.x < 0.0f || pos.y < 0.0f) {
        return FAILURE;
    }
    *out_x = ( (usize)pos.x / NAV_GRID_SIZE ) - 1;
    *out_y = ( (usize)pos.y / NAV_GRID_SIZE ) - 1;
    if (*out_x >= nav->width || *out_y >= nav->height) {
        return FAILURE;
    }
    return SUCCESS;
}

/* Init **********************************************************************/
Result nav_init_global_grid (Map * map) {
    map->nav_grid.width = map->width / NAV_GRID_SIZE;
    map->nav_grid.height = map->height / NAV_GRID_SIZE;
    map->nav_grid.waypoints = listWayPointInit(map->nav_grid.width * map->nav_grid.height, perm_allocator());
    if (map->nav_grid.waypoints.items == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate space for global nav grid");
        return FAILURE;
    }
    map->nav_grid.find_buffer = listFindPointInit(map->nav_grid.waypoints.cap, perm_allocator());
    if (map->nav_grid.find_buffer.items == NULL) {
        listWayPointDeinit(&map->nav_grid.waypoints);
        TraceLog(LOG_ERROR, "Failed to allocate space for find nav grid");
        return FAILURE;
    }
    clear_memory(map->nav_grid.waypoints.items, sizeof(WayPoint *) * map->nav_grid.waypoints.cap);
    clear_memory(map->nav_grid.find_buffer.items, sizeof(FindPoint) * map->nav_grid.find_buffer.cap);

    map->nav_grid.waypoints.len = map->nav_grid.waypoints.cap;
    map->nav_grid.find_buffer.len = map->nav_grid.find_buffer.cap;
    return SUCCESS;
}
Result nav_init_path (Path * path) {
    GlobalNavGrid * global = &path->map->nav_grid;
    NavGraph * result = &path->nav_graph;

    Rectangle path_area;
    if(lines_bounds(&path->lines, &path_area)) {
        TraceLog(LOG_ERROR, " !Failed to obtain path bounds rectangle");
        return FAILURE;
    }

    path_area.x -= PATH_THICKNESS;
    path_area.y -= PATH_THICKNESS;
    path_area.width += PATH_THICKNESS * 2;
    path_area.height += PATH_THICKNESS * 2;

    usize x, y;
    if (nav_position_world_global(global, (Vector2){ path_area.x, path_area.y }, &x, &y)) {
        TraceLog(LOG_ERROR, " !Failed to get grid position for a path");
        return FAILURE;
    }
    result->path = path;
    result->type = GRAPH_PATH;
    result->global = global;
    result->width  = (usize)( path_area.width  / NAV_GRID_SIZE );
    result->height = (usize)( path_area.height / NAV_GRID_SIZE );
    result->offset_x = x;
    result->offset_y = y;
    result->waypoints = listWayPointInit(result->width * result->height, perm_allocator());
    if (result->waypoints.items == NULL) {
        TraceLog(LOG_ERROR, " !Failed to allocate memory for path navgraph");
        return FAILURE;
    }

    usize actual_points = 0;

    TraceLog(LOG_DEBUG, "  Starting path tests");
    for (usize yi = 0; yi < result->height; yi ++) {
        for (usize xi = 0; xi < result->width; xi ++) {
            usize index = result->width * yi + xi;
            if (index >= result->waypoints.cap) {
                TraceLog(LOG_ERROR, "Waypoint list too short");
                listWayPointDeinit(&result->waypoints);
                return FAILURE;
            }
            // test if the point is inside neighboring regions
            usize global_x = xi + x;
            usize global_y = yi + y;
            usize global_index = global->width * global_y + global_x;

            if (global->waypoints.items[global_index] != NULL) {
                goto no_point;
            }

            Vector2 point;
            if (nav_position_global_world(global, global_x, global_y, &point)) {
                TraceLog(LOG_ERROR, "Failed to obtain path waypoint location on global grid");
                listWayPointDeinit(&result->waypoints);
                return FAILURE;
            }

            bool near_line = lines_check_hit(&path->lines, point, PATH_THICKNESS * 0.5f);

            if (near_line) {
                actual_points ++;
                WayPoint * way = MemAlloc(sizeof(WayPoint));
                if (way == NULL) {
                    TraceLog(LOG_ERROR, " !Failed to allocate waypoint in path");
                    listWayPointDeinit(&result->waypoints);
                    return FAILURE;
                }
                clear_memory(way, sizeof(WayPoint));

                way->graph = result;
                way->nav_world_pos_x = global_x;
                way->nav_world_pos_y = global_y;
                way->world_position = point;

                result->waypoints.items[index] = way;
                global->waypoints.items[global_index] = way;
            }
            else {
                no_point:
                result->waypoints.items[index] = NULL;
            }
        }
    }
    if (actual_points == 0) {
        TraceLog(LOG_ERROR, " !Failed to initialize path because it would contain no waypoints");
        listWayPointDeinit(&result->waypoints);
        return FAILURE;
    }
    result->waypoints.len = result->waypoints.cap;
    return SUCCESS;
}
Result nav_init_region (Region * region) {
    GlobalNavGrid * global = &region->map->nav_grid;
    NavGraph * result = &region->nav_graph;
    Rectangle region_area = area_bounds(&region->area);

    usize x, y;
    if (nav_position_world_global(global, ( Vector2 ) { region_area.x, region_area.y }, &x, &y)) {
        TraceLog(LOG_ERROR, " !Failed to obtain grid position for region");
        return FAILURE;
    }

    result->type   = GRAPH_REGION;
    result->region = region;
    result->global = global;
    result->width  = region_area.width  / NAV_GRID_SIZE;
    result->height = region_area.height / NAV_GRID_SIZE;
    result->offset_x = x;
    result->offset_y = y;
    result->waypoints = listWayPointInit(result->width * result->height, perm_allocator());
    if (result->waypoints.items == NULL) {
        TraceLog(LOG_ERROR, " !Failed to initialize waypoint array");
        return FAILURE;
    }

    ListVector2 intersections = listVector2Init(region->area.lines.len, temp_allocator());
    Vector2 a = { region_area.x - region_area.width, region_area.y - region_area.height };

    usize actual_points = 0;
    for (usize yi = 0; yi < result->height; yi ++) {
        for (usize xi = 0; xi < result->width; xi ++) {
            usize index = result->width * yi + xi;

            if (index >= result->waypoints.cap) {
                TraceLog(LOG_ERROR, " !Waypoint array too short");
                listWayPointDeinit(&result->waypoints);
                return FAILURE;
            }

            usize global_x = xi + x;
            usize global_y = yi + y;
            Vector2 point;
            if (nav_position_global_world(global, global_x, global_y, &point)) {
                TraceLog(LOG_ERROR, "Failed to get position of region waypoint");
                listWayPointDeinit(&result->waypoints);
                return FAILURE;
            }

            Line line = { a, point };
            usize points = lines_intersections(region->area.lines, line, &intersections);
            usize crossings = points % 2;

            if (crossings != 0) {
                actual_points ++;
                WayPoint * way = MemAlloc(sizeof(WayPoint));
                if (way == NULL) {
                    TraceLog(LOG_ERROR, " !Failed to allocate memory for region waypoint");
                    listWayPointDeinit(&result->waypoints);
                    return FAILURE;
                }
                clear_memory(way, sizeof(WayPoint));

                way->graph = result;
                way->nav_world_pos_x = global_x;
                way->nav_world_pos_y = global_y;
                way->world_position = point;

                usize global_index = global->width * global_y + global_x;

                result->waypoints.items[index] = way;
                global->waypoints.items[global_index] = way;
            }
            else {
                result->waypoints.items[index] = NULL;
            }
            intersections.len = 0;
        }
    }

    if (actual_points == 0) {
        TraceLog(LOG_ERROR, " !Couldn't create any waypoint");
        listWayPointDeinit(&result->waypoints);
        return FAILURE;
    }
    result->waypoints.len = result->waypoints.cap;
    TraceLog(LOG_DEBUG, " Successfuly initialized navgrid. Total points = %zu, active points = %zu", result->waypoints.len, actual_points);

    return SUCCESS;
}
void nav_deinit_global (GlobalNavGrid * nav) {
    for (usize y = 0; y < nav->height; y++) {
        for (usize x = 0; x < nav->width; x++) {
            WayPoint * point = nav->waypoints.items[nav->width * y + x];
            if (point) {
                MemFree(point);
            }
        }
    }
    listWayPointDeinit(&nav->waypoints);
    listFindPointDeinit(&nav->find_buffer);
}

/* Lookups *******************************************************************/
Result nav_find_waypoint (const NavGraph * graph, Vector2 point, WayPoint ** nullable_result) {
    if (point.x < 0.0f || point.y < 0.0f)
        return FAILURE;
    usize x, y;
    if (nav_position_world_global(graph->global, point, &x, &y)) {
        return FAILURE;
    }

    if (x < graph->offset_x || y < graph->offset_y) {
        return FAILURE;
    }
    x -= graph->offset_x;
    y -= graph->offset_y;
    if (x >= graph->width || y >= graph->height) {
        return FAILURE;
    }

    usize index = graph->width * y + x;
    if (index >= graph->waypoints.len)
        return FAILURE;

    *nullable_result = graph->waypoints.items[index];
    return SUCCESS;
}
Result nav_range_search (WayPoint * start, NavRangeSearchContext * context) {
    isize length = 2;
    isize step = 2;
    char dir = 0;
    int remaining = context->range;

    GlobalNavGrid * grid = start->graph->global;
    isize width = grid->width;
    isize height = grid->height;
    isize x = start->nav_world_pos_x - 1;
    isize y = start->nav_world_pos_y - 1;

    while (remaining > 0) {
        while (step --> 0) {
            if (x < 0 || y < 0)
                goto skip;
            if (x >= width || y >= height)
                goto skip;

            WayPoint * point = grid->waypoints.items[grid->width * y + x];
            if (point && point->unit) {
                bool found;
                switch (context->type) {
                    case NAV_CONTEXT_FRIENDLY: {
                        found = point->unit->player_owned == context->player_id;
                    } break;
                    case NAV_CONTEXT_HOSTILE: {
                        found = point->unit->player_owned != context->player_id;
                    } break;
                }
                if (found) {
                    switch (context->amount) {
                        case NAV_CONTEXT_SINGLE: {
                            context->unit_found = point->unit;
                            return SUCCESS;
                        } break;
                        case NAV_CONTEXT_LIST: {
                            if (listUnitAppend(context->unit_list, point->unit)) {
                                return FATAL;
                            }
                        } break;
                    }
                }
            }

            skip:
            switch (dir) {
                case 0: x += 1; break;
                case 1: y += 1; break;
                case 2: x -= 1; break;
                case 3: y -= 1; break;
            }
        }
        if (dir == 3) {
            remaining -= 1;
            length += 2;
            x -= 1;
            y -= 1;
        }
        dir = (dir + 1) % 4;
        step = length;
    }

    switch (context->amount) {
        case NAV_CONTEXT_SINGLE: return FAILURE;
        case NAV_CONTEXT_LIST: return context->unit_list->len > 0 ? SUCCESS : FAILURE;
        default: return FAILURE;
    }
}
Test nav_find_enemies (NavGraph * graph, usize player_id, ListUnit * result) {
    if (result)
        result->len = 0;

    for (usize i = 0; i < graph->waypoints.len; i++) {
        WayPoint * point = graph->waypoints.items[i];
        if (point == NULL)
            continue;
        if (point->unit == NULL || point->unit->player_owned == player_id)
            continue;

        if (result)
            listUnitAppend(result, point->unit);
        else
            return YES;
    }
    if (result) {
        return result->len ? YES : NO;
    }
    return NO;
}

Result nav_gather_points (WayPoint * around, ListWayPoint * result) {
    GlobalNavGrid * grid = around->graph->global;
    isize index = grid->width * around->nav_world_pos_y + around->nav_world_pos_x;

    isize neighbor_index[8] = {
        -grid->width - 1, -grid->width, 1 - grid->width,
        -1, 1,
        grid->width - 1, grid->width, grid->width + 1
    };

    usize added = 0;
    for (usize i = 0; i < 8; i++) {
        isize idx = index + neighbor_index[i];
        if (idx < 0) {
            continue;
        }
        if ((usize)idx >= grid->waypoints.len) {
            continue;
        }

        if (listWayPointAppend(result, grid->waypoints.items[idx])) {
            return FATAL;
        }
        added ++;
    }
    return added ? SUCCESS : FAILURE;
}

#define HEAP_TYPE FindPoint *
#define HEAP_NAME FindPoint
#define HEAP_DECLARATION
#define HEAP_IMPLEMENTATION
#include "heap.h"

implementList(FindPoint, FindPoint)

int find_point_compare(FindPoint * a, FindPoint * b) {
    if (a && b) {
        if (a->cost > b->cost)
            return 1;
        else
            return -1;
    }
    if (a == NULL && b == NULL) {
        return 0;
    }
    if (a)
        return 1;
    else
        return -1;
}
int find_point_eql(FindPoint * a, FindPoint * b) {
    if (a && b) {
        return a == b;
    }
    if (a == NULL && b == NULL) {
        return 1;
    }
    return 0;
}

Result nav_find_path (WayPoint * start, NavTarget target, ListWayPoint * result) {
    HeapFindPoint heap;
    if (heapFindPointInit(start->graph->waypoints.len, &heap, temp_allocator(), find_point_compare, find_point_eql)) {
        goto failure;
    }

    Vector2 target_position;
    switch (target.type) {
        case NAV_TARGET_REGION: {
            target_position = target.region->castle.position;
        } break;
        case NAV_TARGET_WAYPOINT: {
            target_position = target.waypoint->world_position;
        } break;
    }
    usize start_x = start->nav_world_pos_x;
    usize start_y = start->nav_world_pos_y;
    GlobalNavGrid * grid = start->graph->global;

    clear_memory(grid->find_buffer.items, sizeof(FindPoint) * grid->find_buffer.len);

    float distance_total = 1.0f / Vector2DistanceSqr(start->world_position, target_position);

    FindPoint * wayfind = &grid->find_buffer.items[grid->width * start_y + start_x];
    wayfind->cost = -1.0f;
    wayfind->from = NULL;
    wayfind->visited = true;
    wayfind->queued = true;
    heapFindPointAppend(&heap, wayfind);

    isize neighbor_index[8] = {
        -grid->width - 1, -grid->width, 1 - grid->width,
        -1, 1,
        grid->width - 1, grid->width, grid->width + 1
    };

    while (heap.len > 0) {
        if (heapFindPointPop(&heap, &wayfind)) {
            goto failure;
        }
        usize index = (usize)wayfind - (usize)&grid->find_buffer.items[0];
        index /= sizeof(FindPoint);
        WayPoint * point = grid->waypoints.items[index];

        switch (target.type) {
            case NAV_TARGET_REGION: {
                if (target.region == point->graph->region) {
                    goto success;
                }
            } break;
            case NAV_TARGET_WAYPOINT: {
                if (target.waypoint == point) {
                    goto success;
                }
            } break;
        }

        FindPoint * find_point = &grid->find_buffer.items[index];
        find_point->queued = false;

        Vector2 direction = Vector2Subtract(target_position, point->world_position);
        direction = Vector2Normalize(direction);

        for (usize i = 0; i < 8; i++) {
            isize idx = index + neighbor_index[i];

            if (idx < 0) {
                continue;
            }
            if ( (usize) idx >= grid->waypoints.len) {
                continue;
            }

            WayPoint * neighbor = grid->waypoints.items[idx];
            if (neighbor == NULL) {
                continue;
            }
            if (target.approach_only) {
                switch (target.type) {
                    case NAV_TARGET_REGION: {
                        if (neighbor->graph->region == target.region) {
                            goto success;
                        }
                    } break;
                    case NAV_TARGET_WAYPOINT: {
                        if (neighbor == target.waypoint) {
                            goto success;
                        }
                    } break;
                }
            }
            if (neighbor->blocked || neighbor->unit) {
                continue;
            }
            // we are on border between graphs, either bordering two regions if they're too close, or on border of path and region
            if (neighbor->graph != point->graph) {
                // borders between graphs of the same type are never valid
                if (neighbor->graph->type == point->graph->type) {
                    continue;
                }
                else {
                    Path * path;
                    Region * region;
                    if (point->graph->type == GRAPH_PATH) {
                        path = point->graph->path;
                        region = neighbor->graph->region;
                    }
                    else {
                        region = point->graph->region;
                        path = neighbor->graph->path;
                    }
                    // avoid using path that are too close to region they aren't intended to be used for traversal onto
                    if (path->region_a != region && path->region_b != region) {
                        continue;
                    }

                    Region * other;
                    switch (target.type) {
                        case NAV_TARGET_REGION: other = target.region; break;
                        case NAV_TARGET_WAYPOINT: other = target.waypoint->graph->region; break;
                    }
                    // test if the region is either one where we started from or intend to end up at
                    if (start->graph->region != region && other != region) {
                        continue;
                    }
                }
            }


            Vector2 neighbor_direction = Vector2Subtract(neighbor->world_position, point->world_position);
            neighbor_direction = Vector2Normalize(neighbor_direction);
            float dot = Vector2DotProduct(direction, neighbor_direction);
            float angle_cost = 1.0f - (dot + 1.0f) * 0.5f;

            float distance_cost = Vector2DistanceSqr(neighbor->world_position, target_position) * distance_total;
            float cost = distance_cost + angle_cost + find_point->cost;

            FindPoint * find = &grid->find_buffer.items[idx];

            if (find->visited) {
                if (cost < find->cost) {
                    find->cost = cost;
                    find->from = wayfind;
                    if (find->queued) {
                        usize found_index;
                        if (heapFindPointFind(&heap, find, &found_index, NULL)) {
                            TraceLog(LOG_ERROR, "Failed to find index of the waypoint in the heap");
                            goto failure;
                        }
                        if (heapFindPointUpdate(&heap, found_index, find)) {
                            TraceLog(LOG_ERROR, "Failed to update waypoint in the heap");
                            goto failure;
                        }
                    }
                    else {
                        find->queued = true;
                        if (heapFindPointAppend(&heap, find)) {
                            TraceLog(LOG_ERROR, "Failed to reappend waypoint to the heap");
                            goto failure;
                        }
                    }
                }
            }
            else {
                find->cost = cost;
                find->from = wayfind;
                find->visited = true;
                find->queued = true;
                if (heapFindPointAppend(&heap, find)) {
                    TraceLog(LOG_ERROR, "Failed to append waypoint find to the heap");
                    goto failure;
                }
            }
        }

    }

    failure:
    result->len = 0;
    return FAILURE;

    success:
    result->len = 0;

    while (wayfind) {
        usize point_index = (usize)wayfind - (usize)&grid->find_buffer.items[0];
        point_index /= sizeof(FindPoint);
        WayPoint * point = grid->waypoints.items[point_index];
        if (point == NULL) {
            TraceLog(LOG_ERROR, "For some reason, found point is null");
            goto failure;
        }
        if ( listWayPointAppend(result, point) ) {
            TraceLog(LOG_ERROR, "Failed to append waypoint to result list");
            goto failure;
        }
        wayfind = wayfind->from;
    }

    usize half = result->len / 2;

    for (usize i = 0; i < half; i++) {
        usize other_side = result->len - 1 - i;
        WayPoint * swap = result->items[other_side];
        result->items[other_side] = result->items[i];
        result->items[i] = swap;
    }
    return SUCCESS;
}

/* Debug *********************************************************************/
void nav_render (NavGraph * graph) {
    for (usize i = 0; i < graph->waypoints.len; i++) {
        WayPoint * point = graph->waypoints.items[i];
        if (point) {
            if (graph->type == GRAPH_REGION)
                DrawCircleV(point->world_position, 1.5f, DARKBLUE);
            else
                DrawCircleV(point->world_position, 3.0f, DARKBROWN);
        }
    }
}
