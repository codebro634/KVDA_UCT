#include "../../../include/Games/Wrapper/Determinization.h"

#include <cassert>
#include <fstream>

using namespace DETERMINIZATION;

Model::~Model(){
    if (free_ground_model)
        delete original_model;
}

Model::Model(ABS::Model* original_model,  bool free_ground_model) {
    this->original_model = original_model;
    this->free_ground_model = free_ground_model;
}

[[nodiscard]] std::string Gamestate::toString() const {
    return ground_state->toString();
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return *other_state->ground_state == *ground_state && other_state->seed_offset == seed_offset;
}

size_t Gamestate::hash() const{
    return ground_state->hash();
}

void Model::printState(ABS::Gamestate* state) {
    original_model->printState(dynamic_cast<Gamestate*>(state)->ground_state);
}

ABS::Gamestate* Model::getInitialState(int num) {
    ABS::Gamestate* ostate = original_model->getInitialState(num);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->free_ground_state = true;
    state->seed_offset = num;
    state->turn = ostate->turn;
    state->terminal = ostate->terminal;

    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    ABS::Gamestate* ostate = original_model->getInitialState(rng);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->free_ground_state = true;
    state->turn = ostate->turn;
    state->terminal = ostate->terminal;

    std::uniform_int_distribution<int> dist(1, 10000000);
    state->seed_offset = dist(rng);

    return state;
}

int Model::getNumPlayers() {
    return original_model->getNumPlayers();
}

bool Model::hasTransitionProbs(){
    return true;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto copy = new Gamestate();
    copy->ground_state = original_model->copyState(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
    copy->turn = dynamic_cast<Gamestate*>(uncasted_state)->turn;
    copy->terminal = dynamic_cast<Gamestate*>(uncasted_state)->terminal;
    copy->free_ground_state = dynamic_cast<Gamestate*>(uncasted_state)->free_ground_state;
    copy->seed_offset = dynamic_cast<Gamestate*>(uncasted_state)->seed_offset;
    return copy;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return original_model->getActions(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto casted_state = dynamic_cast<Gamestate*>(uncasted_state);
    auto fixed_rng = std::mt19937(casted_state->seed_offset + casted_state->hash());
    auto [rewards, prob] = original_model->applyAction(casted_state->ground_state, action, fixed_rng, decision_outcomes);
    casted_state->terminal = casted_state->ground_state->terminal;
    casted_state->turn = casted_state->ground_state->turn;
    return {rewards, 1.0};
}