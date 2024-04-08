#include "ai.h"
#include "alloc.h"
#include "std.h"
#include "constants.h"
#include "level.h"

/* Initialization ************************************************************/
void ai_init (usize player_id, GameState * state) {
    PlayerData * player = &state->players.items[player_id];
    player->ai = MemAlloc(sizeof(AIData));
    player->ai->regions = listAIRegionScoreInit(9, perm_allocator());

    for (usize r = 0; r < state->map.regions.len; r++) {
        Region * region = &state->map.regions.items[r];
        if (region->player_id == player_id) {
            AIRegionScore score = {
                .region = region,
                .frontline = true,
                .frontline_distance = 0,
                .enemies_present = false,
                .random_focus_bonus = 0,
            };
            listAIRegionScoreAppend(&player->ai->regions, score);
        }
    }

    unsigned int max = -1;
    max /= 2;
    max -= 1;
    player->ai->seed = GetRandomValue(0, (int)max);

    player->ai->aggressive = GetRandomValue(0, 1);

    player->ai->buildings.resources = GetRandomValue(1, 5) * 0.1f;

    player->ai->buildings.fighter = GetRandomValue(30, 70);
    player->ai->buildings.archer  = GetRandomValue(30, 70);
    player->ai->buildings.support = GetRandomValue(10, 40);
    player->ai->buildings.special = GetRandomValue(20, 50);
    float total = player->ai->buildings.fighter +
                  player->ai->buildings.archer +
                  player->ai->buildings.support +
                  player->ai->buildings.special;

    player->ai->buildings.fighter /= total;
    player->ai->buildings.archer  /= total;
    player->ai->buildings.support /= total;
    player->ai->buildings.special /= total;
}
void ai_deinit (PlayerData * player) {
    listAIRegionScoreDeinit(&player->ai->regions);
    MemFree(player->ai);
}

/* Lookup ********************************************************************/
AIRegionScore * get_score (ListAIRegionScore scores, Region * region) {
    for (usize i = 0; i < scores.len; i++) {
        if (scores.items[i].region == region) {
            return &scores.items[i];
        }
    }
    return NULL;
}
int sort_scores_by_frontline_distance (AIRegionScore * a, AIRegionScore * b) {
    usize av = a->frontline_distance + a->random_focus_bonus;
    usize bv = b->frontline_distance + b->random_focus_bonus;
    if (av > bv) return 1;
    else if(av < bv) return -1;
    return 0;
}

/* AI State Update ***********************************************************/
void update_frontline_status (usize player_id, GameState * state) {
    AIData * ai = state->players.items[player_id].ai;
    ListAIRegionScoreP unprocessed = listAIRegionScorePInit(ai->regions.len, temp_allocator());
    for (usize i = 0; i < ai->regions.len; i++) {
        if (ai->regions.items[i].frontline) {
            ai->regions.items[i].frontline_distance = 0;
        }
        else {
            listAIRegionScorePAppend(&unprocessed, &ai->regions.items[i]);
            ai->regions.items[i].frontline_distance = 1;
        }
    }
    if (state->map.regions.len == ai->regions.len) {
        return;
    }

    while (unprocessed.len > 0) {
        usize len = unprocessed.len;
        while (len --> 0) {
            AIRegionScore * score = unprocessed.items[len];
            bool border = false;
            for (usize i = 0; i < score->region->paths.len; i++) {
                Path * path = score->region->paths.items[i];
                Region * other = path->region_a == score->region ? path->region_b : path->region_a;
                AIRegionScore * other_score = get_score(ai->regions, other);
                if (NULL == other_score || other_score->frontline_distance < score->frontline_distance) {
                    border = true;
                    break;
                }
            }
            if (border) {
                listAIRegionScorePRemove(&unprocessed, len);
            }
            else {
                score->frontline_distance ++;
            }
        }
    }
    listAIRegionScorePDeinit(&unprocessed);
}
void update_random_bias (usize player_id, GameState * state) {
    AIData * ai = state->players.items[player_id].ai;
    for (usize i = 0; i < ai->regions.len; i++) {
        ai->regions.items[i].random_focus_bonus = GetRandomValue(0, 1);
        ai->regions.items[i].gather_troops = GetRandomValue(0, 9) == 0;
    }
}
Test update_enemy_present (usize player_id, GameState * state) {
    AIData * ai = state->players.items[player_id].ai;

    Test changed = NO;
    for (usize i = 0; i < ai->regions.len; i++) {
        Region * region = ai->regions.items[i].region;
        for (usize n = 0; n < region->nav_graph.waypoints.len; n++) {
            WayPoint * point = region->nav_graph.waypoints.items[n];
            if (point && point->unit && point->unit->player_owned != player_id) {
                if (ai->regions.items[i].enemies_present == false) {
                    changed = YES;
                    ai->regions.items[i].enemies_present = true;
                }
                goto next;
            }
        }
        if (ai->regions.items[i].enemies_present) {
            changed = YES;
            ai->regions.items[i].enemies_present = false;
        }
        next:{}
    }
    return changed;
}

