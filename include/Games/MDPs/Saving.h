#pragma once

#ifndef SAVING_H
#define SAVING_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace SAVING
{

    constexpr int BORROW_REPAY_COST = 3;
    constexpr int SAVE_REWARD = 1;
    constexpr int BORROW_INSTANT_REWARD = 2;


    struct Gamestate: public ABS::Gamestate{
        int steps_since_borrow, steps_since_invest, price;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model
    {
    public:
        explicit Model(int pmin, int pmax, int tinvest, int tborrow);
        explicit Model();
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] double getMinV(int steps) const override {return (-BORROW_REPAY_COST + std::min(std::min(std::min(pmin,-BORROW_INSTANT_REWARD),-SAVE_REWARD),0))*steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return std::max(pmax,std::max(BORROW_INSTANT_REWARD,SAVE_REWARD))* steps;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    private:
        int pmin, pmax, tinvest,tborrow;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

