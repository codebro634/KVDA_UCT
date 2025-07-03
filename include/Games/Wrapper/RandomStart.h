#pragma once

#ifndef RANDOMSTART_H
#define RANDOMSTART_H
#include <map>
#include <set>
#include <vector>
#include "../../Agents/RandomAgent.h"
#include "../Gamestate.h"

namespace RANDOMSTART
{

    struct Gamestate: public ABS::Gamestate{
        ABS::Gamestate* ground_state;

        [[nodiscard]] std::string toString() const override;
        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;

        bool free_ground_state = true;
        ~Gamestate() override {
            if (free_ground_state)
                delete ground_state;
        }
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override;
        explicit Model(ABS::Model* original_model, size_t random_steps, bool free_ground_model);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* getInitialState(int num) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override;
        ABS::Model* getGroundModel() {return original_model;}

        [[nodiscard]] virtual double getMaxV(int remaining_steps) const {
            return original_model->getMaxV(remaining_steps);
        }

        [[nodiscard]] virtual double getMinV(int remaining_steps) const {
           return original_model->getMinV(remaining_steps);
        }

        [[nodiscard]] std::vector<int> obsShape() const override {
            return original_model->obsShape();
        }

        [[nodiscard]] std::vector<int> actionShape() const override {
            return original_model->actionShape();
        }

        void getObs(ABS::Gamestate* uncasted_state, int* obs) override {
            original_model->getObs(dynamic_cast<Gamestate*>(uncasted_state)->ground_state, obs);
        }

        [[nodiscard]] int encodeAction(int* decoded_action) override {
            return original_model->encodeAction(decoded_action);
        }

    private:
        ABS::Model* original_model;
        RandomAgent random_agent = RandomAgent();
        size_t random_steps;
        bool free_ground_model;

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

#endif

