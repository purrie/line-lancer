#include <raylib.h>
#include <raymath.h>
#include "bridge.h"
#include "constants.h"
#include "std.h"


Bridge bridge_from_path (Path *const path) {
    Bridge result = {0};
    float progress = 0.0f;
    float length = path_length(path);
    float leftover = length / UNIT_SIZE;
    leftover = leftover - (usize)leftover;
    leftover = UNIT_SIZE * leftover * 0.5f;

    Vector2 point = path_start_point(path, path->region_a);
    OptionalVector2 start = path_follow(path, path->region_a, leftover);

    if (start.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to get initial point of a path");
        return result;
    }
    Vector2 calc = Vector2Subtract(point, start.value);
    point = Vector2Add(point, calc);
    progress = leftover;

    Node * node = MemAlloc(sizeof(Node));
    if (node == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate memory for starting node");
        return result;
    }
    node->previous = NULL;
    node->position = point;
    node->unit     = NULL;
    result.start = node;

    while (progress < length) {
        Node * next = MemAlloc(sizeof(Node));
        if (next == NULL) {
            TraceLog(LOG_ERROR, "Failed to allocate memory for next node");
            clean_up_bridge(&result);
            return result;
        }
        node->next = next;
        next->previous = node;
        next->unit = NULL;
        start = path_follow(path, path->region_a, progress);
        if (start.has_value == false) {
            clean_up_bridge(&result);
            TraceLog(LOG_ERROR, "Failed to create next point on a path");
            return result;
        }
        next->position = start.value;
        progress += UNIT_SIZE;
        node = next;
    }

    node->next = MemAlloc(sizeof(Node));
    if (node->next == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate the last node");
        clean_up_bridge(&result);
        return result;
    }

    start = path_follow(path, path->region_a, length - leftover);
    if (start.has_value == false) {
        TraceLog(LOG_ERROR, "Failed to get the last point of the path");
        clean_up_bridge(&result);
        return result;
    }
    point = path_start_point(path, path->region_b);
    calc = Vector2Subtract(point, start.value);
    point = Vector2Add(point, calc);

    node->next->previous = node;
    node = node->next;
    node->next = NULL;
    node->unit = NULL;
    node->position = point;

    result.end = node;

    return result;
}

Bridge bridge_building_and_path (Path *const path, Building *const building) {
    Bridge  result      = {0};
    Vector2 end_point   = path_start_point(path, building->region);
    Vector2 start_point = building->position;
    usize   length      = (usize)( Vector2Distance(end_point, start_point) / UNIT_SIZE );
    Node  * node        = MemAlloc(sizeof(Node));

    if (node == NULL) {
        TraceLog(LOG_ERROR, "Failed to allocate starting node for building path");
        return result;
    }

    result.start = node;
    node->previous = NULL;
    node->unit     = NULL;
    node->position = Vector2MoveTowards(start_point, end_point, UNIT_SIZE);
    start_point    = node->position;

    while (length --> 1) {
        Node * next = MemAlloc(sizeof(Node)) ;
        if (next == NULL) {
            clean_up_bridge(&result);
            TraceLog(LOG_ERROR, "Failed to allocate next node");
            return result;
        }

        next->previous = node;
        next->unit     = NULL;
        next->position = Vector2MoveTowards(start_point, end_point, UNIT_SIZE);

        node->next     = next;

        node = next;
        start_point = node->position;
    }

    node->next = NULL;
    result.end = node;

    return result;
}

Bridge bridge_castle_and_path (Path *const path, Castle *const castle) {
    Bridge result = {0};
    // TODO implement
    return result;
}

void clean_up_bridge(Bridge * b) {
    Node * n = b->start;
    while (n) {
        if (n->next) {
            n = n->next;
            n->previous->next = NULL;
            MemFree(n->previous);
            n->previous = NULL;
        } else {
            MemFree(n);
            n = NULL;
        }
    }
    b->start = NULL;
    b->end = NULL;
}
