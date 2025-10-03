#pragma once

#ifndef CHESS_H
#define CHESS_H
#include <set>
#include <vector>
#include <random>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

namespace CHE
{
    const static int EMPTY = -1;
    const static int PAWN = 0;
    const static int KNIGHT = 1;
    const static int BISHOP = 2;
    const static int ROOK = 3;
    const static int QUEEN = 4;
    const static int KING = 5;

    struct Gamestate: public ABS::Gamestate{
        std::vector<std::vector<std::vector<int>>> board;
        std::set<std::pair<int,int>> occupied;

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

    private:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

