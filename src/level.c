#include "std.h"
#include "bridge.h"
#include "level.h"
#include "math.h"
#include "alloc.h"
#include "constants.h"
#include "game.h"
#include <raymath.h>

/* Building functions ********************************************************/
void place_building(Building * building, BuildingType type) {
    building->type = type;
    // TODO make proper building modification
    // update texture and whatnot
}

void upgrade_building(Building * building) {
    if (building->upgrades >= 2) return;

    building->upgrades ++;
    // TODO make proper building modification
}

void demolish_building (Building * building) {
    building->type = BUILDING_EMPTY;
    building->upgrades = 0;
}

/* Line Functions **********************************************************/
Result line_intersection (Line a, Line b, Vector2 * value) {
    const Vector2 sn = Vector2Subtract(a.a, a.b);
    const Vector2 wn = Vector2Subtract(b.a, b.b);
    const float d = sn.x * wn.y - sn.y * wn.x;
    if (FloatEquals(d, 0.0f)) {
        return FAILURE;
    }

    const float xd = a.a.x - b.a.x;
    const float yd = a.a.y - b.a.y;

    const float t = (xd * wn.y - yd * wn.x) / d;
    const float u = (xd * sn.y - yd * sn.x) / d;

    if (t < 0.0f || t > 1.0f) {
        return FAILURE;
    }
    if (u < 0.0f || u > 1.0f) {
        return FAILURE;
    }

    value->x = a.a.x + t * (a.b.x - a.a.x);
    value->y = a.a.y + t * (a.b.y - a.a.y);

    return SUCCESS;
}

Test line_intersects (Line a, Line b) {
    const Vector2 sn = Vector2Subtract(a.a, a.b);
    const Vector2 wn = Vector2Subtract(b.a, b.b);
    const float d = sn.x * wn.y - sn.y * wn.x;
    if (FloatEquals(d, 0.0f)) {
        return NO;
    }

    const float xd = a.a.x - b.a.x;
    const float yd = a.a.y - b.a.y;

    const float t = (xd * wn.y - yd * wn.x) / d;
    const float u = (xd * sn.y - yd * sn.x) / d;

    if (t < 0.0f || t > 1.0f) {
        return NO;
    }
    if (u < 0.0f || u > 1.0f) {
        return NO;
    }
    return YES;
}

usize lines_intersections (const ListLine lines, const Line line, ListVector2 * result) {
    usize added = 0;
    for (usize i = 0; i < lines.len; i++) {
        Vector2 value;

        if(line_intersection(lines.items[i], line, &value) == SUCCESS) {
            if (result == NULL) return true;

            bool contains = false;
            for (usize c = 0; c < result->len; c++) {
                if (Vector2Equals(value, result->items[c])) {
                    contains = true;
                    break;
                }
            }

            if (contains == false) {
                if (listVector2Append(result, value)) {
                    TraceLog(LOG_ERROR, "Failed to append to line intersections result list");
                }
                added++;
            }
        }
    }
    return added;
}

void create_sublines(
    ListLine * dest,
    const usize amount,
    const Vector2 from,
    const Vector2 to,
    const Vector2 control
) {
    const float step = 1.0f / (float)amount;
    Vector2 last = from;

    for (usize i = 1; i < amount; i++) {
        const float t = step * (float)i;
        const float nt = 1.0f - t;
        const Vector2 a = Vector2Scale(from, powf(nt, 2.0f));
        const Vector2 b = Vector2Scale(to, powf(t, 2.0f));
        const Vector2 c = Vector2Scale(control, 2.0f * t * nt);
        const Vector2 result = Vector2Add(a, Vector2Add(b, c));
        const Line line = { last, result };
        listLineAppend(dest, line);
        last = result;
    }

    const Line line = { last, to };
    listLineAppend(dest, line);
}