/* AI Events *****************************************************************/
void ai_region_lost (usize player_id, GameState * state, Region * region) {
    AIData * ai = state->players.items[player_id].ai;

    for (usize s = 0; s < ai->regions.len; s++) {
        if (ai->regions.items[s].region == region) {
            listAIRegionScoreRemove(&ai->regions, s);
            break;
        }
    }

    for (usize i = 0; i < region->paths.len; i++) {
        Path * path = region->paths.items[i];
        Region * other = path->region_a == region ? path->region_b : path->region_a;
        if (other->player_id == player_id) {
            AIRegionScore * score = get_score(ai->regions, other);
            if (score) {
                score->frontline = true;
            }
            else {
                TraceLog(LOG_ERROR, "AI couldn't find its own region in its score list");
            }
        }
    }

    ai->new_conquest = true;
    update_frontline_status(player_id, state);
}
void ai_region_gain (usize player_id, GameState * state, Region * region) {
    AIData * ai = state->players.items[player_id].ai;

    bool front = false;
    for (usize i = 0; i < region->paths.len; i++) {
        Path * path = region->paths.items[i];
        Region * other = path->region_a == region ? path->region_b : path->region_a;
        if (other->player_id != player_id) {
            front = true;
        }
        else {
            bool other_front = false;
            for (usize p = 0; p < other->paths.len; p++) {
                Path * other_path = other->paths.items[p];
                Region * other_neighbor = other_path->region_a == other ? other_path->region_b : other_path->region_a;
                if (other_neighbor->player_id != player_id) {
                    other_front = true;
                    break;
                }
            }
            AIRegionScore * score = get_score(ai->regions, other);
            if (score) {
                score->frontline = other_front;
            }
            else {
                TraceLog(LOG_ERROR, "Failed to look up neighbor score for gained region neighbor");
            }
        }
    }
    AIRegionScore score = {0};
    score.frontline = front;
    score.region = region;

    listAIRegionScoreAppend(&ai->regions, score);
    listAIRegionScoreBubblesort(&ai->regions, sort_scores_by_frontline_distance);

    ai->new_conquest = true;
    update_frontline_status(player_id, state);
}

