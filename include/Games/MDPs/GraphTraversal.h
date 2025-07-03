#pragma once

#ifndef GTR_H
#define GTR_H
#include <set>
#include <vector>

#include "../Gamestate.h"
#endif

namespace GTR
{

    struct Gamestate: public ABS::Gamestate{
        int vertex_at;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override {
            return std::string("(") + std::to_string(vertex_at) + std::string(", ") + ABS::Gamestate::toString() + std::string(")");
        }
    };

    class Model: public ABS::Model
    {
    public:
        explicit Model(const std::string& fileName);
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}


    private:
        std::vector<std::vector<std::vector<std::pair<int,double>>>> transition_graph;  //first pair is transition vertex, second entry reward
        std::vector<std::vector<double>> transition_rewards;
        std::set<int> terms;

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

