#include "ai.h"
#include "alloc.h"
#include "std.h"
#include "constants.h"
#include "level.h"

void redirect_building_paths (GameState * state, usize player_index, ListRegionP * ai_regions) {
    (void)state;

    ListPathEntryP endangered_paths = listPathEntryPInit(6, &temp_alloc, NULL);
    ListPathEntryP hostile_paths = listPathEntryPInit(6, &temp_alloc, NULL);

    for (usize i = 0; i < ai_regions->len; i++) {
        Region * region = ai_regions->items[i];

        if (region->paths.len < 2)
            continue;

        endangered_paths.len = 0;
        hostile_paths.len = 0;

        for (usize p = 0; p < region->paths.len; p++) {
            PathEntry * entry = &region->paths.items[p];
            if (path_enemies_present(entry, player_index)) {
                listPathEntryPAppend(&endangered_paths, entry);
            }
            else if (entry->path->region_a->player_id != entry->path->region_b->player_id) {
                listPathEntryPAppend(&hostile_paths, entry);
            }
        }
        if (( endangered_paths.len + hostile_paths.len ) == 0) {
            for (usize p = 0; p < region->paths.len; p++) {
                PathEntry * entry = &region->paths.items[p];
                Region * other;

                if (entry->path->region_a == region) {
                    other = entry->path->region_b;
                }
                else {
                    other = entry->path->region_a;
                }

                for (usize r = 0; r < other->paths.len; r++) {
                    PathEntry * other_entry = &other->paths.items[r];
                    if (path_enemies_present(other_entry, player_index)) {
                        listPathEntryPAppend(&endangered_paths, entry);
                        break;
                    }
                    else if (other_entry->path->region_a->player_id != other_entry->path->region_b->player_id) {
                        listPathEntryPAppend(&hostile_paths, entry);
                        break;
                    }
                }
            }
        }

        if (( endangered_paths.len + hostile_paths.len ) > 0) {
            usize index = 0;
            for (usize b = 0; b < region->buildings.len; b++) {
                Building * building = &region->buildings.items[b];
                switch (building->type) {
                    case BUILDING_EMPTY:
                    case BUILDING_RESOURCE:
                        continue;
                    case BUILDING_FIGHTER:
                    case BUILDING_ARCHER:
                    case BUILDING_SUPPORT:
                    case BUILDING_SPECIAL: {
                        if (endangered_paths.len) {
                            Path * path = endangered_paths.items[index]->path;
                            building_set_spawn_path(building, path);
                            index = (index + 1) % endangered_paths.len;
                        }
                        else {
                            Path * path = hostile_paths.items[index]->path;
                            building_set_spawn_path(building, path);
                            index = (index + 1) % hostile_paths.len;
                        }
                    } break;
                }
            }
        }
        else {
            usize index = 0;
            for (usize b = 0; b < region->buildings.len; b++) {
                Building * building = &region->buildings.items[b];
                switch (building->type) {
                    case BUILDING_EMPTY:
                    case BUILDING_RESOURCE:
                        continue;
                    case BUILDING_FIGHTER:
                    case BUILDING_ARCHER:
                    case BUILDING_SUPPORT:
                    case BUILDING_SPECIAL: {
                        Path * path = region->paths.items[index].path;
                        building_set_spawn_path(building, path);
                        index = (index + 1) % region->paths.len;
                    } break;
                }
            }
        }

    }
}
void redirect_region_paths (GameState * state, usize player_index, ListRegionP * ai_regions) {
    (void)state;
    ListPathEntryP endangered_paths = listPathEntryPInit(6, &temp_alloc, NULL);
    ListPathEntryP hostile_paths = listPathEntryPInit(6, &temp_alloc, NULL);
    ListPathEntryP safe_paths = listPathEntryPInit(6, &temp_alloc, NULL);

    for (usize i = 0; i < ai_regions->len; i++) {
        Region * region = ai_regions->items[i];

        if (region->paths.len < 2)
            continue;

        endangered_paths.len = 0;
        hostile_paths.len = 0;
        safe_paths.len = 0;

        for (usize p = 0; p < region->paths.len; p++) {
            PathEntry * entry = &region->paths.items[p];
            if (path_enemies_present(entry, player_index)) {
                listPathEntryPAppend(&endangered_paths, entry);
            }
            else if (entry->path->region_a->player_id != entry->path->region_b->player_id) {
                listPathEntryPAppend(&hostile_paths, entry);
            }
            else {
                listPathEntryPAppend(&safe_paths, entry);
            }
        }

        if (safe_paths.len == 0)
            continue;

        if (endangered_paths.len + hostile_paths.len > 0) {
            usize hostile_index  = 0;

            hostile_index = 0;
            for (usize f = 0; f < safe_paths.len; f++) {
                Path * from = safe_paths.items[f]->path;
                Path * to = NULL;
                if (endangered_paths.len) {
                    to = endangered_paths.items[hostile_index]->path;
                    hostile_index = (hostile_index + 1) % endangered_paths.len;
                }
                else {
                    to = hostile_paths.items[hostile_index]->path;
                    hostile_index = (hostile_index + 1) % hostile_paths.len;
                }
                region_connect_paths(region, from, to);
            }
        }
        else {
            for (usize p = 0; p < region->paths.len; p++) {
                PathEntry * entry = &region->paths.items[p];
                Region * other;

                if (entry->path->region_a == region) {
                    other = entry->path->region_b;
                }
                else {
                    other = entry->path->region_a;
                }

                for (usize r = 0; r < other->paths.len; r++) {
                    PathEntry * other_entry = &other->paths.items[r];
                    if (other_entry->path->region_a->player_id != other_entry->path->region_b->player_id) {
                        listPathEntryPAppend(&endangered_paths, entry);
                        break;
                    }
                }
            }

            if (endangered_paths.len == 0) {
                for (usize p = 0; p < region->paths.len; p++) {
                    usize next = (p + 1) % region->paths.len;
                    region_connect_paths(region, region->paths.items[p].path, region->paths.items[next].path);
                }
            }
            else {
                usize next = 0;
                for (usize p = 0; p < region->paths.len; p++) {
                    Path * from = region->paths.items[p].path;
                    Path * to = endangered_paths.items[next]->path;
                    if (from == to) {
                        if (endangered_paths.len == 1)
                            continue;
                        next = (next + 1) % endangered_paths.len;
                        to = endangered_paths.items[next]->path;
                    }
                    region_connect_paths(region, from, to);
                    next = (next + 1) % endangered_paths.len;
                }
            }
        }
    }
}

