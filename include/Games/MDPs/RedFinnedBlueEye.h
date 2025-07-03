#ifndef REDFINNEDBLUEEYE_H
#define REDFINNEDBLUEEYE_H

#pragma once

#include "../Gamestate.h"
#include <string>
#include <vector>
#include <random>
#include <set>

namespace RFBE {

    struct Gamestate : public ABS::Gamestate {
        std::vector<int> spring_populations;
        int transloc_cooldown;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model : public ABS::Model {

    private:
        const static int WATER_LEVELS = 7;
        double water_level_probs[WATER_LEVELS];
        std::vector<double> natural_mortality_prob;
        double poison_success_prob, manual_succ_prob, tranloc_succ_prob;
        std::vector<std::vector<std::vector<std::pair<int,double>>>> connected_springs_by_water_level_spread_prob;

        int action_points;

        std::vector<int> initial_spring_populations;

        bool deterministic_gambusia_spread;
        const int RED_FINNED_BLUE_EYE_REWARD = 50;
        const int TRANSLOC_CD = 8;
        const int POISON_PENALTY = 30;
        const int EXTINCT_PENALTY = 200;

    protected:

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    public:
        explicit Model(const std::string& filePath, bool deterministic_spread);
        ~Model() override = default;
        void printState(ABS::Gamestate* uncasted_state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        int getNumPlayers() override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        bool hasTransitionProbs() override {return true;}
    };

} // namespace ABS

#endif //REDFINNEDBLUEEYE_H