void bevel_lines(ListLine * lines, usize resolution, float depth, bool enclosed) {
    if (enclosed) {
        if (lines->len < 3) {
            TraceLog(LOG_ERROR, "Enclosed lines need to have at least 3 segments");
            return;
        }
    }
    else {
        if (lines->len < 2) {
            TraceLog(LOG_ERROR, "There needs to be at least two line segments");
            return;
        }
    }
    if (resolution == 0) {
        TraceLog(LOG_ERROR, "Resolution needs to be larger than zero for bevel to have any effect");
        return;
    }
    if (depth < 1.0f) {
        TraceLog(LOG_ERROR, "Cannot have depth smaller than 1.0f");
        return;
    }

    usize lines_count = lines->len * resolution + lines->len;
    ListLine beveled = listLineInit(lines_count, lines->alloc, lines->dealloc);

    Vector2 a, b, c;
    const usize last = lines->len - 1;

    {
        Line first = lines->items[0];
        Line second = lines->items[1];

        if (enclosed)
            a = Vector2MoveTowards(first.a, first.b, depth);
        else
            a = first.a;
        b = Vector2MoveTowards(first.b, first.a, depth);
        c = Vector2MoveTowards(second.a, second.b, depth);

        listLineAppend(&beveled, (Line) { a, b });
        create_sublines(&beveled, resolution, b, c, first.b);

        a = c;
    }

    for (usize i = 1; i < last; i++) {
        b = Vector2MoveTowards(lines->items[i].b, lines->items[i].a, depth);
        c = Vector2MoveTowards(lines->items[i+1].a, lines->items[i+1].b, depth);
        listLineAppend(&beveled, (Line) { a, b });
        create_sublines(&beveled, resolution, b, c, lines->items[i].b);
        a = c;
    }

    if (enclosed) {
        b = Vector2MoveTowards(lines->items[last].b, lines->items[last].a, depth);
        c = beveled.items[0].a;
        listLineAppend(&beveled, (Line) { a, b });
        create_sublines(&beveled, resolution, b, c, lines->items[last].b);
    }
    else {
        b = lines->items[last].b;
        listLineAppend(&beveled, (Line) { a, b });
    }

    listLineDeinit(lines);
    (*lines) = beveled;
}

Rectangle get_line_bounds(const Line line) {
    Rectangle result;
    result.x = line.a.x < line.b.x ? line.a.x : line.b.x;
    result.y = line.a.y < line.b.y ? line.a.y : line.b.y;
    result.width  = line.a.x > line.b.x ? line.a.x - line.b.x : line.b.x - line.a.x;
    result.height = line.a.y > line.b.y ? line.a.y - line.b.y : line.b.y - line.a.y;
    return result;
}

Rectangle get_lines_bounds(const ListLine lines) {
    Rectangle result = {0};
    if (lines.len == 0) return result;

    result = get_line_bounds(lines.items[0]);

    for (usize i = 1; i < lines.len; i++) {
        Rectangle bounds = get_line_bounds(lines.items[i]);
        result = RectangleUnion(result, bounds);
    }

    return result;
}

/* Area Functions **********************************************************/
Rectangle area_bounds(const Area *const area) {
    return get_lines_bounds(area->lines);
}

bool area_contains_point(const Area *const area, const Vector2 point) {
    Rectangle aabb = area_bounds(area);
    if (CheckCollisionPointRec(point, aabb) == false) {
        return false;
    }

    ListVector2 intersections = listVector2Init(area->lines.len, &temp_alloc, NULL);

    Vector2 a = { aabb.x - aabb.width, aabb.y - aabb.height };
    Line line = { a, point };
    bool contains = false;

    if (lines_intersections(area->lines, line, &intersections)) {
        if ((intersections.len % 2) != 0) {
            contains = true;
        }
    }

    return contains;
}

Test area_line_intersects (Area *const area, Line line) {
    for (usize i = 0; i < area->lines.len; i++) {
        Line t = area->lines.items[i];
        if (line_intersects(t, line)) {
            return YES;
        }
    }
    Line last = { area->lines.items[area->lines.len - 1].b, area->lines.items[0].a };
    if (line_intersects(last, line))
        return YES;
    return NO;
}

/* Building Functions ******************************************************/
float building_size() {
    return 10.0f;
}

Rectangle building_bounds(Building *const building) {
    Rectangle bounds = {0};
    float size       = building_size();
    bounds.x         = building->position.x - size * 0.5f;
    bounds.y         = building->position.y - size * 0.5f;
    bounds.width     = size;
    bounds.height    = size;
    return bounds;
}

Building * get_building_by_position(Map *const map, Vector2 position) {
    for (usize r = 0; r < map->regions.len; r++) {
        ListBuilding * buildings = &map->regions.items[r].buildings;

        for (usize b = 0; b < buildings->len; b++) {
            Rectangle aabb = building_bounds(&buildings->items[b]);

            if (CheckCollisionPointRec(position, aabb)) {
                return &buildings->items[b];
            }
        }
    }

    return NULL;
}

