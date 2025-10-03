#pragma once

#ifndef MAB_H
#define MAB_H
#include <vector>

#include "../Gamestate.h"


/*
 *
 * WARNING: In Multi-Armed-Bandit the reward is stochastic, hence the same (s,a,s') triplet can have different rewards if executed multiple times.
 *
 */

namespace MAB
{

    struct Gamestate: public ABS::Gamestate
    {
        [[nodiscard]] size_t hash() const override;
        bool operator==(const ABS::Gamestate& other) const override;
    };

    class Model: public ABS::Model{

        public:
            ~Model() override = default;
            explicit Model(const std::vector<std::pair<double,double>>& arm_distributions, int arm_copies);
            void printState(ABS::Gamestate* state) override;
            ABS::Gamestate* getInitialState(std::mt19937& rng) override;
            ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
            int getNumPlayers() override;
            bool hasTransitionProbs() override {return true;}

            [[nodiscard]] std::vector<int> obsShape() const override;
            void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
            [[nodiscard]] std::vector<int> actionShape() const override;
            [[nodiscard]] int encodeAction(int* decoded_action) override;

        private:
            std::vector<std::pair<double,double>> arm_distributions; //mean and std of a gaussian distribution
            std::vector<int> actions;

            std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
            std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}


#endif

