#include <raylib.h>
#include <raymath.h>
#include "bridge.h"
#include "constants.h"
#include "std.h"

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
            clean_up_bridge(result);
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
        listBridgeGrow(&building->spawn_paths, building->spawn_paths.cap + 1);
    }
    Bridge  * result      = &building->spawn_paths.items[building->spawn_paths.len];
    Node    * destination = path_start_node(path, building->region, NULL);
    Vector2   start_point = building->position;

    clear_memory(result, sizeof(Bridge));

    if (bridge_points(start_point, destination->position, result)) {
        return FAILURE;
    }

    result->end->next = destination;
    building->spawn_paths.len ++;

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
            clean_up_bridge(&path->bridge);
            TraceLog(LOG_ERROR, "Failed to create next point on a path");
            return FAILURE;
        }

        Node * next = MemAlloc(sizeof(Node));
        if (next == NULL) {
            TraceLog(LOG_ERROR, "Failed to allocate memory for next node");
            clean_up_bridge(&path->bridge);
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
        clean_up_bridge(&path->bridge);
        return FAILURE;
    }

    if (path_follow(path, path->region_a, length - leftover, &point)) {
        TraceLog(LOG_ERROR, "Failed to get the last point of the path");
        clean_up_bridge(&path->bridge);
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

void clean_up_bridge(Bridge * b) {
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
    DrawCircleV(b->start->position, 2.0f, start);
    DrawCircleV(b->end->position, 2.0f, end);
    Node * node = b->start;
    node = node->next;
    while (node != b->end) {
        DrawCircleV(node->position, 1.0f, mid);
        node = node->next;
    }
}
