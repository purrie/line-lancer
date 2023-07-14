#include <raylib.h>
#include <raymath.h>
#include "bridge.h"
#include "constants.h"
#include "std.h"

bool bridge_is_enemy_present (Bridge *const bridge, usize player) {
    Node * node = bridge->start;
    while (node->bridge == bridge) {
        if (node->unit && node->unit->player_owned != player)
            return true;
        node = node->next;
    }
    return false;
}

Result bridge_points (Vector2 a, Vector2 b, Bridge * result) {
    float step_size = UNIT_SIZE * 2.0f;
    float distance = Vector2Distance(a, b);
    if (distance < ( step_size * 2.0f )) {
        TraceLog(LOG_ERROR, "Distance between points is too short to create bridge");
        return FAILURE;
    }
    float offset = distance / step_size;
    usize count  = (usize)(offset + 0.5f) - 1;

    offset = offset - (usize)offset;
    offset = offset * 0.5f;
    offset = offset * step_size;

    Node * node = MemAlloc(sizeof(Node));
    if (node == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate starting node for building path");
        return FAILURE;
    }

    a = Vector2MoveTowards(a, b, step_size + offset);
    result->start  = node;
    node->previous = NULL;
    node->unit     = NULL;
    node->position = a;
    node->bridge = result;

    while (count --> 1) {
        Node * next = MemAlloc(sizeof(Node)) ;
        if (next == NULL) {
            bridge_deinit(result);
            TraceLog(LOG_ERROR, "Failed to allocate next node");
            return FAILURE;
        }

        a = Vector2MoveTowards(a, b, step_size);
        next->previous = node;
        next->unit     = NULL;
        next->position = a;
        next->bridge   = result;
        node->next     = next;

        node = next;
    }

    node->next = NULL;
    result->end = node;
    return SUCCESS;
}

Result bridge_nodes (Node *const a, Node *const b, Bridge * result) {
    if (bridge_points(a->position, b->position, result)) {
        return FAILURE;
    }
    result->start->previous = a;
    result->end->next = b;
    return SUCCESS;
}

Result bridge_building_and_path (Path *const path, Building * building) {
    if (building->spawn_paths.cap == building->spawn_paths.len) {
        if (listBridgeGrow(&building->spawn_paths, building->spawn_paths.cap + 1)) {
            return FAILURE;
        }
        if (listBridgeGrow(&building->defend_paths, building->defend_paths.cap + 1)) {
            return FAILURE;
        }
    }
    Bridge * direct_bridge = &building->spawn_paths.items[building->spawn_paths.len];
    Bridge * defens_bridge = &building->defend_paths.items[building->defend_paths.len];

    PathEntry * entry = region_path_entry(building->region, path);
    if (entry == NULL) {
        return FAILURE;
    }

    Node * destination = path_start_node(path, building->region, NULL);
    Node * defennation = entry->castle_path.end;

    Vector2 start_point = building->position;

    clear_memory(direct_bridge, sizeof(Bridge));
    clear_memory(defens_bridge, sizeof(Bridge));

    if (bridge_points(start_point, destination->position, direct_bridge)) {
        return FAILURE;
    }
    if (bridge_points(start_point, defennation->position, defens_bridge)) {
        return FAILURE;
    }

    direct_bridge->end->next = destination;
    defens_bridge->end->next = defennation;

    building->spawn_paths.len ++;
    building->defend_paths.len ++;

    return SUCCESS;
}

Result bridge_castle_and_path (PathEntry * path, Castle *const castle) {
    Node * start = path_start_node(path->path, castle->region, NULL);
    if (bridge_nodes(start, &castle->guardian_spot, &path->castle_path)) {
        return FAILURE;
    }
    path->castle_path.end = path->castle_path.end->previous;
    MemFree(path->castle_path.end->next);
    path->castle_path.end->next = &castle->guardian_spot;

    return SUCCESS;
}

