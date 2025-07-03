
#ifndef PUSHYOURLUCK_H
#define PUSHYOURLUCK_H

#pragma once

#include "../Gamestate.h"
#include <string>
#include <vector>
#include <random>
#include <set>

namespace PushYL {

    struct Gamestate : public ABS::Gamestate {
        std::set<int> last_seen_vals;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model : public ABS::Model {

    private:
        std::vector<double> die_values;
        std::vector<std::vector<double>> die_probs;
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

#endif //PUSHYOURLUCK_H
