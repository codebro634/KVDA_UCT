#pragma once

#ifndef TS_H
#define TS_H
#include <set>
#include <vector>

#include "../Gamestate.h"
#endif

namespace TS
{

    struct Gamestate: public ABS::Gamestate{
        short state;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;

        [[nodiscard]] std::string toString() const override {
            return "((" + std::to_string(state) + ")" + ", " + ABS::Gamestate::toString() + ")";
        }
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}


    private:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

