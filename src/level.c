#include "std.h"
#include "level.h"
#include "math.h"
#include "alloc.h"
#include "constants.h"
#include "game.h"
#include "mesh.h"
#include "pathfinding.h"
#include "units.h"
#include "audio.h"
#include <raymath.h>

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
void create_sublines (
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
Result lines_bounds (const ListLine * lines, Rectangle * result) {
    if (lines->len == 0)
        return FAILURE;

    *result = (Rectangle){ lines->items[0].a.x, lines->items[0].a.y, 0, 0 };

    for (usize i = 0; i < lines->len; i++) {
        *result = RectangleEnvelop(*result, lines->items[i].b);
    }

    return SUCCESS;
}
Test lines_check_hit (const ListLine * lines, Vector2 point, float distance) {
    for (usize i = 0; i < lines->len; i++) {
        Line line = lines->items[i];

        float angle = Vector2AngleHorizon(Vector2Subtract(line.b, line.a));
        float length = Vector2DistanceSqr(line.a, line.b);
        Vector2 local_point = Vector2Subtract(point, line.a);
        Vector2 rotated = Vector2Rotate(local_point, -angle);
        float dist = rotated.y < 0.0f ? -rotated.y : rotated.y;

        if (rotated.x < -distance) continue;
        if (rotated.x * rotated.x > length + distance) continue;

        if (dist <= distance)
            return YES;
    }
    return NO;
}

void bevel_lines (ListLine * lines, usize resolution, float depth, bool enclosed) {
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
    ListLine beveled = listLineInit(lines_count, lines->mem);

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
Rectangle get_line_bounds (const Line line) {
    Rectangle result;
    result.x = line.a.x < line.b.x ? line.a.x : line.b.x;
    result.y = line.a.y < line.b.y ? line.a.y : line.b.y;
    result.width  = line.a.x > line.b.x ? line.a.x - line.b.x : line.b.x - line.a.x;
    result.height = line.a.y > line.b.y ? line.a.y - line.b.y : line.b.y - line.a.y;
    return result;
}
Rectangle get_lines_bounds (const ListLine lines) {
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
Rectangle area_bounds (const Area * area) {
    return get_lines_bounds(area->lines);
}
bool area_contains_point (const Area * area, const Vector2 point) {
    Rectangle aabb = area_bounds(area);
    if (CheckCollisionPointRec(point, aabb) == false) {
        return false;
    }

    ListVector2 intersections = listVector2Init(area->lines.len, temp_allocator());

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
Test area_line_intersects (const Area * area, Line line) {
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
    building->units_spawned = 0;
}
usize building_buy_cost (BuildingType type) {
    // @balance
    switch (type) {
        case BUILDING_FIGHTER:  return 10;
        case BUILDING_ARCHER:   return 15;
        case BUILDING_SUPPORT:  return 20;
        case BUILDING_SPECIAL:  return 40;
        case BUILDING_RESOURCE: return 15;
        default: {
            TraceLog(LOG_ERROR, "Attempted to get cost of unbuildable building type");
        } return 10000;
    }
}
usize building_upgrade_cost_raw (BuildingType type, usize level) {
    // @balance
    return building_buy_cost(type) * (level + 2);
}
usize building_upgrade_cost (const Building * building) {
    return building_upgrade_cost_raw(building->type, building->upgrades);
}
usize building_cost_to_spawn (const Building * building) {
    // @balance
    switch (building->type) {
        case BUILDING_EMPTY:
        case BUILDING_RESOURCE:
            return 0;
        case BUILDING_FIGHTER: return 1 + building->upgrades;
        case BUILDING_ARCHER:  return 1 + building->upgrades;
        case BUILDING_SUPPORT: return 1 + building->upgrades;
        case BUILDING_SPECIAL: return 2 * (building->upgrades + 1);
    }
    TraceLog(LOG_ERROR, "Attempted to get spawning cost from unhandled building: %s", building_type_to_string(building->type));
    return 0;
}
usize building_generated_income (const Building * building) {
    // @balance
    if (building->type == BUILDING_RESOURCE)
        return 3 * ( building->upgrades + 1) * ( building->upgrades + 1 );
    return 0;
}
float building_trigger_interval (const Building * building) {
    // @balance
    switch (building->type) {
        case BUILDING_EMPTY:
            return 0.0f;
            break;
        case BUILDING_RESOURCE:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 10.0f - building->upgrades;
                case FACTION_MAGES:   return 10.0f - building->upgrades;
            }
            break;
        case BUILDING_FIGHTER:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
            break;
        case BUILDING_ARCHER:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
            break;
        case BUILDING_SUPPORT:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 5.0f - building->upgrades;
                case FACTION_MAGES:   return 5.0f - building->upgrades;
            }
            break;
        case BUILDING_SPECIAL:
            switch (building->region->faction) {
                case FACTION_KNIGHTS: return 7.0f - building->upgrades;
                case FACTION_MAGES:   return 7.0f - building->upgrades;
            }
            break;
    }
    TraceLog(LOG_WARNING, "Attempted to obtain trigger interval from unhandled building: T: %s, F:%s", building_type_to_string(building->type), faction_to_string(building->region->faction));
    return 0.0f;
}
float building_size () {
    return 10.0f;
}
usize building_max_units (const Building * building) {
    // @balance
    if (building->type == BUILDING_EMPTY || building->type == BUILDING_RESOURCE)
        return 0;
    return 5 * (building->upgrades + 1);
}
Rectangle building_bounds (const Building * building) {
    Rectangle bounds = {0};
    float b_size       = building_size();
    bounds.x         = building->position.x - b_size * 0.5f;
    bounds.y         = building->position.y - b_size * 0.5f;
    bounds.width     = b_size;
    bounds.height    = b_size;
    return bounds;
}
Building * get_building_by_position (const Map * map, Vector2 position) {
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
const char * building_name (BuildingType building, FactionType faction, usize upgrade) {
    if (upgrade > 2) return "Max Level";

    switch (faction) {
        case FACTION_KNIGHTS:
            switch (building) {
                case BUILDING_FIGHTER:
                    switch (upgrade) {
                        case 0: return "Militia Barracks";
                        case 1: return "Fighters Guild";
                        case 2: return "Veteran Company";
                    }
                case BUILDING_ARCHER:
                    switch (upgrade) {
                        case 0: return "Archery Targets";
                        case 1: return "Shooting Range";
                        case 2: return "Bowmen School";
                    }
                case BUILDING_SUPPORT:
                    switch (upgrade) {
                        case 0: return "Shrine";
                        case 1: return "Temple";
                        case 2: return "Cathedral";
                    }
                case BUILDING_SPECIAL:
                    switch (upgrade) {
                        case 0: return "Horse Trainers";
                        case 1: return "Rider School";
                        case 2: return "Knight Mansion";
                    }
                case BUILDING_RESOURCE:
                    switch (upgrade) {
                        case 0: return "Farms";
                        case 1: return "Ranch";
                        case 2: return "Village";
                    }
                case BUILDING_EMPTY: return "";
            }
        case FACTION_MAGES:
            switch (building) {
                case BUILDING_FIGHTER:
                    switch (upgrade) {
                        case 0: return "Golem Mines";
                        case 1: return "Golem Workshop";
                        case 2: return "Golem Factoria";
                    }
                case BUILDING_ARCHER:
                    switch (upgrade) {
                        case 0: return "Mage's Study";
                        case 1: return "Magic School";
                        case 2: return "Magic University";
                    }
                case BUILDING_SUPPORT:
                    switch (upgrade) {
                        case 0: return "Summoning Circle";
                        case 1: return "Ritual Grounds";
                        case 2: return "Tempest Shrine";
                    }
                case BUILDING_SPECIAL:
                    switch (upgrade) {
                        case 0: return "Genie Tomb";
                        case 1: return "Genie Temple";
                        case 2: return "Genie Citadel";
                    }
                case BUILDING_RESOURCE:
                    switch (upgrade) {
                        case 0: return "Mana Crystal";
                        case 1: return "Mana Array";
                        case 2: return "Mana Cluster";
                    }
                case BUILDING_EMPTY: return "";
            }
    }
}
Texture2D building_image (const Assets * assets, FactionType faction, BuildingType building, usize level) {
    if (level > BUILDING_MAX_UPGRADES || faction > FACTION_LAST || building > BUILDING_TYPE_LAST)
        return (Texture2D){0};
    const BuildingSpriteSet * set = &assets->buildings[faction];
    switch (building) {
        case BUILDING_EMPTY: return (Texture2D) {0};
        case BUILDING_FIGHTER: return set->fighter[level];
        case BUILDING_ARCHER: return set->archer[level];
        case BUILDING_SUPPORT: return set->support[level];
        case BUILDING_SPECIAL: return set->special[level];
        case BUILDING_RESOURCE: return set->resource[level];
    }
}

/* Path Functions **********************************************************/
Result path_by_position (const Map * map, Vector2 position, Path ** result) {
    for (usize p = 0; p < map->paths.len; p++) {
        Path * path = &map->paths.items[p];
        if (lines_check_hit(&path->lines, position, PATH_THICKNESS)) {
            *result = path;
            return SUCCESS;
        }
    }
    return FAILURE;
}

Result path_clone (Path * dest, const Path * src) {
    dest->lines = listLineInit(src->lines.len, src->lines.mem);
    dest->lines.len = src->lines.len;
    copy_memory(dest->lines.items, src->lines.items, sizeof(Line) * src->lines.len);
    return SUCCESS;
}
void path_deinit (Path * path) {
    listWayPointDeinit(&path->nav_graph.waypoints);
    UnloadModel(path->model);
    listLineDeinit(&path->lines);
    clear_memory(path, sizeof(Path));
}

/* Region Functions **********************************************************/
void region_reset_unit_pathfinding (Region * region) {
    for (usize w = 0; w < region->nav_graph.waypoints.len; w++) {
        WayPoint * point = region->nav_graph.waypoints.items[w];
        if (point && point->unit && point->unit->player_owned == region->player_id) {
            point->unit->pathfind.len = 0;
        }
    }
}
void region_change_ownership (GameState * state, Region * region, usize player_id) {
    { // sounds
        usize local_player;
        if (get_local_player_index(state, &local_player)) {
            local_player = 1;
        }
        if (region->player_id == local_player) {
            play_sound(state->resources, SOUND_REGION_LOST);
        }
        else if (player_id == local_player) {
            play_sound(state->resources, SOUND_REGION_CONQUERED);
        }
    }

    region->player_id = player_id;
    region->faction = state->players.items[player_id].faction;
    region->active_path = region->paths.len;
    setup_unit_guardian(region);
    // @balance
    region->castle.health *= 0.2f;

    for (usize i = 0; i < region->buildings.len; i++) {
        Building * b = &region->buildings.items[i];
        demolish_building(b);
    }
}
Region * region_by_unit (const Unit * unit) {
    return unit->waypoint->graph->region;
}
Result region_by_position (const Map * map, Vector2 position, Region ** result) {
    for (usize r = 0; r < map->regions.len; r++) {
        Region * region = &map->regions.items[r];
        if (area_contains_point(&region->area, position)) {
            *result = region;
            return SUCCESS;
        }
    }
    return FAILURE;
}

Result region_clone (Region * dest, const Region * src) {
    dest->player_id = src->player_id;
    dest->faction = src->faction;

    dest->area.lines = listLineInit(src->area.lines.len, src->area.lines.mem);
    dest->area.lines.len = src->area.lines.len;
    copy_memory(dest->area.lines.items, src->area.lines.items, sizeof(Line) * src->area.lines.len);

    dest->nav_graph = (NavGraph){0};
    dest->castle = (Unit){0};
    dest->castle.position = src->castle.position;

    dest->buildings = listBuildingInit(src->buildings.len, src->buildings.mem);
    dest->buildings.len = src->buildings.len;
    for (usize b = 0; b < dest->buildings.len; b++) {
        Building * to = &dest->buildings.items[b];
        Building * from = &src->buildings.items[b];
        clear_memory(to, sizeof(Building));
        to->position = from->position;
        to->region = dest;
    }

    dest->paths = listPathPInit(src->paths.len, src->paths.mem);
    dest->active_path = src->paths.len;


    return SUCCESS;
}
void region_deinit (Region * region) {
    for (usize b = 0; b < region->buildings.len; b++) {
        TraceLog(LOG_DEBUG, "  Deiniting building nr %zu", b);
        Building * building = &region->buildings.items[b];
        listWayPointDeinit(&building->spawn_points);
    }
    TraceLog(LOG_DEBUG, "  Releasing Buildings");
    listBuildingDeinit(&region->buildings);
    TraceLog(LOG_DEBUG, "  Releasing Paths");
    listPathPDeinit(&region->paths);

    TraceLog(LOG_DEBUG, "  Releasing Effects");
    listMagicEffectDeinit(&region->castle.effects);
    TraceLog(LOG_DEBUG, "  Releasing Attacks");
    listAttackDeinit(&region->castle.incoming_attacks);

    TraceLog(LOG_DEBUG, "  Releasing Area");
    listLineDeinit(&region->area.lines);
    TraceLog(LOG_DEBUG, "  Unloading Models");
    UnloadModel(region->area.model);
    UnloadModel(region->area.outline);
    TraceLog(LOG_DEBUG, "  Deinitializing region nav grid");
    listWayPointDeinit(&region->nav_graph.waypoints);

    clear_memory(region, sizeof(Region));
}

/* Map Functions ***********************************************************/
float get_expected_income (const Map * map, usize player) {
    float income = 0.0f;

    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        if (region->player_id != player)
            continue;

        income += (float)REGION_INCOME / (float)REGION_INCOME_INTERVAL;
        for (usize b = 0; b < region->buildings.len; b++) {
            Building * building = &region->buildings.items[b];
            if (building->type == BUILDING_RESOURCE)
                income += ( (float)building_generated_income(building) / building_trigger_interval(building) );
        }
    }

    return income;
}
float get_expected_maintenance_cost (const Map * map, usize player) {
    float cost = 0.0f;

    for (usize i = 0; i < map->regions.len; i++) {
        Region * region = &map->regions.items[i];
        if (region->player_id != player)
            continue;

        for (usize b = 0; b < region->buildings.len; b++) {
            Building * building = &region->buildings.items[b];
            if (building->type == BUILDING_EMPTY)
                continue;
            if (building->type != BUILDING_RESOURCE)
                cost += ( (float)building_cost_to_spawn(building) / (float)building_trigger_interval(building) );
        }
    }

    return cost;
}
Region * map_get_region_at (const Map * map, Vector2 point) {
    for (usize r = 0; r < map->regions.len; r++) {
        Region * region = &map->regions.items[r];
        if (area_contains_point(&region->area, point)) {
            return region;
        }
    }
    return NULL;
}

void render_map (Map * map) {
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
    }
    for (usize i = 0; i < map->paths.len; i++) {
        ListLine * lines = &map->paths.items[i].lines;
        for (usize l = 0; l < lines->len; l++) {
            DrawLineV(lines->items[l].a, lines->items[l].b, ORANGE);
        }
    }
}
void render_map_mesh (const GameState * state) {
    Shader shader = state->map.background.materials[0].shader;
    int time_loc = GetShaderLocation(shader, "time");
    float time = GetTime();
    SetShaderValue(shader, time_loc, &time, SHADER_UNIFORM_FLOAT);

    DrawModel(state->map.background, Vector3Zero(), 1.0f, WHITE);
    for (usize i = 0; i < state->map.regions.len; i++) {
        const Region * region = &state->map.regions.items[i];
        DrawModel(region->area.model, Vector3Zero(), 1.0f, WHITE);
        DrawModel(region->area.outline, Vector3Zero(), 1.0f, get_player_color(region->player_id));

        const ListBuilding * buildings = &region->buildings;
        for (usize b = 0; b < buildings->len; b++) {
            Building * building = &buildings->items[b];
            const Texture2D * sprite = NULL;
            switch (building->type) {
                case BUILDING_EMPTY: {
                } break;
                case BUILDING_FIGHTER: {
                    sprite = &state->resources->buildings[region->faction].fighter[building->upgrades];
                } break;
                case BUILDING_ARCHER: {
                    sprite = &state->resources->buildings[region->faction].archer[building->upgrades];
                } break;
                case BUILDING_SUPPORT: {
                    sprite = &state->resources->buildings[region->faction].support[building->upgrades];
                } break;
                case BUILDING_SPECIAL: {
                    sprite = &state->resources->buildings[region->faction].special[building->upgrades];
                } break;
                case BUILDING_RESOURCE: {
                    sprite = &state->resources->buildings[region->faction].resource[building->upgrades];
                } break;
            }
            if (sprite != NULL) {
                Rectangle source = (Rectangle){ 0, 0, sprite->width, sprite->height };
                Rectangle destination = (Rectangle){
                    building->position.x,
                    building->position.y,
                    NAV_GRID_SIZE * 2,
                    NAV_GRID_SIZE * 2
                };
                Vector2 origin = (Vector2){ destination.width * 0.5f, destination.height * 0.5f };
                DrawTexturePro(*sprite, source, destination, origin, 0.0f, WHITE);
            }
            else {
                DrawCircleGradient(building->position.x, building->position.y, UNIT_SIZE, GRAY, (Color){ 255, 255, 255, 0 });
            }
        }

        if (region->active_path < region->paths.len && region->player_id) {
            Path * active_path = region->paths.items[region->active_path];
            Vector2 end = active_path->region_a == region ?
                active_path->lines.items[0].a :
                active_path->lines.items[active_path->lines.len - 1].b;
            Rectangle whole = {0, 0, state->resources->flag.width, state->resources->flag.height};
            Rectangle target = {end.x, end.y, NAV_GRID_SIZE, NAV_GRID_SIZE};
            Vector2 origin = {target.width * 0.5f, target.height * 0.5f};
            DrawTexturePro(state->resources->flag, whole, target, origin, 0, get_player_color(region->player_id));
        }

    }

    #ifdef RENDER_PATHS
    for (usize i = 0; i < state->map.paths.len; i++) {
        const Path * path = &state->map.paths.items[i];
        DrawModel(path->model, Vector3Zero(), 1.0f, ORANGE);
        #ifdef RENDER_PATHS_DEBUG
        for (usize l = 0; l < path->lines.len; l++) {
            Line line = path->lines.items[l];
            DrawLineEx(line.a, line.b, 3.0f, DARKPURPLE);
        }
        #endif
    }
    #endif

    #ifdef RENDER_NAV_GRID
    const Map * map = &state->map;
    for (usize p = 0; p < map->paths.len; p++) {
        Path * path = &map->paths.items[p];
        nav_render(&path->nav_graph);
    }
    for (usize r = 0; r < map->regions.len; r++) {
        Region * region = &map->regions.items[r];
        nav_render(&region->nav_graph);
    }
    #endif
}

void map_clamp (Map * map) {
    Vector2 map_size = { (float)map->width, (float)map->height };

    for (usize i = 0; i < map->regions.len; i++) {
        ListLine * region = &map->regions.items[i].area.lines;
        for (usize l = 0; l < region->len; l++) {
            region->items[l].a = Vector2Clamp(region->items[l].a, Vector2Zero(), map_size);
            region->items[l].b = Vector2Clamp(region->items[l].b, Vector2Zero(), map_size);
        }
    }
}
Result map_clone (Map * dest, const Map * src) {
    clear_memory(dest, sizeof(Map));
    dest->name = src->name;
    dest->width = src->width;
    dest->height = src->height;
    dest->player_count = src->player_count;
    dest->paths = listPathInit(src->paths.len, perm_allocator());
    dest->regions = listRegionInit(src->regions.len, perm_allocator());

    for (usize p = 0; p < src->paths.len; p++) {
        dest->paths.len ++;
        if (path_clone(&dest->paths.items[p], &src->paths.items[p])) {
            goto fail;
        }
        dest->paths.items[p].map = dest;
    }

    for (usize r = 0; r < src->regions.len; r++) {
        dest->regions.len ++;
        if (region_clone(&dest->regions.items[r], &src->regions.items[r])) {
            goto fail;
        }
        dest->regions.items[r].map = dest;
    }

    return SUCCESS;

    fail:
    map_deinit(dest);
    return FAILURE;
}
void map_deinit (Map * map) {
    UnloadModel(map->background);
    for (usize p = 0; p < map->paths.len; p++) {
        TraceLog(LOG_DEBUG, "Deinitializing path %zu", p);
        path_deinit(&map->paths.items[p]);
    }
    for (usize r = 0; r < map->regions.len; r++) {
        TraceLog(LOG_DEBUG, "Deinitializing region %zu", r);
        region_deinit(&map->regions.items[r]);
    }
    nav_deinit_global(&map->nav_grid);
    listPathDeinit(&map->paths);
    listRegionDeinit(&map->regions);
    clear_memory(map, sizeof(Map));
}
Result map_make_connections (Map * map) {
    TraceLog(LOG_INFO, "Connecting map");
    if (nav_init_global_grid(map)) {
        TraceLog(LOG_ERROR, "!Failed to initialize map nav grid");
        return FAILURE;
    }
    for (usize i = 0; i < map->regions.len; i++) {
        TraceLog(LOG_DEBUG, " Connecting region %zu", i);
        Region * region = &map->regions.items[i];

        if (nav_init_region(region)) {
            TraceLog(LOG_ERROR, "!Failed to initialize region %zu navigation grid", i);
            return FAILURE;
        }

        for (usize b = 0; b < region->buildings.len; b++) {
            TraceLog(LOG_DEBUG, "  Connecting building %zu", b);
            Building * building = &region->buildings.items[b];
            building->region = region;
            building->spawn_points = listWayPointInit(8, perm_allocator());
            WayPoint * point;

            if (nav_find_waypoint(&region->nav_graph, building->position, &point)) {
                TraceLog(LOG_ERROR, "!Failed to find navigation position for building nr %zu", b);
                return FAILURE;
            }
            if (point == NULL) {
                TraceLog(LOG_ERROR, "!The point the building %zu is at doesn't have nav grid coverage", b);
                return FAILURE;
            }
            if (point->blocked) {
                TraceLog(LOG_ERROR, "!The position of the building %zu overlaps with another one", b);
                return FAILURE;
            }

            building->position = point->world_position;
            point->blocked = true;
            if (nav_gather_points(point, &building->spawn_points)) {
                TraceLog(LOG_ERROR, "!Failed to gather spawn points");
                return FAILURE;
            }
        }

        WayPoint * point;
        TraceLog(LOG_DEBUG, "Castle position [%.1f, %.1f]", region->castle.position.x, region->castle.position.y);
        if (nav_find_waypoint(&region->nav_graph, region->castle.position, &point)) {
            TraceLog(LOG_ERROR, "!Failed to find position for the castle");
            return FAILURE;
        }
        if (point == NULL) {
            TraceLog(LOG_ERROR, "!Position of the castle is outside of region's nav grid");
            return FAILURE;
        }
        if (point->blocked) {
            TraceLog(LOG_ERROR, "!Position of the castle overlaps with a building");
            return FAILURE;
        }
        setup_unit_guardian(region);
        point->unit = &region->castle;
        region->castle.waypoint = point;
        region->castle.position = point->world_position;

        TraceLog(LOG_DEBUG, "+Completed connecting region %zu", i);
    }
    TraceLog(LOG_DEBUG, "Completed connecting regions, connecting paths to regions");

    // connect path <-> region
    for (usize i = 0; i < map->paths.len; i++) {
        TraceLog(LOG_DEBUG, " Connecting path %zu", i);
        Path * path = &map->paths.items[i];

        for (usize r = 0; r < map->regions.len; r++) {
            Region * region = &map->regions.items[r];

            Vector2 a = path->lines.items[0].a;
            Vector2 b = path->lines.items[path->lines.len - 1].b;

            if (area_contains_point(&region->area, a)) {
                path->region_a = region;
                TraceLog(LOG_DEBUG, "Connecting region %d to start of path %d", r, i);
            }
            if (area_contains_point(&region->area, b)) {
                path->region_b = region;
                TraceLog(LOG_DEBUG, "Connecting region %d to end of path %d", r, i);
            }

            if (path->region_a && path->region_b) {
                if (path->region_a == path->region_b) {
                    TraceLog(LOG_ERROR, "!Path has both ends in the same region");
                    return FAILURE;
                }
                if (nav_init_path(path)) {
                    TraceLog(LOG_ERROR, "!Failed to initialize path's navigation grid");
                    return FAILURE;
                }

                if (listPathPAppend(&path->region_a->paths, path)) {
                    TraceLog(LOG_ERROR, "!Failed to add path to region a");
                    return FAILURE;
                }
                if (listPathPAppend(&path->region_b->paths, path)) {

                    TraceLog(LOG_ERROR, "!Failed to add path to region b");
                    return FAILURE;
                }
                TraceLog(LOG_INFO, "Path connected");
                break;
            }
        }
    }
    TraceLog(LOG_INFO, "Map connected successfully");

    return SUCCESS;
}
void map_subdivide_paths (Map * map) {
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

    bevel_lines(&map->paths.items[i].lines, PATH_BEVEL, depth, false);
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
void map_apply_textures (const Assets * assets, Map * map) {
    for (usize i = 0; i < map->regions.len; i++) {
        Material * mat = map->regions.items[i].area.model.materials;
        SetMaterialTexture(mat, MATERIAL_MAP_DIFFUSE, assets->ground_texture);
    }
    Shader shader = assets->water_shader;
    float size[2] = { assets->water_texture.width, assets->water_texture.height };
    SetShaderValue(shader, GetShaderLocation(shader, "size"), &size, SHADER_ATTRIB_VEC2);
    /* SetShaderValueV(shader, GetShaderLocation(shader, "size"), &size, SHADER_ATTRIB_FLOAT, 2); */
    map->background.materials[0].shader = assets->water_shader;
    SetMaterialTexture(&map->background.materials[0], MATERIAL_MAP_DIFFUSE, assets->water_texture);
}
Result map_prepare_to_play (const Assets * assets, Map * map) {
  map_clamp(map);
  map_subdivide_paths(map);
  if(map_make_connections(map)) {
    return FAILURE;
  }
  generate_map_mesh(map);
  map_apply_textures(assets, map);
  return SUCCESS;
}
