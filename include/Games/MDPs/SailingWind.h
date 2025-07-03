#pragma once

#ifndef SW_H
#define SW_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace SW
{

    const static int DEFAULT_ROWS = 10, DEFAULT_COLS  = 10; //5, 10 , 20 ,25 ,30 have been tested in paper

    inline std::vector<std::vector<double>>  STOCHASTIC_WIND_PROBS = {
        {0.4, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3},
        {0.4, 0.3, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0},
        { 0.0, 0.4, 0.3, 0.3, 0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.4, 0.3, 0.3, 0.0, 0.0, 0.0},
        { 0.0, 0.0, 0.0, 0.4, 0.2, 0.4, 0.0, 0.0},
        { 0.0, 0.0, 0.0, 0.0, 0.3, 0.3, 0.4, 0.0},
        { 0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.3, 0.4},
        { 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.3, 0.3}
    };

    inline std::vector<std::vector<double>>  DETERMINISTIC_WIND_PROBS = {
        {0,0,1,0,0,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,0,0,0,0,0},
        {1,0,0,0,0,0,0,0},
        {0,0,0,0,0,0,0,0}
    };

    struct Gamestate: public ABS::Gamestate{
        int x, y,  wind_dir;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override;
    };

    class Model: public ABS::Model
    {
    public:
        explicit Model(int rows, int cols, bool deterministic = false);
        explicit Model(bool deterministic = false);
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}
        std::vector<double> heuristicsValue(ABS::Gamestate* state) override;

        [[nodiscard]] double getMinV(int steps) const override {return -6*steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return -6;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

        [[nodiscard]] ABS::Gamestate* deserialize(std::string &ostring) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    private:
        int ROWS,COLS;
        std::vector<std::vector<double>> wind_probs;

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

