#pragma once

#ifndef TRAVELLING_SALES_PERSON_H
#define TRAVELLING_SALES_PERSON_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace TRAVELLING_SALES_PERSON
{

    struct Gamestate: public ABS::Gamestate{
        int current_node;
        long long visited_nodes;

        [[nodiscard]] bool has_been_visited(const int node) const { return (visited_nodes >> node) & 1; }
        void set_visited_node(const int node) { visited_nodes |= (1 << node); }

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override;
    };


    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* getInitialState(int num) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] ABS::Gamestate* deserialize(std::string &ostring) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

        [[nodiscard]] double getMinV(int steps) const override {return steps * -*std::max_element(costs.begin(), costs.end());}
        [[nodiscard]] double getMaxV(int steps) const override {return 0;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

    private:
        int nr_of_nodes;
        int origin;
        std::vector<double> costs;

        [[nodiscard]] double get_cost(const int from, const int to) const { return costs[from * nr_of_nodes + to]; }

    protected:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    };

}