/* Path Functions **********************************************************/
Path * path_on_point (Map *const map, Vector2 point, Movement * direction) {
    for (usize i = 0; i < map->paths.len; i++) {
        Path * path = &map->paths.items[i];
        if (CheckCollisionPointCircle(point, path->bridge.start->position, 10.0f)) {
            if (direction)
                *direction = MOVEMENT_DIR_FORWARD;
            return path;
        }
        if (CheckCollisionPointCircle(point, path->bridge.end->position, 10.0f)) {
            if (direction)
                *direction = MOVEMENT_DIR_BACKWARD;
            return path;
        }
    }
    if (direction)
        *direction = MOVEMENT_INVALID;
    return NULL;
}

Vector2 path_start_point(Path *const path, Region *const from) {
    if (path->region_a == from) {
        return path->lines.items[0].a;
    }
    else if (path->region_b == from) {
        usize last = path->lines.len - 1;
        return path->lines.items[last].b;
    }
    else {
        TraceLog(LOG_ERROR, "Tried to get point start for region that isn't connected to path");
        return Vector2Zero();
    }
}

Node * path_start_node (Path *const path, Region *const from, Movement * direction_forward) {
    if (path->region_a == from) {
        if (direction_forward) {
            *direction_forward = MOVEMENT_DIR_FORWARD;
        }
        return path->bridge.start;
    }
    else if (path->region_b == from) {
        if (direction_forward) {
            *direction_forward = MOVEMENT_DIR_BACKWARD;
        }
        return path->bridge.end;
    }
    else
        return NULL;
}

Result path_follow(Path *const path, Region *const from, float distance, Vector2 * value) {
    if (path->region_a == NULL || path->region_b == NULL || path->region_a == path->region_b) {
        TraceLog(LOG_ERROR, "Path is not correctly initialized");
        return FAILURE;
    }

    float progress = 0.0f;
    bool reverse = from == path->region_b;
    for (usize i = 0; i < path->lines.len; i++) {
        usize index = reverse ? path->lines.len - i - 1 : i;

        Vector2 a = path->lines.items[index].a;
        Vector2 b = path->lines.items[index].b;

        float length = Vector2Distance(a, b);
        if (progress + length > distance) {
            float t = ((progress + length) - distance) / length;
            if (reverse) {
                // TODO this seems wrong
                *value = Vector2Lerp(a, b, t);
            }
            else {
                *value = Vector2Lerp(b, a, t);
            }
            return SUCCESS;
        }
        progress += length;
    }

    return FAILURE;
}

Region * path_end_region(Path *const path, Region *const from) {
    return path->region_a == from ? path->region_b : path->region_a;
}

float path_length(Path *const path) {
    float sum = 0.0f;
    for (usize i = 0; i < path->lines.len; i++) {
        float d = Vector2Distance(path->lines.items[i].a, path->lines.items[i].b);
        sum += d;
    }

    return sum;
}

/* Region Functions **********************************************************/
void region_update_paths (Region * region) {
    for (usize e = 0; e < region->paths.len; e++) {
        PathEntry * entry = &region->paths.items[e];
        Movement dir;
        Node * start = path_start_node(entry->path, region, &dir);
        Node * connect;
        bool is_owner_same = entry->path->region_a->player_id == entry->path->region_b->player_id;

        if (is_owner_same) {
            Node * s = entry->redirects.items[entry->active_redirect].bridge->start;
            Node * e = entry->redirects.items[entry->active_redirect].bridge->end;

            connect = Vector2DistanceSqr(start->position, s->position) >
                      Vector2DistanceSqr(start->position, e->position) ?
                      e : s;
        }
        else
            connect = entry->castle_path.start;

        if (dir == MOVEMENT_DIR_FORWARD)
            start->previous = connect;
        else
            start->next = connect;

        if (is_owner_same) {
            usize next = (e + 1) % region->paths.len;
            entry->castle_path.end->next = region->paths.items[next].castle_path.end;
        }
        else {
            entry->castle_path.end->next = &region->castle.guardian_spot;
        }
    }
}

void region_change_ownership (Region * region, usize player_id) {
    region->player_id = player_id;
    region_update_paths(region);
    for (usize i = 0; i < region->paths.len; i++) {
        Region * other;
        PathEntry * entry = &region->paths.items[i];
        if (entry->path->region_a == region) {
            other = entry->path->region_b;
        }
        else {
            other = entry->path->region_a;
        }
        region_update_paths(other);
    }
    unit_guardian(region);

    for (usize i = 0; i < region->buildings.len; i++) {
        Building * b = &region->buildings.items[i];
        demolish_building(b);
    }
}

