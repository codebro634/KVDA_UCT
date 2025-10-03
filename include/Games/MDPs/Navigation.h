//
// Created by chris on 09.10.2024.
//

#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <vector>
#include <string>
#include <random>

#include "../Gamestate.h"

#endif //NAVIGATION_H

namespace Navigation
{

    struct Gamestate: public ABS::Gamestate {
        std::pair<int, int> position;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;

        [[nodiscard]] std::string toString() const override {
            return "((" + std::to_string(position.first) + ", " + std::to_string(position.second) + ")" + ", " + ABS::Gamestate::toString() + ")";
        }
    };


    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName, bool idle_action, bool state_dependent_rewards=false);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] double getMinV(int steps) const override {return -steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return -1;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    private:
        std::pair<std::vector<double>, double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
        inline bool isAllowedAction(ABS::Gamestate* uncasted_state, int action) const;

        std::pair<int, int> spawn;
        std::pair<int, int> goal;
        std::pair<int, int> size_;

        std::vector<std::vector<double>> map;

        std::uniform_real_distribution<> dist;

        bool idle_action;
        bool state_dependent_rewards;

    };

}
