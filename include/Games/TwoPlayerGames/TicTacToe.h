#pragma once

#ifndef TTT_H
#define TTT_H
#include <vector>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

namespace TTT
{
    const static int EMPTY = -1;

    struct Gamestate: public ABS::Gamestate{
        std::vector<std::vector<int>> board = std::vector<std::vector<int>>(3,std::vector<int>(3,EMPTY));

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;

        [[nodiscard]] std::string toString() const override {
            std::string str = "([";
            for (auto & i : board) {
                str += "[";
                for (int j : i) {
                    str += std::to_string(j) + ", ";
                }
                str += "None], ";
            }
            str += "None], ";
            return str + ABS::Gamestate::toString() + ")";
        }
    };



    class Model: public ABS::Model, public WLG::Model
    {
        public:
            explicit Model(bool zero_sum);
            ~Model() override = default;
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
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

