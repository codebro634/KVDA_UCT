
#ifndef SKILLSTEACHING_H
#define SKILLSTEACHING_H

#pragma once

#include "../Gamestate.h"
#include <string>
#include <vector>
#include <random>

namespace ST {

    struct SkillsTeachingState : public ABS::Gamestate {
        std::vector<bool> proficiencyMed;
        std::vector<bool> proficiencyHigh;
        std::vector<bool> answeredRight;
        std::vector<bool> hintedRight;
        std::vector<bool> hintDelayVar;
        std::vector<bool> updateTurn;
        ~SkillsTeachingState() override = default;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class SkillsTeachingModel : public ABS::Model {

    private:
        std::vector<std::vector<int>> prerequisites;
        std::vector<double> skillWeights;
        std::vector<double> highProb;
        std::vector<double> prob_all_pre;
        std::vector<double> prob_all_pre_med;
        std::vector<double> prob_per_pre;
        std::vector<double> prob_per_pre_med;
        std::vector<double> lose_prob;

        std::vector<int> actions;
        bool reduced_action_space;
        bool idling_allowed;

    protected:

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    public:
        explicit SkillsTeachingModel(const std::string& filePath, bool idle_action, bool reduced_action_space);
        ~SkillsTeachingModel() override = default;
        void printState(ABS::Gamestate* uncasted_state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        int getNumPlayers() override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;
    };

} // namespace ABS



#endif //SKILLSTEACHING_H
