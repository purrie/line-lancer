#include <assert.h>
#include "std.h"
#include "level.h"
#include "math.h"

implementList(Region, Region)
implementList(Path, Path)
implementList(Building, Building)
implementList(Line, Line)

OptionalVector2 get_line_intersection (Line a, Line b) {
    OptionalVector2 ret = {0};

    const Vector2 sn = Vector2Subtract(a.a, a.b);
    const Vector2 wn = Vector2Subtract(b.a, b.b);
    const float d = sn.x * wn.y - sn.y * wn.x;
    if (FloatEquals(d, 0.0f)) {
        return ret;
    }

    const float xd = a.a.x - b.a.x;
    const float yd = a.a.y - b.a.y;

    const float t = (xd * wn.y - yd * wn.x) / d;
    const float u = (xd * sn.y - yd * sn.x) / d;

    if (t < 0.0f || t > 1.0f) {
        return ret;
    }
    if (u < 0.0f || u > 1.0f) {
        return ret;
    }

    ret.has_value = true;
    ret.value.x = a.a.x + t * (a.b.x - a.a.x);
    ret.value.y = a.a.y + t * (a.b.y - a.a.y);

    return ret;
}

bool get_lines_intersections (const ListLine lines, const Line line, ListVector2 * result) {
    for (usize i = 0; i < lines.len; i++) {
        OptionalVector2 r = get_line_intersection(lines.items[i], line);

        if (r.has_value) {
            if (result == NULL) return true;

            bool contains = false;
            for (usize c = 0; c < result->len; c++) {
                if (Vector2Equals(r.value, result->items[c])) {
                    contains = true;
                    break;
                }
            }

            if (contains == false) {
                listVector2Append(result, r.value);
            }
        }
    }
    if (result == NULL || result->len == 0) return false;
    return true;
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
        assert(lines->len > 2 && "Enclosed lines need to have at least 3 segments");
    }
    else {
        assert(lines->len > 1 && "There needs to be at least two line segments");
    }
    assert(resolution > 0);
    assert(depth > 1.0f && "Cannot have depth smaller than 1.0f");

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

Rectangle area_bounds(const Area *const area) {
    return get_lines_bounds(area->lines);
}

bool area_contains_point(const Area *const area, const Vector2 point) {
    Rectangle aabb = area_bounds(area);
    if (CheckCollisionPointRec(point, aabb) == false) {
        return false;
    }

    Vector2 intersections_raw[area->lines.len];
    ListVector2 intersections = {0};
    intersections.items = &intersections_raw[0];
    intersections.cap = area->lines.len;

    Vector2 a = { aabb.x - aabb.width, aabb.y - aabb.height };
    Line line = { a, point };

    if (get_lines_intersections(area->lines, line, &intersections)) {
        if ((intersections.len % 2) == 0) {
            return false;
        } else {
            return true;
        }
    }

    return false;
}

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
        DrawModel(map->regions.items[i].area.model, Vector3Zero(), 1.0f, PURPLE);

        ListBuilding * buildings = &map->regions.items[i].buildings;
        for (usize b = 0; b < buildings->len; b++) {
            DrawModel(buildings->items[b].model, Vector3Zero(), 1.0f, BLUE);
        }

        DrawModel(map->regions.items[i].castle.model, Vector3Zero(), 1.0f, RED);
    }

    for (usize i = 0; i < map->paths.len; i++) {
        DrawModel(map->paths.items[i].model, Vector3Zero(), 1.0f, ORANGE);
    }
}

Camera2D setup_camera(Map * map) {
    Camera2D cam = {0};
    Vector2 map_size;
    float screen_w = GetScreenWidth();
    float screen_h = GetScreenHeight();
    map_size.x = ( screen_w - 10.0f ) / (float)map->width;
    map_size.y = ( screen_h - 10.0f ) / (float)map->height;
    cam.zoom = (map_size.x < map_size.y) ? map_size.x : map_size.y;
    if (map_size.x > map_size.y) {
        cam.offset.x = (1.0f - (map_size.y / map_size.x)) * screen_w * 0.5f;
    }
    if (map_size.y > map_size.x) {
        cam.offset.y = (1.0f - (map_size.x / map_size.y)) * screen_h * 0.5f;
    }
    cam.offset.x += 5.0f;
    cam.offset.y += 5.0f;

    return cam;
}
