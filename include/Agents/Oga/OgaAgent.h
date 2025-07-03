
#pragma once

#ifndef OGAAGENT_H
#define OGAAGENT_H
#include <map>

#include "OgaGroundNodes.h"
#include "../../Utils/ValueIteration.h"
#include "../Agent.h"
#endif

namespace OGA
{
    struct OgaBudget{
        int amount;
        std::string quantity;
    };

    struct OgaSearchStats{
        OgaBudget budget;
        int completed_iterations{};
        unsigned total_forward_calls{};
        unsigned max_depth{};

        // For node id tracking
        unsigned max_abs_state_id = 0;
        unsigned max_abs_q_id = 0;
        unsigned max_state_id = 0;
        unsigned max_q_id = 0;

        //For global std exploration factor
        double total_squared_v{};
        double total_v{};
        int global_num_vs = 0;
    };

    struct OgaArgs
    {
        OgaBudget budget;
        unsigned recency_count_limit = 3;
        double exploration_parameter = 2.0; //multiplied with the dynamic (global std) exp param
        double discount = 1.0;
        int num_rollouts = 1;
        int rollout_length = -1;
        OgaBehaviorFlags behavior_flags;

        //Only optionally needed when tracking statistics
        bool track_statistics = false;
        std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map = nullptr;
        Agent* distribution_agent = nullptr;
    };


    class OgaAgent final : public Agent
    {
    private:
        OgaStateNode* selectSuccessorState(OgaTree* tree, OgaStateNode* node, ABS::Model* model, OgaSearchStats& search_stats, std::mt19937& rng,
                                           bool* new_state);
        OgaStateNode* treePolicy(OgaTree* tree, ABS::Model* model, OgaSearchStats& search_stats, std::mt19937& rng);

        std::vector<double> rollout(const OgaStateNode* leaf, ABS::Model* model, std::mt19937& rng) const;
        std::vector<double> nnEvaluate(const OgaStateNode* leaf, ABS::Model* model) const;

        void backup(OgaTree* tree, OgaStateNode* leaf, std::vector<double> values, OgaSearchStats& search_stats) const;
        int selectAction(ABS::Model* model, OgaStateNode* node, bool greedy, OgaSearchStats& search_stats, std::mt19937& rng);

        double exploration_parameter;
        double discount;
        int num_rollouts;
        int rollout_length;
        unsigned recency_count_limit;
        OgaBudget budget;
        const OgaArgs args;

        //Statistics
        bool track_statistics;
        std::map<std::string,std::map<int,double>> layerwise_statistics; //statisticsname -> depth -> value
        std::map<std::string,double> global_statistics; //statisticsname -> value
        std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map;
        Agent* distribution_agent;

        constexpr static double TIEBREAKER_NOISE = 1e-6;

    public:
        explicit OgaAgent(const OgaArgs& args);
        ~OgaAgent() override;

        int getAction(ABS::Model* model, ABS::Gamestate* state, std::mt19937& rng) override;
        int getAction(ABS::Model* model, ABS::Gamestate* state, std::mt19937& rng, OgaTree** treePtr); // Used for testing

        double getStatistics(std::string name, int layer);
        bool containsStatistic(std::string name) { return layerwise_statistics.contains(name); }
        std::map<int,double> getLayerwiseStatistic(std::string name) { return layerwise_statistics[name]; }

        static void runTests();
    };
}
