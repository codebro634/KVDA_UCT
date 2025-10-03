
#ifndef TRIANGLETIREWORLD_H
#define TRIANGLETIREWORLD_H

#pragma once

#include "../Gamestate.h"
#include <string>
#include <vector>
#include <random>

namespace TRT {

    struct Gamestate : public ABS::Gamestate {
        int vehicle_pos;
        std::vector<bool> spare_tires;
        bool notflattire;
        bool hasspare;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model : public ABS::Model {

    private:
        std::vector<std::vector<int>> connections;
        double flat_prob;
        int goal_loc;
        bool reduced_action_space;
        bool idle_action;

        int init_pos;
        std::vector<int> init_spare;

        bool big_payoff;

    protected:

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    public:
        explicit Model(const std::string& filePath, bool idle_action, bool reduced_action_space, bool big_payoff);
        ~Model() override = default;
        void printState(ABS::Gamestate* uncasted_state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        int getNumPlayers() override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;
    };

} // namespace ABS


#endif //TRIANGLETIREWORLD_H