/* AI Logic ******************************************************************/
void redirect_region_paths (GameState * state, usize player_index) {
    AIData * ai = state->players.items[player_index].ai;
    if (ai->regions.len == 0) {
        return;
    }

    usize player_seed = ai->seed;
    usize player_turn = player_seed + state->turn;
    usize secs_bias = FPS * 60;
    usize secs_2 = FPS * 2;
    Test repath = ai->new_conquest;
    if ((player_turn % secs_bias) == 0) {
        update_random_bias(player_index, state);
        repath = YES;
    }
    if ((player_turn % secs_2) == 0) {
        if (update_enemy_present(player_index, state)) {
            repath = YES;
        }
    }
    if (repath == NO) {
        return;
    }
    ai->new_conquest = false;

    ListAIRegionScore regions = ai->regions;
    bool aggressive = ai->aggressive;
    ListAIRegionScore collect = listAIRegionScoreInit(regions.len, temp_allocator());

    for (usize r = 0; r < regions.len; r++) {
        collect.len = 0;
        AIRegionScore score = regions.items[r];
        if (score.enemies_present) {
            score.region->active_path = score.region->paths.len;
            region_reset_unit_pathfinding(score.region);
            continue;
        }
        if (score.frontline && ! aggressive) {
            for (usize n = 0; n < score.region->paths.len; n++) {
                Path * path = score.region->paths.items[n];
                Region * other = path->region_a == score.region ? path->region_b : path->region_a;
                AIRegionScore * other_score = get_score(regions, other);
                if (other_score) {
                    if (other_score->enemies_present) {
                        score.region->active_path = n;
                        region_reset_unit_pathfinding(score.region);
                        goto next_region;
                    }
                }
                else if (other->player_id == player_index) {
                    TraceLog(LOG_ERROR, "Couldn't find expected region in AI region list");
                }
            }
            goto attack;
        }
        if (score.frontline && aggressive) {
        attack:{}
            usize preferred = (player_seed + score.random_focus_bonus) % score.region->paths.len;
            usize guard = ( score.region->paths.len + 1) * (preferred + 1);
            usize index = 0;
            while (guard --> 0) {
                Path * path = score.region->paths.items[index];
                Region * other = path->region_a == score.region ? path->region_b : path->region_a;
                if (other->player_id != player_index) {
                    if (preferred == 0) {
                        score.region->active_path = index;
                        region_reset_unit_pathfinding(score.region);
                        break;
                    }
                    else {
                        preferred --;
                    }
                }
                index = (index + 1) % score.region->paths.len;
            }
            if (guard == 0 && preferred != 0) {
                TraceLog(LOG_ERROR, "Triggered guard in redirect frontline paths");
            }
            continue;
        }
        Region * target = NULL;
        for (usize c = 0; c < score.region->paths.len; c++) {
            Path * path = score.region->paths.items[c];
            Region * other = path->region_a == score.region ? path->region_b : path->region_a;
            AIRegionScore * other_score = get_score(regions, other);
            if (other_score) {
                if (other_score->enemies_present) {
                    target = other_score->region;
                    goto route_to_target;
                }
                listAIRegionScoreAppend(&collect, *other_score);
            }
            else {
                TraceLog(LOG_ERROR, "Couldn't find score for neighboring region in ai path safe redirection");
            }
        }
        if (score.gather_troops) {
            score.region->active_path = score.region->paths.len;
            region_reset_unit_pathfinding(score.region);
            continue;
        }
        if (collect.len == 0) {
            TraceLog(LOG_ERROR, "Unexpected lack of neighboring regions in ai territory");
            continue;
        }
        if (collect.len == 1) {
            target = collect.items[0].region;
        }
        else {
            listAIRegionScoreBubblesort(&collect, sort_scores_by_frontline_distance);
            usize index = 0;
            while (index < (collect.len - 1)) {
                usize score_a = collect.items[index].frontline_distance + collect.items[index].random_focus_bonus;
                usize score_b = collect.items[index + 1].frontline_distance + collect.items[index + 1].random_focus_bonus;
                if (score_a == score_b) {
                    index ++;
                }
                else{
                    break;
                }
            }
            if (index == 0) {
                target = collect.items[0].region;
            }
            else {
                usize target_index = (player_seed + score.random_focus_bonus) % (index + 1);
                target = collect.items[target_index].region;
            }
        }
        route_to_target:
        for (usize s = 0; s < score.region->paths.len; s++) {
            Path * path = score.region->paths.items[s];
            if (path->region_a == target || path->region_b == target) {
                score.region->active_path = s;
                region_reset_unit_pathfinding(score.region);
                break;
            }
        }
    next_region:{}
    }

    listAIRegionScoreDeinit(&collect);
}
void make_purchasing_decision (GameState * state, usize player_index) {
    PlayerData * player = &state->players.items[player_index];
    AIData * ai = player->ai;
    usize player_turn = ai->seed + state->turn;

    if (player_turn % FPS || ai->regions.len == 0) {
        return;
    }

    #if ( BUILDING_MAX_UPGRADES != 2 )
    #error "Expected max building level to be 2 for this AI to work correctly"
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
    usize buildings_unupgraded_spread[BUILDING_TYPE_COUNT] = {0};
    usize buildings_first_level_spread[BUILDING_TYPE_COUNT] = {0};
    usize buildings_second_level_spread[BUILDING_TYPE_COUNT] = {0};


    for (usize r = 0; r < ai->regions.len; r++) {
        for (usize b = 0; b < ai->regions.items[r].region->buildings.len; b++) {
            Building * building = &ai->regions.items[r].region->buildings.items[b];

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
    bool needs_income = upkeep + ai->buildings.resources >= income;
    bool too_many_spendings = (buildings_unupgraded_spread[BUILDING_RESOURCE] +
                               buildings_first_level_spread[BUILDING_RESOURCE] * 2 +
                               buildings_second_level_spread[BUILDING_RESOURCE] * 3) < buildings_total;
    too_many_spendings = too_many_spendings && player->resource_gold < 500;
    if (needs_income || too_many_spendings) {
        wanted_building = BUILDING_RESOURCE;
        wanted_upgrade = buildings_unupgraded_spread[BUILDING_RESOURCE] || buildings_first_level_spread[BUILDING_RESOURCE];
        wanted_building_upgrade_level = buildings_unupgraded_spread[BUILDING_RESOURCE] == 0;
        if (wanted_upgrade == false && buildings_unupgraded_spread[BUILDING_RESOURCE] == 0 && buildings_empty_count == 0) {
            goto warrior_buildings;
        }
    }
    else {
        warrior_buildings:
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
            float desired_spread[BUILDING_TYPE_LAST] = {0};
            desired_spread[BUILDING_FIGHTER] = ai->buildings.fighter;
            desired_spread[BUILDING_ARCHER]  = ai->buildings.archer;
            desired_spread[BUILDING_SUPPORT] = ai->buildings.support;
            desired_spread[BUILDING_SPECIAL] = ai->buildings.special;

            BuildingType try_wanted = ai->seed % BUILDING_TYPE_LAST;
            usize counter = BUILDING_TYPE_LAST;
            while (counter > 0) {
                if (try_wanted == BUILDING_EMPTY) {
                    try_wanted += 1;
                }
                float spread = (buildings_unupgraded_spread[try_wanted] +
                                buildings_first_level_spread[try_wanted] +
                                buildings_second_level_spread[try_wanted]) /
                    (float)buildings_total;

                if (spread < desired_spread[try_wanted]) {
                    wanted_building = try_wanted;
                    goto building_chosen;
                }
                try_wanted = (try_wanted + 1) % BUILDING_TYPE_LAST;
                counter --;
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
        if (building_upgrade_cost_raw(wanted_building, 1) > player->resource_gold) {
            return;
        }

        usize len = ai->regions.len;
        while (len --> 0) { // list is expected to be sorted with safe regions at the back
            Region * region = ai->regions.items[len].region;
            for (usize b = 0; b < region->buildings.len; b++) {
                Building * building = &region->buildings.items[b];

                if (building->type != wanted_building || building->upgrades != wanted_building_upgrade_level) {
                    continue;
                }

                usize cost = building_upgrade_cost(building);
                if (player->resource_gold < cost) {
                    continue;
                }

                upgrade_building(building);
                player->resource_gold -= cost;
                return;
            }
        }
    }
    else {
        usize cost = building_buy_cost(wanted_building);
        if (cost > player->resource_gold)
            return;

        usize len = ai->regions.len;
        while (len --> 0) {
            Region * region = ai->regions.items[len].region;
            usize buildings = region->buildings.len;
            while (buildings --> 0) {
                usize index = (buildings + ai->seed + state->turn) % region->buildings.len;
                Building * building = & region->buildings.items[index];

                if (building->type != BUILDING_EMPTY)
                    continue;

                place_building(building, wanted_building);
                player->resource_gold -= cost;
                return;
            }
        }
        TraceLog(LOG_ERROR, "AI #%zu wanted to buy building %d but couldn't find empty spot for it", player_index, wanted_building);
    }

}

void simulate_ai (GameState * state) {
    for (usize i = 1; i < state->players.len; i++) {
        if (state->players.items[i].type != PLAYER_AI)
            continue;

        if (state->players.items[i].ai->regions.len == 0)
            continue;

        redirect_region_paths(state, i);
        make_purchasing_decision(state, i);
    }
}
