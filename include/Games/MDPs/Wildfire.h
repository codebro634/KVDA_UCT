#pragma once

#ifndef WF_H
#define WF_H
#include <vector>

#include "../Gamestate.h"

namespace WF
{

    constexpr int COST_CUTOUT = 5;
    constexpr int COST_PUTOUT = 10;
    constexpr int COST_TARGET_BURN = 100;
    constexpr int COST_NONTARGET_BURN = 5;

    struct Gamestate: public ABS::Gamestate{
        std::vector<std::vector<bool>> burning;
        std::vector<std::vector<bool>> out_of_fuel;
        int num_burning, num_out_of_fuel; //redundant only needed for efficient hash-value calulcation
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    private:
        int width{}, height{};
        std::vector<std::vector<bool>> cut_connections;
        std::vector<int> actions;
        std::vector<bool> is_target;
        std::vector<int> init_burns;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}


#endif

