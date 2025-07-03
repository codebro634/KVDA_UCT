#ifndef WILDLIFEPRESERVE_H
#define WILDLIFEPRESERVE_H

#pragma once

#include "../Gamestate.h"
#include <string>
#include <vector>
#include <random>
#include <set>

namespace WLP {

    struct Gamestate : public ABS::Gamestate {
        std::vector<std::vector<bool>> defenses;
        std::vector<bool> area_successful_attacks;
        std::set<int> caught_poachers;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model : public ABS::Model {

    private:
        int poacher_max_memory;
        std::vector<std::vector<std::vector<double>>> poacher_area_num_attack_weight;
        std::vector<double> area_defense_reward;
        std::vector<double> area_attack_penalty;
        int rangers;
        int num_poachers;
        std::vector<int> actions;

    protected:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    public:
        explicit Model(const std::string& filePath);
        ~Model() override = default;
        void printState(ABS::Gamestate* uncasted_state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        int getNumPlayers() override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        bool hasTransitionProbs() override {return false;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;
    };

} // namespace ABS

#endif //WILDLIFEPRESERVE_H
