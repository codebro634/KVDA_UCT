#include "../../../include/Games/Wrapper/FiniteHorizon.h"

#include <cassert>
#include <fstream>

using namespace FINITEH;

[[nodiscard]] std::string Gamestate::toString() const {
    return std::to_string('(') + std::to_string(remaining_steps) + "," + ground_state->toString() + std::to_string(')');
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return *other_state->ground_state == *ground_state && remaining_steps == other_state->remaining_steps;
}

size_t Gamestate::hash() const{
    return ground_state->hash();
}

Model::~Model(){
    if (free_ground_model)
        delete original_model;
}

Model::Model(ABS::Model* original_model, size_t horizon_length, bool free_ground_model) {
    this->original_model = original_model;
    this->horizon_length = horizon_length;
    this->free_ground_model = free_ground_model;
}

void Model::printState(ABS::Gamestate* state) {
    std::cout << "Remaining steps: " << dynamic_cast<Gamestate*>(state)->remaining_steps  << std::endl;
    original_model->printState(dynamic_cast<Gamestate*>(state)->ground_state);
}

ABS::Gamestate* Model::wrapState(ABS::Gamestate* state, size_t remaining_steps) {
    auto new_state = new Gamestate();
    new_state->ground_state = state;
    new_state->remaining_steps = remaining_steps;
    new_state->turn = state->turn;
    new_state->terminal = state->terminal || remaining_steps <= 0;
    new_state->free_ground_state = false;
    return new_state;
}

ABS::Gamestate* Model::unwrapState(ABS::Gamestate* state) {
    return dynamic_cast<Gamestate*>(state)->ground_state;
}

ABS::Gamestate* Model::getInitialState(int num) {
    ABS::Gamestate* ostate = original_model->getInitialState(num);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->turn = ostate->turn;
    state->terminal = ostate->terminal; //should be false
    state->free_ground_state = true;
    state->remaining_steps = horizon_length;
    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    ABS::Gamestate* ostate = original_model->getInitialState(rng);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->turn = ostate->turn;
    state->terminal = ostate->terminal; //should be false
    state->free_ground_state = true;
    state->remaining_steps = horizon_length;
    return state;
}

int Model::getNumPlayers() {
    return original_model->getNumPlayers();
}

bool Model::hasTransitionProbs(){
    return original_model->hasTransitionProbs();
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto copy = new Gamestate();
    copy->ground_state = original_model->copyState(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
    copy->turn = dynamic_cast<Gamestate*>(uncasted_state)->turn;
    copy->terminal = dynamic_cast<Gamestate*>(uncasted_state)->terminal;
    copy->remaining_steps = dynamic_cast<Gamestate*>(uncasted_state)->remaining_steps;
    copy->free_ground_state = dynamic_cast<Gamestate*>(uncasted_state)->free_ground_state;
    return copy;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return original_model->getActions(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    assert (dynamic_cast<Gamestate*>(uncasted_state)->remaining_steps > 0);
    auto casted_state = dynamic_cast<Gamestate*>(uncasted_state);
    casted_state->remaining_steps--;
    auto [rewards, prob] = original_model->applyAction(casted_state->ground_state, action, rng, decision_outcomes);
    casted_state->terminal |= casted_state->ground_state->terminal || casted_state->remaining_steps <= 0;
    casted_state->turn = casted_state->ground_state->turn;
    return {rewards, prob};
}