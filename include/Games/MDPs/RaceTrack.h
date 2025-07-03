#pragma once

#ifndef RT_H
#define RTH
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "../Gamestate.h"
#endif

namespace RT
{

    struct Gamestate: public ABS::Gamestate{
        int x, y,  dx, dy;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName, double fail_prob, bool reset_at_crash, bool simplified_observation_space);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        std::vector<double> heuristicsValue(ABS::Gamestate* state) override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] double getMinV(int steps) const override {return -steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return -1;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    private:

        double fail_prob;
        bool reset_at_crash;

        std::vector<std::vector<bool>> obstacle_map;
        std::vector<std::vector<bool>> goal_map;
        std::vector<std::pair<int,int>> start_positions;
        std::set<std::pair<int,int>> start_set;

        //for heuristic
        std::map<std::pair<int,int>,int> distances_to_goal;
        void calculate_goal_distances();

        //for ML
        bool simplified_observation_space;

        void resetToStart(Gamestate* state, std::mt19937& rng) const;
        [[nodiscard]] bool valid_pos(int x, int y) const;
        std::pair<int,int> path_interrupt_pos(int x1, int y1, int x2, int y2);
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