int sort_regions_by_risk (Region ** a, Region ** b) {
    Region * aa = *a;
    Region * bb = *b;
    int a_bad_neighbors = 0;
    int b_bad_neighbors = 0;

    for (usize i = 0; i < aa->paths.len; i++) {
        PathEntry * entry = &aa->paths.items[i];
        if (entry->path->region_a->player_id != entry->path->region_b->player_id)
            a_bad_neighbors ++;
    }

    for (usize i = 0; i < bb->paths.len; i++) {
        PathEntry * entry = &bb->paths.items[i];
        if (entry->path->region_a->player_id != entry->path->region_b->player_id)
            b_bad_neighbors ++;
    }

    return a_bad_neighbors - b_bad_neighbors;
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
    bool wanted_upgrade = false;

    // choose what to buy
    if (upkeep + 0.3f > income) {
        wanted_building = BUILDING_RESOURCE;
    }
    else {
        usize buildings_empty_count = 0;
        usize buildings_unupgraded_count = 0;
        usize buildings_first_level_count = 0;
        usize buildings_second_level_count = 0;
        usize buildings_total = 0;
        usize buildings_unupgraded_spread[BUILDING_TYPE_LAST] = {0};
        usize buildings_first_level_spread[BUILDING_TYPE_LAST] = {0};
        usize buildings_second_level_spread[BUILDING_TYPE_LAST] = {0};


        for (usize r = 0; r < ai_regions->len; r++) {
            for (usize b = 0; b < ai_regions->items[r]->buildings.len; b++) {
                Building * building = &ai_regions->items[r]->buildings.items[b];

                switch (building->type) {
                    case BUILDING_FIGHTER:
                    case BUILDING_ARCHER:
                    case BUILDING_SUPPORT:
                    case BUILDING_SPECIAL:
                        buildings_total += 1;
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
                                buildings_second_level_count += 1;
                                buildings_second_level_spread[building->type] += 1;
                                break;
                        }
                        break;
                    case BUILDING_EMPTY:
                        buildings_empty_count += 1;
                    case BUILDING_RESOURCE: break;
                }
            }
        }

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
        }

        for (BuildingType try = BUILDING_FIGHTER; try < BUILDING_TYPE_LAST; try ++) {
            usize count = buildings_first_level_spread[try];
            for (BuildingType against = try + 1; against < BUILDING_TYPE_LAST; against ++) {
                usize against_count = buildings_first_level_spread[against];
                if (count < against_count)
                    goto next_first_try;
            }
            wanted_building = try;
            goto building_chosen;
            next_first_try: {}
        }

        for (BuildingType try = BUILDING_FIGHTER; try < BUILDING_TYPE_LAST; try ++) {
            usize count = buildings_second_level_spread[try];
            for (BuildingType against = try + 1; against < BUILDING_TYPE_LAST; against ++) {
                usize against_count = buildings_second_level_spread[against];
                if (count < against_count)
                    goto next_second_try;
            }
            wanted_building = try;
            goto building_chosen;
            next_second_try: {}
        }

        // fallback
        if (wanted_upgrade == false) {
            wanted_building = BUILDING_FIGHTER;
        }
        else {
            for (BuildingType try = BUILDING_FIGHTER; try < BUILDING_TYPE_LAST; try ++) {
                usize count = buildings_first_level_spread[try];
                if (count > 0) {
                    wanted_building = try;
                    goto building_chosen;
                }
            }
            for (BuildingType try = BUILDING_FIGHTER; try < BUILDING_TYPE_LAST; try ++) {
                usize count = buildings_second_level_spread[try];
                if (count > 0) {
                    wanted_building = try;
                    goto building_chosen;
                }
            }
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

                if (building->type != wanted_building || building->upgrades == BUILDING_MAX_UPGRADES) {
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
    }

}

void simulate_ai (GameState * state) {
    if (state->turn % FPS != 0) {
        return;
    }
    ListRegionP ai_regions = listRegionPInit(state->map.regions.len, &temp_alloc, NULL);

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
        redirect_building_paths(state, i, &ai_regions);
    }
}
