#pragma once

#ifndef FASP_H
#define FASP_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace FASP
{

    struct Gamestate: public ABS::Gamestate{
        int state_num = 0;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model
    {
    public:
        explicit Model(int action_space_size) : action_space_size(action_space_size) {};
        explicit Model();
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] double getMinV(int steps) const override {return -1 * steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return steps;}

    private:
        int action_space_size;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    };

}

