#include "../../../include/Games/MDPs/FixedActionSpaceEnv.h"

using namespace FASP;

bool Gamestate::operator==(const ABS::Gamestate& other) const {
    auto other_ = dynamic_cast<const Gamestate*>(&other);
    return state_num == other_->state_num;
}

size_t Gamestate::hash() const {
    return std::hash<int>{}(state_num);
}

Model::Model() {
    action_space_size = 3; //default
}

void Model::printState(ABS::Gamestate* state) {
    auto other_ = dynamic_cast<Gamestate*>(state);
    std::cout << "FASP State: " << other_->state_num << std::endl;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    std::vector<int> actions;
    for(int i = 0; i < action_space_size; i++) {
        actions.push_back(i);
    }
    return actions;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    return new Gamestate();
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    return new Gamestate();
}

int Model::getNumPlayers() {
    return 1;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    size_t decision_point = 0;

    const int num_outcomes = 100;
    if (decision_outcomes == nullptr)
        state->state_num = std::uniform_int_distribution<int>(1,num_outcomes)(rng);
    else
        state->state_num = getDecisionPoint(decision_point, 1, num_outcomes, decision_outcomes);
    return {{1.0},1.0 / (double) num_outcomes};
}