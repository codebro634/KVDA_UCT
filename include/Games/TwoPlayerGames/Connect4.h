#pragma once

#ifndef CONNECT4_H
#define CONNECT4_H
#include <vector>
#include <random>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

namespace C4
{
    const static int EMPTY = -1;
    const static int ROWS = 6;
    const static int COLS = 7;

    struct Gamestate: public ABS::Gamestate{
        std::vector<std::vector<int>> board = std::vector<std::vector<int>>(ROWS,std::vector<int>(COLS,EMPTY));
        int num_stones_on_board = 0; //redundant, just for efficiency
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
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
            [[nodiscard]] std::vector<double> heuristicsValue(ABS::Gamestate* uncasted_state) override;

        private:
            std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
            std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