Result bridge_region (Region * region) {
    for (usize p = 0; p < region->paths.len; p++) {
        PathEntry * from_entry = &region->paths.items[p];
        Path * from = from_entry->path;

        // connect path <-> path
        for (usize o = p + 1; o < region->paths.len; o++) {
            PathEntry * to_entry = &region->paths.items[o];
            Path * to = to_entry->path;

            Node * a = path_start_node(from, region, NULL);
            Node * b = path_start_node(to, region, NULL);

            PathBridge path = { to };
            path.bridge = MemAlloc(sizeof(Bridge));
            if(bridge_nodes(a, b, path.bridge)) {
                MemFree(path.bridge);
                TraceLog(LOG_ERROR, "Failed to bridge internal paths of a region");
                return FAILURE;
            }

            listPathBridgeAppend(&from_entry->redirects, path);
            path.to = from;
            listPathBridgeAppend(&to_entry->redirects, path);
        }

        if (bridge_castle_and_path(from_entry, &region->castle)) {
            TraceLog(LOG_ERROR, "Failed to bridge path and castle of a region");
            return FAILURE;
        }
    }

    for (usize f = 0; f < region->paths.len; f++) {
        PathEntry * from = &region->paths.items[f];
        Path * from_path = from->path;

        if (from->defensive_paths.cap < from->redirects.len) {
            if (listBridgeGrow(&from->defensive_paths, from->redirects.len)) {
                return FAILURE;
            }
        }

        for (usize t = 0; t < from->redirects.len; t++) {
            Path * to_path = from->redirects.items[t].to;
            PathEntry * to = region_path_entry(region, to_path);
            Node * start = path_start_node(from_path, region, NULL);
            Node * end   = to->castle_path.end;

            Bridge * bridge = &from->defensive_paths.items[t];
            clear_memory(bridge, sizeof(Bridge));
            if (bridge_points(start->position, end->position, bridge)) {
                return FAILURE;
            }

            bridge->end->next = end;
            from->defensive_paths.len = t + 1;
        }
    }

    for (usize b = 0; b < region->buildings.len; b++) {
        Building * building = &region->buildings.items[b];

        float distance = 9999.9f;
        for (usize p = 0; p < region->paths.len; p++) {
            Path * path = region->paths.items[p].path;

            float d = Vector2Distance(building->position, path_start_point(path, building->region));
            if (d < distance) {
                distance = d;
                building->active_spawn = p;
            }

            if (bridge_building_and_path(path, building)) {
                return FAILURE;
            }
            // TODO might be a good idea to bridge buildings and castle paths so spawned units go to defend the guardian when under attack
        }
    }

    return SUCCESS;
}

Result bridge_over_path (Path * path) {
    float step_size = UNIT_SIZE * 2.0f;
    TraceLog(LOG_INFO, "Bridging Path");
    float progress = 0.0f;
    float length = path_length(path);
    float leftover = length / step_size;
    leftover = leftover - (usize)leftover;
    leftover = leftover * step_size * 0.5f;

    Vector2 point;
    if (path_follow(path, path->region_a, leftover, &point)) {
        TraceLog(LOG_ERROR, "Failed to get initial point of a path");
        return FAILURE;
    }

    Node * node = MemAlloc(sizeof(Node));
    if (node == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for starting node");
        return FAILURE;
    }

    node->previous = NULL;
    node->unit     = NULL;
    node->position = point;
    node->bridge   = &path->bridge;
    path->bridge.start = node;

    progress = leftover + step_size;
    while (progress + step_size < length) {
        if (path_follow(path, path->region_a, progress, &point)) {
            bridge_deinit(&path->bridge);
            TraceLog(LOG_ERROR, "Failed to create next point on a path");
            return FAILURE;
        }

        Node * next = MemAlloc(sizeof(Node));
        if (next == NULL) {
            TraceLog(LOG_ERROR, "Failed to allocate memory for next node");
            bridge_deinit(&path->bridge);
            return FAILURE;
        }

        node->next = next;
        next->previous = node;
        next->unit = NULL;
        next->bridge = &path->bridge;


        next->position = point;
        progress += step_size;
        node = next;
    }

    node->next = MemAlloc(sizeof(Node));
    if (node->next == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate the last node");
        bridge_deinit(&path->bridge);
        return FAILURE;
    }

    if (path_follow(path, path->region_a, length - leftover, &point)) {
        TraceLog(LOG_ERROR, "Failed to get the last point of the path");
        bridge_deinit(&path->bridge);
        return FAILURE;
    }

    node->next->previous = node;
    node = node->next;
    node->bridge = &path->bridge;
    node->next = NULL;
    node->unit = NULL;
    node->position = point;

    path->bridge.end = node;

    return SUCCESS;
}

void bridge_deinit (Bridge * b) {
    if (b->start == NULL && b->end == NULL)
        return;
    Node * n = b->start;
    while (n != b->end) {
        n = n->next;
        MemFree(n->previous);
    }
    MemFree(b->end);
    b->start = NULL;
    b->end = NULL;
}

void render_bridge(Bridge *const b, Color mid, Color start, Color end) {
    #ifdef RENDER_DEBUG_BRIDGE
    DrawCircleV(b->start->position, 2.0f, start);
    DrawCircleV(b->end->position, 2.0f, end);
    Node * node = b->start;
    node = node->next;
    while (node != b->end) {
        DrawCircleV(node->position, 1.0f, mid);
        node = node->next;
    }
    #endif
}
