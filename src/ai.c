#include "ai.h"
#include "alloc.h"
#include "std.h"
#include "constants.h"
#include "level.h"

void redirect_building_paths (GameState * state, usize player_index, ListRegionP * ai_regions) {
    for (usize i = 0; i < ai_regions->len; i++) {
        Region * region = ai_regions->items[i];

        if (region->paths.len < 2)
            continue;

        usize hostile_paths_count = 0;
        PathEntry ** hostile_paths = temp_alloc(sizeof(PathEntry*) * region->paths.len);

        for (usize p = 0; p < region->paths.len; p++) {
            PathEntry * entry = &region->paths.items[p];
            if (entry->path->region_a->player_id != entry->path->region_b->player_id) {
                hostile_paths[hostile_paths_count++] = entry;
            }
        }

        if (hostile_paths_count) {
            usize index = 0;
            for (usize b = 0; b < region->buildings.len; b++) {
                Building * building = &region->buildings.items[b];
                switch (building->type) {
                    case BUILDING_TYPE_COUNT:
                    case BUILDING_EMPTY:
                    case BUILDING_RESOURCE:
                        continue;
                    case BUILDING_FIGHTER:
                    case BUILDING_ARCHER:
                    case BUILDING_SUPPORT:
                    case BUILDING_SPECIAL: {
                        building_set_spawn_path(building, hostile_paths[index]->path);
                        index = (index + 1) % hostile_paths_count;
                    } break;
                }
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

                bool is_in_danger = false;
                for (usize r = 0; r < other->paths.len; r++) {
                    PathEntry * other_entry = &other->paths.items[r];
                    if (other_entry->path->region_a->player_id != other_entry->path->region_b->player_id) {
                        is_in_danger = true;
                        break;
                    }
                }

                if (is_in_danger) {
                    hostile_paths[hostile_paths_count++] = entry;
                }
            }

            if (hostile_paths_count) {
                usize index = 0;
                for (usize b = 0; b < region->buildings.len; b++) {
                    Building * building = &region->buildings.items[b];
                    switch (building->type) {
                        case BUILDING_TYPE_COUNT:
                        case BUILDING_EMPTY:
                        case BUILDING_RESOURCE:
                            continue;
                        case BUILDING_FIGHTER:
                        case BUILDING_ARCHER:
                        case BUILDING_SUPPORT:
                        case BUILDING_SPECIAL: {
                            building_set_spawn_path(building, hostile_paths[index]->path);
                            index = (index + 1) % hostile_paths_count;
                        } break;
                    }
                }
            }
            else {
                usize index = 0;
                for (usize b = 0; b < region->buildings.len; b++) {
                    Building * building = &region->buildings.items[b];
                    switch (building->type) {
                        case BUILDING_TYPE_COUNT:
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
}

void redirect_region_paths (GameState * state, usize player_index, ListRegionP * ai_regions) {
    // TODO maybe this function doesn't need to run unless state of region ownership has changed?
    for (usize i = 0; i < ai_regions->len; i++) {
        Region * region = ai_regions->items[i];

        if (region->paths.len < 2)
            continue;

        usize hostile_paths = 0;
        for (usize p = 0; p < region->paths.len; p++) {
            PathEntry * entry = &region->paths.items[p];
            if (entry->path->region_a->player_id != entry->path->region_b->player_id) {
                hostile_paths ++;
            }
        }

        if (hostile_paths == region->paths.len) {
            continue;
        }

        if (hostile_paths) {
            usize friendly_paths = region->paths.len - hostile_paths;
            usize friendly_index = 0;
            usize hostile_index  = 0;

            PathEntry ** friendly = temp_alloc(sizeof(PathEntry*) * friendly_paths);
            PathEntry ** hostile  = temp_alloc(sizeof(PathEntry*) * hostile_paths);

            for (usize p = 0; p < region->paths.len; p++) {
                PathEntry * entry = &region->paths.items[p];
                if (entry->path->region_a->player_id != entry->path->region_b->player_id) {
                    hostile[hostile_index++] = entry;
                }
                else {
                    friendly[friendly_index++] = entry;
                }
            }

            hostile_index = 0;
            for (usize f = 0; f < friendly_paths; f++) {
                region_connect_paths(region, friendly[f]->path, hostile[hostile_index]->path);
                hostile_index = (hostile_index + 1) % hostile_paths;
            }
        }
        else {

            usize endangered_regions = 0;
            PathEntry ** endangered = temp_alloc(sizeof(PathEntry*) * region->paths.len);

            for (usize p = 0; p < region->paths.len; p++) {
                PathEntry * entry = &region->paths.items[p];
                Region * other;

                if (entry->path->region_a == region) {
                    other = entry->path->region_b;
                }
                else {
                    other = entry->path->region_a;
                }
                usize danger_level = 0;

                for (usize r = 0; r < other->paths.len; r++) {
                    PathEntry * other_entry = &other->paths.items[r];
                    if (other_entry->path->region_a->player_id != other_entry->path->region_b->player_id) {
                        danger_level = 1;
                        break;
                    }
                }

                if (danger_level) {
                    endangered[endangered_regions] = entry;
                    endangered_regions ++;
                }
            }

            if (endangered_regions == 0) {
                for (usize p = 0; p < region->paths.len; p++) {
                    usize next = (p + 1) % region->paths.len;
                    region_connect_paths(region, region->paths.items[p].path, region->paths.items[next].path);
                }
            }
            else {
                usize next = 0;
                for (usize p = 0; p < region->paths.len; p++) {
                    Path * from = region->paths.items[p].path;
                    Path * to = endangered[next]->path;
                    if (from == to) {
                        if (endangered_regions == 1)
                            continue;
                        next = (next + 1) % endangered_regions;

                    }
                    region_connect_paths(region, region->paths.items[p].path, region->paths.items[next].path);
                    next = (next + 1) % endangered_regions;
                }
            }
        }
    }
}

void make_purchasing_decision (GameState * state, usize player_index, ListRegionP * ai_regions) {
    PlayerData * ai = &state->players.items[player_index];

    bool can_afford_something = ai->resource_gold >= 10;

    if (can_afford_something) {
        usize * spread = temp_alloc(sizeof(usize) * BUILDING_TYPE_COUNT);
        usize * buildings_first_level = temp_alloc(sizeof(usize) * BUILDING_TYPE_COUNT);
        usize * buildings_unupgraded = temp_alloc(sizeof(usize) * BUILDING_TYPE_COUNT);

        clear_memory(spread, sizeof(usize) * BUILDING_TYPE_COUNT);
        clear_memory(buildings_first_level, sizeof(usize) * BUILDING_TYPE_COUNT);
        clear_memory(buildings_unupgraded, sizeof(usize) * BUILDING_TYPE_COUNT);

        #if ( BUILDING_MAX_UPGRADES != 2 )
        #error "Expected max building level to be 2 for this AI to work correctly"
        #endif

        usize resource_upgrades = 0;
        usize combat_buildings = 0;
        usize combat_upgrades = 0;
        usize combat_raw = 0;

        for (usize r = 0; r < ai_regions->len; r++) {
            for (usize b = 0; b < ai_regions->items[r]->buildings.len; b++) {
                Building * building = &ai_regions->items[r]->buildings.items[b];
                switch (building->type) {
                    case BUILDING_TYPE_COUNT:
                        continue;
                    case BUILDING_RESOURCE:
                        resource_upgrades += building->upgrades;
                    case BUILDING_EMPTY:
                        break;
                    case BUILDING_FIGHTER:
                    case BUILDING_ARCHER:
                    case BUILDING_SUPPORT:
                    case BUILDING_SPECIAL:
                        combat_buildings += 1;
                        if (building->upgrades) {
                            if (building->upgrades != BUILDING_MAX_UPGRADES)
                                buildings_first_level[building->type] += 1;
                            combat_upgrades += building->upgrades;
                        }
                        else {
                            buildings_unupgraded[building->type] += 1;
                            combat_raw += 1;
                        }
                        break;
                }
                spread[building->type] ++;
            }
        }

        BuildingType wanted_building = BUILDING_EMPTY;
        bool want_to_build;
        size income = get_expected_income(state->current_map, player_index);


        if (income < 1) {
            // TODO this is not ideal, improve by calculating wanted income in relation to expense

            wanted_building = BUILDING_RESOURCE;
            TraceLog(LOG_INFO, "AI wants to build resource building");

            if (( buildings_unupgraded[BUILDING_RESOURCE] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_RESOURCE, 0)) ||
                ( buildings_first_level[BUILDING_RESOURCE] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_RESOURCE, 1))) {
                want_to_build = false;
                TraceLog(LOG_INFO, "AI decided to upgrade");
            }
            else if (spread[BUILDING_EMPTY] && ai->resource_gold >= building_buy_cost(BUILDING_RESOURCE)) {
                want_to_build = true;
                TraceLog(LOG_INFO, "AI decided to buy");
            }
            else {
                TraceLog(LOG_INFO, "AI can't afford resources");
                return;
            }
        }
        else {
            bool can_build = spread[BUILDING_EMPTY] > 0;
            bool can_upgrade = combat_raw > 0 || ( combat_upgrades / BUILDING_MAX_UPGRADES ) != combat_buildings;
            if (can_build && can_upgrade) {
                if (spread[BUILDING_EMPTY] + combat_upgrades > combat_buildings) {
                    goto decide_what_to_buy;
                }
                else {
                    goto decide_what_to_upgrade;
                }
            }
            else if (can_upgrade) {
                goto decide_what_to_upgrade;
            }
            else if (can_build) {
                goto decide_what_to_buy;
            }
            else {
                // can't do anything
                return;
            }

            decide_what_to_upgrade:
            {
                want_to_build = false;

                if (( buildings_first_level[BUILDING_SPECIAL] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_SPECIAL, 1) ) ||
                    ( buildings_unupgraded[BUILDING_SPECIAL] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_SPECIAL, 0) )
                ) {
                    wanted_building = BUILDING_SPECIAL;
                    TraceLog(LOG_INFO, "AI decided to upgrade special units");
                }
                else if (( buildings_first_level[BUILDING_SUPPORT] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_SUPPORT, 1) ) ||
                            ( buildings_unupgraded[BUILDING_SUPPORT] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_SUPPORT, 0) )
                ) {
                    wanted_building = BUILDING_SUPPORT;
                    TraceLog(LOG_INFO, "AI decided to upgrade support units");
                }
                else if (( buildings_first_level[BUILDING_ARCHER] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_ARCHER, 1) ) ||
                            ( buildings_unupgraded[BUILDING_ARCHER] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_ARCHER, 0) )
                ) {
                    wanted_building = BUILDING_ARCHER;
                    TraceLog(LOG_INFO, "AI decided to upgrade archer units");
                }
                else if (( buildings_first_level[BUILDING_FIGHTER] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_FIGHTER, 1) ) ||
                            ( buildings_unupgraded[BUILDING_FIGHTER] && ai->resource_gold >= building_upgrade_cost_raw(BUILDING_FIGHTER, 0) )
                ) {
                    wanted_building = BUILDING_FIGHTER;
                    TraceLog(LOG_INFO, "AI decided to upgrade fighter units");
                }
            }

            goto wanted_building_chosen;

            decide_what_to_buy:
            {
                want_to_build = true;

                if (spread[BUILDING_FIGHTER] <= combat_buildings / 2) {
                    wanted_building = BUILDING_FIGHTER;
                    TraceLog(LOG_INFO, "AI decided to buy fighter building");
                }
                else if (spread[BUILDING_ARCHER] <= combat_buildings / 4) {
                    wanted_building = BUILDING_ARCHER;
                    TraceLog(LOG_INFO, "AI decided to buy archer building");
                }
                else if (spread[BUILDING_SUPPORT] <= combat_buildings / 4) {
                    wanted_building = BUILDING_SUPPORT;
                    TraceLog(LOG_INFO, "AI decided to buy support building");
                }
                else {
                    wanted_building = BUILDING_SPECIAL;
                    TraceLog(LOG_INFO, "AI decided to buy special building");
                }

                if (ai->resource_gold < building_buy_cost(wanted_building)) {
                    TraceLog(LOG_INFO, "  but can't afford it...");
                    return;
                }
            }

        }
        wanted_building_chosen: {}


        Building * chosen_building = NULL;

        {
            for (usize r = 0; r < ai_regions->len; r++) {
                Region * region = ai_regions->items[r];
                bool is_safe = true;
                for (usize p = 0; p < region->paths.len; p++) {
                    PathEntry * entry = &region->paths.items[p];
                    if (entry->path->region_a->player_id != entry->path->region_b->player_id) {
                        is_safe = false;
                        break;
                    }
                }
                if (is_safe) {
                    for (usize b = 0; b < region->buildings.len; b++) {
                        Building * building = &region->buildings.items[b];
                        if (want_to_build) {
                            if (building->type == BUILDING_EMPTY) {
                                chosen_building = building;
                                goto building_chosen;
                            }
                        }
                        else {
                            if (building->type == wanted_building && building->upgrades != BUILDING_MAX_UPGRADES && ai->resource_gold >= building_upgrade_cost(building)) {
                                chosen_building = building;
                                goto building_chosen;
                            }
                        }
                    }
                }
            }

            for (usize r = 0; r < ai_regions->len; r++) {
                Region * region = ai_regions->items[r];

                for (usize b = 0; b < region->buildings.len; b++) {
                    Building * building = &region->buildings.items[b];
                    if (want_to_build) {
                        if (building->type == BUILDING_EMPTY) {
                            chosen_building = building;
                            goto building_chosen;
                        }
                    }
                    else {
                        if (building->type == wanted_building && building->upgrades != BUILDING_MAX_UPGRADES && ai->resource_gold >= building_upgrade_cost(building)) {
                            chosen_building = building;
                            goto building_chosen;
                        }
                    }
                }
            }
        }

        building_chosen:

        if (want_to_build) {
            if (chosen_building == NULL) {
                TraceLog(LOG_ERROR, "Wanted to buy a building but couldn't find empty spot for AI nr = %d", player_index);
                return;
            }
            ai->resource_gold -= building_buy_cost(wanted_building);
            place_building(chosen_building, wanted_building);
            TraceLog(LOG_INFO, "AI purchased a building");
        }
        else {
            if (chosen_building == NULL) {
                TraceLog(LOG_ERROR, "Wanted to upgrade a building but couldn't find empty spot for AI nr = %d", player_index);
                return;
            }
            ai->resource_gold -= building_upgrade_cost(chosen_building);
            upgrade_building(chosen_building);
            TraceLog(LOG_INFO, "AI purchased an upgrade");
        }
    }
}

void simulate_ai (GameState * state) {
    if (state->turn % FPS != 0) {
        return;
    }
    TraceLog(LOG_INFO, "AI tick");

    ListRegionP ai_regions = listRegionPInit(state->current_map->regions.len, &temp_alloc, NULL);

    for (usize i = 1; i < state->players.len; i++) {
        if (state->players.items[i].type != PLAYER_AI)
            continue;

        ai_regions.len = 0;
        for (usize r = 0; r < state->current_map->regions.len; r++) {
            Region * region = &state->current_map->regions.items[r];
            if (region->player_id == i) {
                listRegionPAppend(&ai_regions, region);
            }
        }
        if (ai_regions.len == 0)
            continue;

        TraceLog(LOG_INFO, "Processing AI player #%zu", i);

        make_purchasing_decision(state, i, &ai_regions);
        redirect_region_paths(state, i, &ai_regions);
        redirect_building_paths(state, i, &ai_regions);
    }
}
