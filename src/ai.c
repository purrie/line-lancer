#include "ai.h"
#include "alloc.h"
#include "std.h"
#include "constants.h"
#include "level.h"

int sort_regions_by_risk (Region ** a, Region ** b) {
    Region * aa = *a;
    Region * bb = *b;
    int a_bad_neighbors = 0;
    int b_bad_neighbors = 0;

    for (usize i = 0; i < aa->paths.len; i++) {
        Path * entry = aa->paths.items[i];
        if (entry->region_a->player_id != entry->region_b->player_id)
            a_bad_neighbors ++;
    }

    for (usize i = 0; i < bb->paths.len; i++) {
        Path * entry = bb->paths.items[i];
        if (entry->region_a->player_id != entry->region_b->player_id)
            b_bad_neighbors ++;
    }

    return a_bad_neighbors - b_bad_neighbors;
}

void redirect_region_paths (GameState * state, usize player_index, ListRegionP * ai_regions) {
    ListRegionP frontline = listRegionPInit(state->map.regions.len, temp_allocator());
    ListRegionP in_danger = listRegionPInit(state->map.regions.len, temp_allocator());
    ListRegionP safe_regions = listRegionPInit(state->map.regions.len, temp_allocator());
    usize player_seed = state->players.items[player_index].seed;

    for (usize i = 0; i < ai_regions->len; i++) {
        Region * region = ai_regions->items[i];

        for (usize g = 0; g < region->nav_graph.waypoints.len; g++) {
            WayPoint * point = region->nav_graph.waypoints.items[g];
            if (point && point->unit && point->unit->player_owned != player_index) {
                if (listRegionPAppend(&in_danger, region)) {
                    TraceLog(LOG_ERROR, "Failed to add region to endangered regions in ai calculation");
                }
                goto next_region;
            }
        }
        for (usize p = 0; p < region->paths.len; p++) {
            Path * path = region->paths.items[p];
            if (path->region_a->player_id != player_index || path->region_b->player_id != player_index) {
                if (listRegionPAppend(&frontline, region)) {
                    TraceLog(LOG_ERROR, "Failed to add region to frontline list in ai calculation");
                }
                goto next_region;
            }
        }
        if (listRegionPAppend(&safe_regions, region)) {
            TraceLog(LOG_ERROR, "Failed to add region to list of safe regions");
        }

        next_region: {}
    }

    // All regions in danger have disabled out routing so units can focus on defense
    for (usize d = 0; d < in_danger.len; d++) {
        Region * region = in_danger.items[d];
        if (region->active_path != region->paths.len) {
            region->active_path = region->paths.len;
            region_reset_unit_pathfinding(region);
        }
    }

    // Frontline regions send units to enemy regions
    for (usize f = 0; f < frontline.len; f++) {
        Region * region = frontline.items[f];
        usize index = (player_seed + state->turn / (FPS * 100)) % region->paths.len;
        while (1) {
            Path * path = region->paths.items[index];
            if (path->region_a->player_id != player_index || path->region_b->player_id != player_index) {
                break;
            }
            index = ( index + 1 ) % region->paths.len;
        }
        if (region->active_path != index) {
            region->active_path = index;
            region_reset_unit_pathfinding(region);
        }
    }

    // safe regions prioritize sending units to regions in danger, when neighboring none of those, they send units out at random
    for (usize s = 0; s < safe_regions.len; s++) {
        Region * region = safe_regions.items[s];
        Region * danger_zone = NULL;
        usize danger_index = 0;
        for (usize p = 0; p < region->paths.len; p++) {
            usize index = ( p + player_seed ) % region->paths.len;
            Path * path = region->paths.items[index];
            Region * other = path->region_a == region ? path->region_b : path->region_a;
            bool is_in_danger = false;
            for (usize d = 0; d < in_danger.len; d++) {
                Region * danger_test = in_danger.items[d];
                if (danger_test == other) {
                    is_in_danger = true;
                    break;
                }
            }
            if (is_in_danger) {
                danger_zone = other;
                danger_index = p;
            }
        }
        if (danger_zone) {
            if (region->active_path != danger_index) {
                region->active_path = danger_index;
                region_reset_unit_pathfinding(region);
            }
            goto next_safe;
        }
        usize time_offset = state->turn / (FPS * 60);
        usize index = (time_offset + player_seed) % ( region->paths.len + 1 );
        if (region->active_path != index) {
            region->active_path = index;
            region_reset_unit_pathfinding(region);
        }
    next_safe: {}
    }
}
void make_purchasing_decision (GameState * state, usize player_index, ListRegionP * ai_regions) {
    PlayerData * ai = &state->players.items[player_index];

    #if ( BUILDING_MAX_UPGRADES != 2 )
    #error "Expected max building level to be 2 for this AI to work correctly"
    #endif

    #ifdef DEBUG
    if (BUILDING_TYPE_LAST != BUILDING_RESOURCE) {
        TraceLog(LOG_FATAL, "We expect the resource building to be last to prevent out of bounds error");
        return;
    }
    #endif

    float income = get_expected_income(&state->map, player_index);
    float upkeep = get_expected_maintenance_cost(&state->map, player_index);

    BuildingType wanted_building = BUILDING_EMPTY;
    usize wanted_building_upgrade_level = 0;
    bool wanted_upgrade = false;

    usize buildings_empty_count = 0;
    usize buildings_unupgraded_count = 0;
    usize buildings_first_level_count = 0;
    /* usize buildings_second_level_count = 0; */
    usize buildings_total = 0;
    usize buildings_unupgraded_spread[BUILDING_TYPE_LAST + 1] = {0};
    usize buildings_first_level_spread[BUILDING_TYPE_LAST + 1] = {0};
    usize buildings_second_level_spread[BUILDING_TYPE_LAST + 1] = {0};


    for (usize r = 0; r < ai_regions->len; r++) {
        for (usize b = 0; b < ai_regions->items[r]->buildings.len; b++) {
            Building * building = &ai_regions->items[r]->buildings.items[b];

            switch (building->type) {
                case BUILDING_FIGHTER:
                case BUILDING_ARCHER:
                case BUILDING_SUPPORT:
                case BUILDING_SPECIAL:
                    buildings_total += 1;
                    goto count_individual;
                case BUILDING_RESOURCE:
                    count_individual:
                    switch(building->upgrades) {
                        case 0:
                            buildings_unupgraded_count += 1;
                            buildings_unupgraded_spread[building->type] += 1;
                            break;
                        case 1:
                            buildings_first_level_count += 1;
                            buildings_first_level_spread[building->type] += 1;
                            break;
                        case 2:
                            /* buildings_second_level_count += 1; */
                            buildings_second_level_spread[building->type] += 1;
                            break;
                    }
                    break;
                case BUILDING_EMPTY:
                    buildings_empty_count += 1;
            }
        }
    }

    // choose what to buy
    if (upkeep + 0.3f >= income) {
        wanted_building = BUILDING_RESOURCE;
        wanted_upgrade = buildings_unupgraded_spread[BUILDING_RESOURCE] || buildings_first_level_spread[BUILDING_RESOURCE];
        wanted_building_upgrade_level = buildings_unupgraded_spread[BUILDING_RESOURCE] == 0;
    }
    else {

        if (buildings_total == 0) {
            wanted_building = BUILDING_FIGHTER;
            goto building_chosen;
        }

        if (buildings_empty_count == 0) {
            if (buildings_unupgraded_count > 0 || buildings_first_level_count > 0) {
                wanted_upgrade = true;
            }
            else {
                // nothing to do
                return;
            }
        }
        // TODO after unit balance pass, ensure AI properly chooses when to upgrade vs when to buy

        if (wanted_upgrade == false) {
            BuildingType try_wanted = BUILDING_FIGHTER;
            float desired_spread[BUILDING_TYPE_LAST] = {0};
            desired_spread[BUILDING_FIGHTER] = 0.5f;
            desired_spread[BUILDING_ARCHER] = 0.3f;
            desired_spread[BUILDING_SUPPORT] = 0.1f;
            desired_spread[BUILDING_SPECIAL] = 0.1f;

            while (try_wanted != BUILDING_TYPE_LAST) {
                float spread = (buildings_unupgraded_spread[try_wanted] + buildings_first_level_spread[try_wanted] + buildings_second_level_spread[try_wanted]) / (float)buildings_total;
                if (spread < desired_spread[try_wanted]) {
                    wanted_building = try_wanted;
                    goto building_chosen;
                }
                try_wanted += 1;
            }
            wanted_building = BUILDING_SPECIAL;
            goto building_chosen;
        }

        wanted_building = BUILDING_FIGHTER;

        for (BuildingType try = BUILDING_ARCHER; try < BUILDING_TYPE_LAST; try ++) {
            if (buildings_unupgraded_spread[wanted_building] < buildings_unupgraded_spread[try]) {
                wanted_building = try;
            }
        }

        if (buildings_unupgraded_spread[wanted_building] == 0) {
            wanted_building = BUILDING_FIGHTER;
            for (BuildingType try = BUILDING_ARCHER; try < BUILDING_TYPE_LAST; try ++) {
                if (buildings_first_level_spread[wanted_building] < buildings_first_level_spread[try]) {
                    wanted_building = try;
                }
            }

            if (buildings_first_level_spread[wanted_building] == 0) {
                TraceLog(LOG_INFO, "Nothing to upgrade for AI #%zu", player_index);
                return;
            }
            wanted_building_upgrade_level = 1;
        }
        else {
            wanted_building_upgrade_level = 0;
        }
    }

    building_chosen:

    if (wanted_building == BUILDING_EMPTY)
        return;

    if (wanted_upgrade) {
        if (building_upgrade_cost_raw(wanted_building, 1) > ai->resource_gold) {
            return;
        }

        listRegionPBubblesort(ai_regions, &sort_regions_by_risk);
        for (usize i = 0; i < ai_regions->len; i++) {
            Region * region = ai_regions->items[i];
            for (usize b = 0; b < region->buildings.len; b++) {
                Building * building = &region->buildings.items[b];

                if (building->type != wanted_building || building->upgrades != wanted_building_upgrade_level) {
                    continue;
                }

                usize cost = building_upgrade_cost(building);
                if (ai->resource_gold < cost) {
                    continue;
                }

                upgrade_building(building);
                ai->resource_gold -= cost;
                return;
            }
        }
    }
    else {
        usize cost = building_buy_cost(wanted_building);
        if (cost > ai->resource_gold)
            return;

        listRegionPBubblesort(ai_regions, &sort_regions_by_risk);
        for (usize i = 0; i < ai_regions->len; i++) {
            Region * region = ai_regions->items[i];
            for (usize b = 0; b < region->buildings.len; b++) {
                Building * building = &region->buildings.items[b];

                if (building->type != BUILDING_EMPTY)
                    continue;

                place_building(building, wanted_building);
                ai->resource_gold -= cost;
                return;
            }
        }
        TraceLog(LOG_ERROR, "AI #%zu wanted to buy building %d but couldn't find empty spot for it", player_index, wanted_building);
    }

}

void simulate_ai (GameState * state) {
    if (state->turn % FPS != 0) {
        return;
    }
    ListRegionP ai_regions = listRegionPInit(state->map.regions.len, temp_allocator());

    for (usize i = 1; i < state->players.len; i++) {
        if (state->players.items[i].type != PLAYER_AI)
            continue;

        ai_regions.len = 0;
        for (usize r = 0; r < state->map.regions.len; r++) {
            Region * region = &state->map.regions.items[r];
            if (region->player_id == i) {
                listRegionPAppend(&ai_regions, region);
            }
        }
        if (ai_regions.len == 0)
            continue;

        make_purchasing_decision(state, i, &ai_regions);
        redirect_region_paths(state, i, &ai_regions);
    }
}
