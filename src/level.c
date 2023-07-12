#include "std.h"
#include "bridge.h"
#include "level.h"
#include "math.h"
#include "alloc.h"
#include "constants.h"
#include "game.h"
#include "mesh.h"
#include <raymath.h>

/* Building functions ********************************************************/
void place_building (Building * building, BuildingType type) {
    building->type = type;
    // TODO make proper building modification
    // update texture and whatnot
}

void upgrade_building (Building * building) {
    if (building->upgrades >= BUILDING_MAX_UPGRADES) return;

    building->upgrades ++;
    // TODO make proper building modification
}

void demolish_building (Building * building) {
    building->type = BUILDING_EMPTY;
    building->upgrades = 0;
}

usize building_buy_cost (BuildingType type) {
    switch (type) {
        case BUILDING_FIGHTER:  return 10;
        case BUILDING_ARCHER:   return 15;
        case BUILDING_SUPPORT:  return 15;
        case BUILDING_SPECIAL:  return 20;
        case BUILDING_RESOURCE: return 10;
        default: {
            TraceLog(LOG_ERROR, "Attempted to get cost of unbuildable building type");
        } return 10000;
    }
}

usize building_upgrade_cost_raw (BuildingType type, usize level) {
    return building_buy_cost(type) * (level + 2);
}

usize building_upgrade_cost (Building *const building) {
    return building_upgrade_cost_raw(building->type, building->upgrades);
}

usize building_cost_to_spawn (Building *const building) {
    switch (building->type) {
        case BUILDING_EMPTY:
        case BUILDING_RESOURCE:
            return 0;
        case BUILDING_ARCHER:
        case BUILDING_FIGHTER:
        case BUILDING_SUPPORT:
        case BUILDING_SPECIAL:
            return 1 + building->upgrades;
        default:
            TraceLog(LOG_ERROR, "Attempted to get unit spawn cost from unhandled building type");
            return 0;
    }
}

usize building_generated_income (Building *const building) {
    if (building->type == BUILDING_RESOURCE)
        return 3 * ( building->upgrades + 1);
    return 0;
}

float building_trigger_interval (Building *const building) {
    switch (building->type) {
        case BUILDING_TYPE_COUNT:
        case BUILDING_EMPTY:
            break;
        case BUILDING_RESOURCE:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 10.0f - building->upgrades;
                case FACTION_MAGES:   return 10.0f - building->upgrades;
            }
        case BUILDING_FIGHTER:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
        case BUILDING_ARCHER:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
        case BUILDING_SUPPORT:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
        case BUILDING_SPECIAL:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
    }
    TraceLog(LOG_WARNING, "Attempted to obtain spawn interval from non-spawning building");
    return 0.0f;
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

