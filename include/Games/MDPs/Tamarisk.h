#pragma once

#ifndef TAMARISK_H
#define TAMARISK_H

#include "../Gamestate.h"
#include <unordered_map>
#include <bitset>

#endif // TAMARISK_H

namespace TAM {

class Gamestate : public ABS::Gamestate {

public:
    int tamarisk_at;
    int native_at;

    std::vector<int> num_tamarisk_reach;

    //topology info to interepret tamarisk_at and native_at
    int num_slots;
    int max_slots_per_reach;
    std::vector<int> slots_at_reach;

    [[nodiscard]] std::string toString() const override;

    bool operator==(const ABS::Gamestate& other) const override;
    [[nodiscard]] size_t hash() const override;
};


    //Tamarisk model constants
static constexpr double ERADICATION_RATE =0.9;
static constexpr double RESTORATION_RATE = 0.9;
static constexpr double DOWNSTREAM_SPREAD_RATE = 0.6;
static constexpr double UPSTREAM_SPREAD_RATE = 0.15;
static constexpr double DEATH_RATE_TAMARISK = 0.05;
static constexpr double DEATH_RATE_NATIVE = 0.05;
static constexpr double EXOGENOUS_PROD_RATE_NATIVE =  0.1;
static constexpr double EXOGENOUS_PROD_RATE_TAMARISK = 0.1;
static constexpr double COMPETITION_WIN_RATE_NATIVE = 0.2;
static constexpr double COMPETITION_WIN_RATE_TAMARISK = 0.8;
static constexpr double COST_PER_INVADED_REACH = 5.0;
static constexpr double COST_PER_TREE = 0.5;
static constexpr double COST_PER_EMPTY_SLOT = 0.25;
static constexpr double ERADICATION_COST = 0.49;
static constexpr double RESTORATION_COST = 0.9;
static constexpr double RESTORATION_COST_FOR_EMPTY_SLOT = 0.4;
static constexpr double RESTORATION_COST_FOR_INVADED_SLOT = 0.8;
static constexpr bool COMBINATORIAL_ACTION_SPACE = false;

class Model : public ABS::Model {
private:

    // Topology
    int num_reaches{};
    int num_slots{};
    std::vector<int> slots_at_reach; // number of slots at reach
    std::vector<int> action_list;
    int init_tamarisk_at{};
    int init_native_at{};

protected:
    std::pair<std::vector<double>, double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
    std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

public:
    explicit Model();
    void init_model_from_file(const std::string& filePath);
    explicit Model(const std::string& fileName);
     ~Model() override = default;

    void printState(ABS::Gamestate* uncasted_state) override;
    ABS::Gamestate* getInitialState(std::mt19937& rng) override;
    int getNumPlayers() override;
    ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
    bool hasTransitionProbs() override {return true;}

    [[nodiscard]] double getMinV(int steps) const override {
        if(COMBINATORIAL_ACTION_SPACE)
            throw std::runtime_error("MinV Not implemented for Tamarisk combinatorial action space");
        else {
            double min_step = -num_reaches * (COST_PER_INVADED_REACH + std::max(RESTORATION_COST,ERADICATION_COST));
            min_step -= num_slots * std::max(COST_PER_TREE, COST_PER_EMPTY_SLOT+ RESTORATION_COST_FOR_EMPTY_SLOT);
            return min_step * steps;
        }
    }
    [[nodiscard]] double getMaxV(int steps) const override {return 0;}
    [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

    [[nodiscard]] std::vector<int> obsShape() const override;
    void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
    [[nodiscard]] std::vector<int> actionShape() const override;
    [[nodiscard]] int encodeAction(int* decoded_action) override;


};

} // namespace ABS