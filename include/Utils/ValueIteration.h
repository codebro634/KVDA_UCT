#pragma once

#ifndef VALUEITERATION_H
#define VALUEITERATION_H

#include <chrono>
#include <unordered_map>
#include "../Games/Gamestate.h"

#include "../Games/Wrapper/FiniteHorizon.h"

#endif

namespace VALUE_IT
{

    struct QMapHash {
        size_t operator()(const std::pair<const FINITEH::Gamestate*,int>& a) const {
            return a.first->hash() + (std::hash<int>{}(a.first->remaining_steps) << 24) + (std::hash<int>{}(a.second) << 16);
        }
    };

    struct QMapCompare {
        bool operator()(const std::pair<const FINITEH::Gamestate*,int>& a, const std::pair<const FINITEH::Gamestate*,int>& b ) const {
            return a.first->operator==(*b.first) && a.second == b.second;
        }
    };

    bool outOfTime(std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, std::chrono::time_point<std::chrono::system_clock> start, int time_limit);
    bool runValueIteration(FINITEH::Model* model, int num_init_samples, double discount, std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, std::mt19937& rng, bool verbose, int time_limit = std::numeric_limits<int>::max());
    bool runValueIteration(FINITEH::Model* model, FINITEH::Gamestate* branch_state, int branch_action, double discount, std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, std::mt19937& rng, bool verbose, std::chrono::time_point<std::chrono::system_clock> start_time, int time_limit = std::numeric_limits<int>::max());

    void saveQTable(const std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, const std::string& filename, bool free_memory = false);
    void loadQTable(ABS::Model* model, std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>* Q_map, const std::string& filename);

}


