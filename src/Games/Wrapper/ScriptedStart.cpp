#include "../../../include/Games/Wrapper/ScriptedStart.h"

#include <cassert>
#include <fstream>

using namespace SCRIPTEDSTART;

[[nodiscard]] std::string Gamestate::toString() const {
    return ground_state->toString();
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return *other_state->ground_state == *ground_state;
}

size_t Gamestate::hash() const{
    return ground_state->hash();
}

Model::~Model(){
    if (free_ground_model)
        delete original_model;
}

Model::Model(ABS::Model* original_model,std::vector<int> start_steps, bool free_ground_model) {
    this->original_model = original_model;
    this->startScript = start_steps;
    this->free_ground_model = free_ground_model;
}

void Model::printState(ABS::Gamestate* state) {
    original_model->printState(dynamic_cast<Gamestate*>(state)->ground_state);
}

ABS::Gamestate* Model::getInitialState(int num) {
    ABS::Gamestate* ostate = original_model->getInitialState(num);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->free_ground_state = true;

    auto rng = std::mt19937(num);
    for (int a : startScript) {
        [[maybe_unused]] auto legal_actions = original_model->getActions(ostate);
        assert(std::find(legal_actions.begin(), legal_actions.end(), a) != legal_actions.end());
        original_model->applyAction(ostate, a, rng, nullptr);
        if (ostate->terminal)
            break;
    }

    state->turn = ostate->turn;
    state->terminal = ostate->terminal; //should be false

    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    ABS::Gamestate* ostate = original_model->getInitialState(rng);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->free_ground_state = true;

    for (int a : startScript) {
        [[maybe_unused]] auto legal_actions = original_model->getActions(ostate);
        assert(std::find(legal_actions.begin(), legal_actions.end(), a) != legal_actions.end());
        original_model->applyAction(ostate, a, rng, nullptr);
        if (ostate->terminal)
            break;
    }

    state->turn = ostate->turn;
    state->terminal = ostate->terminal; //should be false

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
    copy->free_ground_state = dynamic_cast<Gamestate*>(uncasted_state)->free_ground_state;
    return copy;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return original_model->getActions(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto casted_state = dynamic_cast<Gamestate*>(uncasted_state);
    auto [rewards, prob] = original_model->applyAction(casted_state->ground_state, action, rng, decision_outcomes);
    casted_state->terminal = casted_state->ground_state->terminal;
    casted_state->turn = casted_state->ground_state->turn;
    return {rewards, prob};
}