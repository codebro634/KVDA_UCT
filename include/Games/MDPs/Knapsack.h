#pragma once

#ifndef KNAPSACK_H
#define KNAPSACK_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace KNAPSACK
{

    struct Gamestate: public ABS::Gamestate{
        std::vector<int> total_knapsack_weights; // the total weight of objects for each knapsack
        std::vector<int> total_knapsack_values; // the total value of objects for each knapsack
        int current_item_weight;
        int current_item_value;

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


        [[nodiscard]] double getMinV(int steps) const override {return 0;}
        [[nodiscard]] double getMaxV(int steps) const override;
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;


    protected:

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    private:
        int nr_of_knapsacks;
        std::vector<int> knapsack_capacity;
        std::vector<int> item_weights;
        std::vector<int> item_values;
        std::vector<double> item_probabilities;
        int initial_item_weight, initial_item_value;
        int random_item(std::mt19937& rng) const;

    };

}