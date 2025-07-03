#pragma once

#ifndef MISCANALYSIS_H
#define MISCANALYSIS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ValueIteration.h"
#include "../../include/Agents/Agent.h"

namespace MISC
{

    void createQTable(ABS::Model* ground_model,int horizon = 50, std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map_ptr = nullptr, std::string save_path = "", bool verbose =true, int time_limit = std::numeric_limits<int>::max());

    //To find critical states
    std::vector<ABS::Gamestate*> gatherSmallQDiffStates(ABS::Model& model, unsigned int num_states,std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map,  Agent* agent, std::mt19937& rng);

    //For benchmark games paper to collect statistics about every environment
    bool randomAnalysis(size_t idx, int q_time_limit);
    std::string randomAnalysis(ABS::Model* model, const std::string& instance_name, int horizon, bool verbose, int time_limit_qtable, bool outcome_sampling);

    //For measuring the number of times a tree policy requires an intra-abstraction policy
    void measureIntraAbsRate();

    //For oga-cad paper to measure the drop rates
    void estimateAbsDropNumbers();

    //analysis of oga state abstractions
    void estimateStateAbstractions();

    //analysis for smart reward handling
    void estimateQAbstractionsKVDA();

    //analysis for oga-eps comparison to pruned oga and random oga
    void estimateQAbstractionsOgaEps();

    //analysis for aupo on sysadmin
    void estimateConfIntervalsAupo();

    template <class T>
struct PointedHash
    {
        size_t operator()(const T* p) const
        {
            return p->hash();
        }
    };

    template <class T>
    struct PointedCompare
    {
        bool operator()(const T* lhs, const T* rhs) const
        {
            return lhs == rhs || *lhs == *rhs;
        }
    };

    template <class T>
    using Set = std::unordered_set<T*, PointedHash<T>, PointedCompare<T>>;

}


#endif //MISCANALYSIS_H
