#pragma once

#ifndef PYLOS_H
#define PYLOS_H
#include <set>
#include <tuple>
#include <vector>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

/**
 * Pylos without piece recovery
 */
namespace PYL
{
    const static int STACK_LEVEL_IDX = -1;
    const static int EMPTY = -1;
    const static int NUM_LEVELS = 4; //must be even so both sum of squares is even so that both players have same amount of stones

    struct Gamestate: public ABS::Gamestate{
        std::vector<std::vector<std::vector<int>>> levels = std::vector<std::vector<std::vector<int>>>(NUM_LEVELS,std::vector<std::vector<int>>(NUM_LEVELS,std::vector<int>(NUM_LEVELS,EMPTY)));
        std::set<std::tuple<int,int,int>> top_level_stones = {};
        int stones_p0;
        int stones_p1;

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
        static int actionToHash(int from_l, int from_x, int from_y, int to_l, int to_x, int to_y);
        static std::tuple<int, int, int , int,int,int> hashToAction(int hash);
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

