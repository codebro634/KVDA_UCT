
#include "../../../include/Games/MDPs/PushYourLuck.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <chrono>
#include <queue>
#include <set>

using namespace PushYL;

std::vector<int> Model::obsShape() const {
    return {static_cast<int>(die_probs[0].size())};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<PushYL::Gamestate*>(uncasted_state);
    for (size_t i = 0; i < die_probs[0].size(); i++)
        obs[i] = 0;
    for (auto val : state->last_seen_vals)
        obs[val] = 1;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    auto shape = std::vector<int>();
    for (size_t i = 0; i < die_probs.size(); i++)
        shape.push_back(2);
    return shape;
}

int Model::encodeAction(int* decoded_action) {
    int action = 0;
    for (size_t i = 0; i < die_probs.size(); i++) {
        if (decoded_action[i] == 1)
            action |= (1 << i);
    }
    return action;
}

Model::Model(const std::string& fileName) {

    std::ifstream in(fileName);
    if(!in.is_open()){
        std::cerr << "Could not open file: " << fileName << std::endl;
        exit(1);
    }
    int dices, non_zero_vals;
    in >> dices >> non_zero_vals;

    const int num_vals = 20;

    for (int i = 0; i < num_vals; i++) {
        if (i < non_zero_vals) {
            double val;
            in >> val;
            die_values.push_back(val);
        } else {
            die_values.push_back(0);
        }
    }

    die_probs = std::vector<std::vector<double>>(dices, std::vector<double>(num_vals,0.0));
    for (int i = 0; i < dices; i++) {
        for (int j = 0; j < non_zero_vals; j++) {
            in >> die_probs[i][j];
        }
    }

    actions.push_back(0); //cash out
    for (int i = 1; i < std::pow(2,dices); i++) //for each dice whether to throw or not to throw
        actions.push_back(i);
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return other_state->last_seen_vals == last_seen_vals;
}

size_t Gamestate::hash() const{
    int hash = 0;
    for (auto val : last_seen_vals)
        hash = (hash << 5) | val;
    return hash;
}

void Model::printState(ABS::Gamestate* state) {
    auto pyl_state = dynamic_cast<Gamestate*>(state);
    std::cout << "Marked values: ";
    for (auto val : pyl_state->last_seen_vals) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)  {
    auto state = new Gamestate();
    state->last_seen_vals = {};
    return state;
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return actions;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
        auto* newState = dynamic_cast<PushYL::Gamestate*>(uncasted_state);
        auto oldState = *newState;

        size_t decision_point = 0;
        double outcomeProb = 1;

        bool reset_markings = false;
        if (action != 0){
            assert (action > 0);
            std::set<int> thrown_values = {};
            for(size_t i = 0; i < die_probs.size(); i++){
                int thrown = (action >> i) & 1;
                if(thrown){
                    //throw dice i
                    std::discrete_distribution<int> dist(die_probs[i].begin(), die_probs[i].end());
                    int val;
                    if (decision_outcomes == nullptr)
                        val = dist(rng);
                    else {
                        std::vector<int> non_zero_probs;
                        for (size_t j = 0; j < die_probs[i].size(); j++) {
                            if (die_probs[i][j] > 0)
                                non_zero_probs.push_back(j);
                        }
                        val = non_zero_probs[getDecisionPoint(decision_point, 0, non_zero_probs.size()-1, decision_outcomes)];
                    }

                    outcomeProb *= die_probs[i][val];
                    if(thrown_values.contains(val) || oldState.last_seen_vals.contains(val)) //reset
                        reset_markings = true;
                    thrown_values.insert(val);
                    newState->last_seen_vals.insert(val);
                }
            }
        }

        double reward = 0;
        if(action == 0){
            reset_markings = true;
            reward = 1;
            for(auto val : newState->last_seen_vals)
                reward *= die_values[val];
        }

        if(reset_markings)
            newState->last_seen_vals.clear();

        std::vector<double> rew(1, reward);
        return std::make_pair(rew, std::nan(""));
}