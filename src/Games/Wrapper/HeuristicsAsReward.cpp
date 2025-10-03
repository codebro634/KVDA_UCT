#include "../../../include/Games/Wrapper/HeuristicsAsReward.h"
#include <cassert>

using namespace HEURISTICSASREWARD;

Model::~Model(){
    if (free_ground_model)
        delete original_model;
}

Model::Model(ABS::Model* original_model,  bool free_ground_model, double constant_reward) {
    this->original_model = original_model;
    this->free_ground_model = free_ground_model;
    this->const_reward = constant_reward;
}

void Model::printState(ABS::Gamestate* state) {
    original_model->printState(state);
}

ABS::Gamestate* Model::getInitialState(int num) {
    return original_model->getInitialState(num);
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    return original_model->getInitialState(rng);
}

int Model::getNumPlayers() {
    return original_model->getNumPlayers();
}

bool Model::hasTransitionProbs(){
    return original_model->hasTransitionProbs();
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    return original_model->copyState(uncasted_state);
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return original_model->getActions(uncasted_state);
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto old_v = original_model->heuristicsValue(uncasted_state);
    auto [rewards, prob] = original_model->applyAction(uncasted_state, action, rng, decision_outcomes);
    assert (prob == 1);
    auto new_v = original_model->heuristicsValue(uncasted_state);
    for (int i = 0; i < (int)rewards.size(); i++)
        rewards[i] = this->const_reward + new_v[i] - old_v[i];
    return {rewards, prob};
}