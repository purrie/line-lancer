#include "level.h"

implementList(Region, Region)
implementList(Path, Path)
implementList(Building, Building)
implementList(Line, Line)


void map_deinit(Map * map) {
    for (usize i = 0; i < map->regions.len; i++) {
        listLineDeinit(&map->regions.items[i].area.lines);
        listBuildingDeinit(&map->regions.items[i].buildings);
    }
    for (usize i = 0; i < map->paths.len; i++) {
        listLineDeinit(&map->paths.items[i].lines);
    }
    listPathDeinit(&map->paths);
    listRegionDeinit(&map->regions);
}

void map_resize(Map * map, usize size_x, usize size_y) {
    float x = (float)size_x / (float)map->width;
    float y = (float)size_y / (float)map->height;

    for (usize r = 0; r < map->regions.len; r++) {
        ListLine * region = &map->regions.items[r].area.lines;
        for (usize l = 0; l < region->len; l++) {
            region->items[l].a.x *= x;
            region->items[l].a.y *= y;
            region->items[l].b.x *= x;
            region->items[l].b.y *= y;
        }
        ListBuilding * building = &map->regions.items[r].buildings;
        for (usize b = 0; b < building->len; b++) {
            building->items[b].position.x *= x;
            building->items[b].position.y *= y;
        }
        map->regions.items[r].castle.position.x *= x;
        map->regions.items[r].castle.position.y *= y;
    }

    for (usize p = 0; p < map->paths.len; p++) {
        ListLine * path = &map->paths.items[p].lines;
        for (usize l = 0; l < path->len; l++) {
            path->items[l].a.x *= x;
            path->items[l].a.y *= y;
            path->items[l].b.x *= x;
            path->items[l].b.y *= y;
        }
    }
}

void map_transl(Map * map, int pos_x, int pos_y) {
    float x = (float)pos_x;
    float y = (float)pos_y;

    for (usize r = 0; r < map->regions.len; r++) {
        ListLine * region = &map->regions.items[r].area.lines;
        for (usize l = 0; l < region->len; l++) {
            region->items[l].a.x += x;
            region->items[l].a.y += y;
            region->items[l].b.x += x;
            region->items[l].b.y += y;
        }
        ListBuilding * building = &map->regions.items[r].buildings;
        for (usize b = 0; b < building->len; b++) {
            building->items[b].position.x += x;
            building->items[b].position.y += y;
        }
        map->regions.items[r].castle.position.x += x;
        map->regions.items[r].castle.position.y += y;
    }

    for (usize p = 0; p < map->paths.len; p++) {
        ListLine * path = &map->paths.items[p].lines;
        for (usize l = 0; l < path->len; l++) {
            path->items[l].a.x += x;
            path->items[l].a.y += y;
            path->items[l].b.x += x;
            path->items[l].b.y += y;
        }
    }
}