Region * region_by_guardian (ListRegion *const regions, Unit *const guardian) {
    for (usize i = 0; i < regions->len; i++) {
        Region * region = &regions->items[i];
        if (&region->castle.guardian == guardian) {
            return region;
        }
    }
    TraceLog(LOG_ERROR, "Attempted to get region from a non-guardian unit");
    return NULL;
}

/* Map Functions ***********************************************************/
void map_clamp(Map * map) {
    Vector2 map_size = { (float)map->width, (float)map->height };

    for (usize i = 0; i < map->regions.len; i++) {
        ListLine * region = &map->regions.items[i].area.lines;
        for (usize l = 0; l < region->len; l++) {
            region->items[l].a = Vector2Clamp(region->items[l].a, Vector2Zero(), map_size);
            region->items[l].b = Vector2Clamp(region->items[l].b, Vector2Zero(), map_size);
        }
    }
}

void render_map(Map * map) {
    for (usize i = 0; i < map->regions.len; i++) {
        ListLine * lines = &map->regions.items[i].area.lines;
        DrawCircleV(lines->items[0].a, 2.0f, BLUE);
        DrawCircleV(lines->items[0].b, 2.0f, RED);
        for (usize l = 0; l < lines->len; l++) {
            DrawLineV(lines->items[l].a, lines->items[l].b, BLUE);
        }
        ListBuilding * buildings = &map->regions.items[i].buildings;
        for (usize b = 0; b < buildings->len; b++) {
            DrawCircleV(buildings->items[b].position, 20.0f, BLUE);
        }
        Vector2 castle = map->regions.items[i].castle.position;
        /* TraceLog(LOG_INFO, "Castle pos = [%.2f, %.2f]", castle.x, castle.y); */
        DrawCircleV(castle, 30.0f, RED);
    }
    for (usize i = 0; i < map->paths.len; i++) {
        ListLine * lines = &map->paths.items[i].lines;
        for (usize l = 0; l < lines->len; l++) {
            DrawLineV(lines->items[l].a, lines->items[l].b, ORANGE);
        }
    }
}

void render_map_mesh(Map * map) {
    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        DrawModel(region->area.model, Vector3Zero(), 1.0f, WHITE);

        for (usize l = 0; l < region->area.lines.len; l++) {
            Line line = region->area.lines.items[l];

            Vector2 grow = Vector2Subtract(line.a, line.b);
            grow = Vector2Normalize(grow);
            grow = Vector2Scale(grow, 0.5f);
            line.a = Vector2Add(line.a, grow);
            grow = Vector2Scale(grow, -1.0f);
            line.b = Vector2Add(line.b, grow);

            DrawLineEx(line.a, line.b, 4.0f, get_player_color(region->player_id));
        }

        ListBuilding * buildings = &region->buildings;
        for (usize b = 0; b < buildings->len; b++) {
            Building * building = &buildings->items[b];
            render_bridge(&building->spawn_paths.items[building->active_spawn], BLACK, RED, BLUE);
            switch (building->type) {
                case BUILDING_EMPTY: {
                    DrawModel(building->model, Vector3Zero(), 1.0f, BLUE);
                } break;
                case BUILDING_FIGHTER: {
                    DrawModel(building->model, Vector3Zero(), 1.0f, DARKBLUE);
                } break;
                case BUILDING_ARCHER: {
                    DrawModel(building->model, Vector3Zero(), 1.0f, BROWN);
                } break;
                case BUILDING_SUPPORT: {
                    DrawModel(building->model, Vector3Zero(), 1.0f, DARKGREEN);
                } break;
                case BUILDING_SPECIAL: {
                    DrawModel(building->model, Vector3Zero(), 1.0f, DARKPURPLE);
                } break;
                case BUILDING_RESOURCE: {
                    DrawModel(building->model, Vector3Zero(), 1.0f, YELLOW);
                } break;
            }
        }

        DrawModel(region->castle.model, Vector3Zero(), 1.0f, RED);

        for (usize e = 0; e < region->paths.len; e++) {
            PathEntry * entry = &region->paths.items[e];
            if (entry->path->region_a->player_id == entry->path->region_b->player_id) {
                render_bridge(entry->redirects.items[entry->active_redirect].bridge, BLACK, RED, BLUE);
            }
            else {
                render_bridge(&entry->castle_path, BLACK, RED, BLUE);
            }
        }
    }

    #ifdef RENDER_PATHS
    for (usize i = 0; i < map->paths.len; i++) {
        Path * path = &map->paths.items[i];
        DrawModel(path->model, Vector3Zero(), 1.0f, ORANGE);

        render_bridge(&path->bridge, BLACK, RED, BLUE);
    }
    #endif
}

