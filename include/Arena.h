#pragma once

#ifndef ARENA_H
#define ARENA_H

#include <vector>
#include <random>
#include <set>
#include <unordered_map>

#include "Games/Gamestate.h"
#include "Agents/Agent.h"
#include "Utils/ValueIteration.h"

#endif //ARENA_H

enum OutputMode{
    MUTED,
    VERBOSE,
    CSV,
    CSV_OMIT_TIMES
};

std::vector<std::vector<double>> playGames(ABS::Model* model,
    int num_maps,
    std::vector<Agent*> agents,
    std::mt19937& rng,
    OutputMode output_mode,
    std::pair<int,int> horizons,
    bool planning_beyond_execution_horizon = false,
    bool random_init_state =true,
    double required_conf_range = std::numeric_limits<double>::max(),
    std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map = nullptr,
    const std::string& rng_save_path = "",
    int episode_num_offset = 0);

//Return is of dimension [2][num_player]. For each player, the first element is the total reward, the second element is the average decision time in milliseconds.