Result building_set_spawn_path (Building * building, Path *const path) {
    for (usize i = 0; i < building->spawn_paths.len; i++) {
        Bridge bridge = building->spawn_paths.items[i];
        if (bridge.end->next == path->bridge.end || bridge.end->next == path->bridge.start) {
            building->active_spawn = i;
            return SUCCESS;
        }
    }
    return FAILURE;
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

Result path_clone (Path * dest, Path *const src) {
    dest->lines = listLineInit(src->lines.len, src->lines.alloc, src->lines.dealloc);
    dest->lines.len = src->lines.len;
    copy_memory(dest->lines.items, src->lines.items, sizeof(Line) * src->lines.len);
    return SUCCESS;
}

void path_deinit (Path * path) {
    bridge_deinit(&path->bridge);
    UnloadModel(path->model);
    listLineDeinit(&path->lines);
    clear_memory(path, sizeof(Path));
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

void region_change_ownership (GameState * state, Region * region, usize player_id) {
    region->player_id = player_id;
    region->faction = state->players.items[player_id].faction;
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
    setup_unit_guardian(region);

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

Result region_connect_paths (Region * region, Path * from, Path * to) {
    if (from->region_a->player_id != from->region_b->player_id)
        return FAILURE;

    for (usize f = 0; f < region->paths.len; f++) {
        PathEntry * entry = &region->paths.items[f];

        if (entry->path != from) {
            continue;
        }

        for (usize s = 0; s < entry->redirects.len; s++) {
            if (entry->redirects.items[s].to == to) {
                entry->active_redirect = s;
                Movement dir;
                Node * node = path_start_node(entry->path, region, &dir);
                Node * next = NULL;
                Node * start = entry->redirects.items[s].bridge->start;

                if (start->previous == node) {
                    next = start;
                }
                else {
                    next = entry->redirects.items[s].bridge->end;
                }

                if (dir == MOVEMENT_DIR_FORWARD) {
                    node->previous = next;
                }
                else {
                    node->next = next;
                }

                return SUCCESS;
            }
        }
        break;
    }
    return FAILURE;
}

Result region_clone (Region * dest, Region *const src) {
    dest->player_id = src->player_id;
    dest->faction = src->faction;

    dest->area.lines = listLineInit(src->area.lines.len, src->area.lines.alloc, src->area.lines.dealloc);
    dest->area.lines.len = src->area.lines.len;
    copy_memory(dest->area.lines.items, src->area.lines.items, sizeof(Line) * src->area.lines.len);

    dest->castle.position = src->castle.position;
    dest->castle.region = dest;

    dest->castle.guardian_spot.position = src->castle.guardian_spot.position;
    dest->castle.guardian_spot.unit = &dest->castle.guardian;

    dest->buildings = listBuildingInit(src->buildings.len, src->buildings.alloc, src->buildings.dealloc);
    dest->buildings.len = src->buildings.len;
    for (usize b = 0; b < dest->buildings.len; b++) {
        Building * to = &dest->buildings.items[b];
        Building * from = &src->buildings.items[b];
        clear_memory(to, sizeof(Building));
        to->position = from->position;
        to->region = dest;
    }

    dest->paths = listPathEntryInit(src->paths.len, src->paths.alloc, src->paths.dealloc);

    return SUCCESS;
}

void region_deinit (Region * region) {
    for (usize e = 0; e < region->paths.len; e++) {
        PathEntry * entry = &region->paths.items[e];
        bridge_deinit(&entry->castle_path);
        for (usize r = 0; r < entry->redirects.len; r++) {
            PathBridge * pb = &entry->redirects.items[r];
            if (pb->bridge->start) {
                bridge_deinit(pb->bridge);
            }
            else {
                MemFree(pb->bridge);
            }
        }
        listPathBridgeDeinit(&entry->redirects);
    }
    listPathEntryDeinit(&region->paths);

    for (usize b = 0; b < region->buildings.len; b++) {
        Building * building = &region->buildings.items[b];
        UnloadModel(building->model);
        for (usize p = 0; p < building->spawn_paths.len; p++) {
            bridge_deinit(&building->spawn_paths.items[p]);
        }
        listBridgeDeinit(&building->spawn_paths);
    }
    listBuildingDeinit(&region->buildings);

    listLineDeinit(&region->area.lines);
    UnloadModel(region->area.model);

    UnloadModel(region->castle.model);

    clear_memory(region, sizeof(Region));
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
                case BUILDING_TYPE_COUNT: {
                    TraceLog(LOG_ERROR, "Building count is not a valid building type to render");
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

size get_expected_income (Map *const map, usize player) {
    float income = 0.0f;

    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        if (region->player_id != player)
            continue;

        income += (float)REGION_INCOME;
        for (usize b = 0; b < region->buildings.len; b++) {
            Building * building = &region->buildings.items[b];
            if (building->type == BUILDING_RESOURCE)
                income += ( building_generated_income(building) / building_trigger_interval(building) ) * REGION_INCOME_INTERVAL;
            else if (building->type != BUILDING_EMPTY)
                income -= ( building_cost_to_spawn(building) / building_trigger_interval(building) ) * REGION_INCOME_INTERVAL;
        }
    }

    return (size)income;
}

Region * map_get_region_at (Map *const map, Vector2 point) {
    for (usize r = 0; r < map->regions.len; r++) {
        Region * region = &map->regions.items[r];
        if (area_contains_point(&region->area, point)) {
            return region;
        }
    }
    return NULL;
}

void map_deinit (Map * map) {
    for (usize p = 0; p < map->paths.len; p++) {
        path_deinit(&map->paths.items[p]);
    }
    for (usize r = 0; r < map->regions.len; r++) {
        region_deinit(&map->regions.items[r]);
    }
    listPathDeinit(&map->paths);
    listRegionDeinit(&map->regions);
    clear_memory(map, sizeof(Map));
}

Result map_clone (Map * dest, Map *const src) {
    clear_memory(dest, sizeof(Map));
    dest->name = src->name;
    dest->width = src->width;
    dest->height = src->height;
    dest->player_count = src->player_count;
    dest->paths = listPathInit(src->paths.len, &MemAlloc, &MemFree);
    dest->regions = listRegionInit(src->regions.len, &MemAlloc, &MemFree);

    for (usize p = 0; p < src->paths.len; p++) {
        dest->paths.len ++;
        if (path_clone(&dest->paths.items[p], &src->paths.items[p])) {
            goto fail;
        }
    }

    for (usize r = 0; r < src->regions.len; r++) {
        dest->regions.len ++;
        if (region_clone(&dest->regions.items[r], &src->regions.items[r])) {
            goto fail;
        }
    }

    return SUCCESS;

    fail:
    map_deinit(dest);
    return FAILURE;
}

Result map_make_connections(Map * map) {
  TraceLog(LOG_INFO, "Connecting map");
  // connect path <-> region
  for (usize i = 0; i < map->paths.len; i++) {
    Path * path = &map->paths.items[i];

    for (usize r = 0; r < map->regions.len; r++) {
      Region * region = &map->regions.items[r];

      Vector2 a = path->lines.items[0].a;
      Vector2 b = path->lines.items[path->lines.len - 1].b;

      if (area_contains_point(&region->area, a)) {
        path->region_a = region;
        TraceLog(LOG_INFO, "Connecting region %d to start of path %d", r, i);
      }
      if (area_contains_point(&region->area, b)) {
        path->region_b = region;
        TraceLog(LOG_INFO, "Connecting region %d to end of path %d", r, i);
      }

      if (path->region_a && path->region_b) {
        if (path->region_a == path->region_b) {
          return FAILURE;
        }

        if (bridge_over_path(path)) {
          return FAILURE;
        }

        PathEntry a = { .path = path, .redirects = listPathBridgeInit(6, &MemAlloc, &MemFree) };
        PathEntry b = { .path = path, .redirects = listPathBridgeInit(6, &MemAlloc, &MemFree) };
        listPathEntryAppend(&path->region_a->paths, a);
        listPathEntryAppend(&path->region_b->paths, b);
        TraceLog(LOG_INFO, "Path connected");
        break;
      }
    }
  }

  for (usize i = 0; i < map->regions.len; i++) {
    Region * region = &map->regions.items[i];

    for (usize b = 0; b < region->buildings.len; b++) {
      region->buildings.items[b].region = region;
      region->buildings.items[b].spawn_paths = listBridgeInit(region->paths.len, &MemAlloc, &MemFree);
    }

    region->castle.region = region;

    if (bridge_region(region)) {
      return FAILURE;
    }

    region_update_paths(region);
    setup_unit_guardian(region);
  }

  return SUCCESS;
}

void map_subdivide_paths(Map * map) {
  TraceLog(LOG_INFO, "Smoothing map meshes");
  for(usize i = 0; i < map->paths.len; i++) {
    Vector2 a = map->paths.items[i].lines.items[0].a;
    Vector2 b = map->paths.items[i].lines.items[0].b;
    float depth = Vector2DistanceSqr(a, b);
    for (usize l = 1; l < map->paths.items[i].lines.len; l++) {
      a = map->paths.items[i].lines.items[l].a;
      b = map->paths.items[i].lines.items[l].b;
      float test = Vector2DistanceSqr(a, b);
      if (test < depth) depth = test;
    }
    depth = sqrtf(depth) * 0.25f;

    bevel_lines(&map->paths.items[i].lines, MAP_BEVEL, depth, false);
  }

  for (usize i = 0; i < map->regions.len; i++) {
    Vector2 a = map->regions.items[i].area.lines.items[0].a;
    Vector2 b = map->regions.items[i].area.lines.items[0].b;
    float depth = Vector2DistanceSqr(a, b);
    for (usize l = 1; l < map->regions.items[i].area.lines.len; l++) {
      a = map->regions.items[i].area.lines.items[l].a;
      b = map->regions.items[i].area.lines.items[l].b;
      float test = Vector2DistanceSqr(a, b);
      if (test < depth) depth = test;
    }
    depth = sqrtf(depth) * 0.25f;

    bevel_lines(&map->regions.items[i].area.lines, MAP_BEVEL, depth, true);
  }
}

Result map_prepare_to_play (Map * map) {
  if(map_make_connections(map)) {
    return FAILURE;
  }
  map_clamp(map);
  map_subdivide_paths(map);
  generate_map_mesh(map);
  return SUCCESS;
}
