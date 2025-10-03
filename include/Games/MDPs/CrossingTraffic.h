#pragma once

#ifndef CT_H
#define CT_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace CT
{

    struct Gamestate: public ABS::Gamestate{
        std::vector<int> obstacles;
        int x,y;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model
    {
    public:
        explicit Model(int width, int height, float spawn_rate,bool idle_action = true);
        explicit Model();
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] double getMinV(int steps) const override {return  -steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return -1;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;


    private:
        int width;
        int height;
        float spawn_rate;
        bool idle_action;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

