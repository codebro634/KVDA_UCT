#include "../../../include/Games/Wrapper/MultiPlayerToMDP.h"

#include <cassert>
#include <fstream>
using namespace std;

using namespace MPTOMDP;


bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return *other_state->ground_state == *ground_state && opponent_seed == other_state->opponent_seed;
}

size_t Gamestate::hash() const{
    return ground_state->hash() ^ (static_cast<size_t>(opponent_seed) << 16); //hash the ground state and the opponent seed
}

Model::~Model(){
    if (free_ground_model)
        delete original_model;
    for(auto& [_,agent] : agents)
        delete agent;
}

Model::Model(ABS::Model* original_model,std::map<int,Agent*> agents, double discount, int player, bool deterministic_opponents, bool free_ground_model){
    this->original_model = original_model;
    this->agents = agents;
    this->discount = discount;
    this->player = player;
    this->deterministic_opponents = deterministic_opponents;
    this->free_ground_model = free_ground_model;

    for (int i = 0; i < original_model->getNumPlayers(); i++){
        assert (i == player || agents.contains(i));
    }
}

void Model::printState(ABS::Gamestate* state) {
    original_model->printState(dynamic_cast<Gamestate*>(state)->ground_state);
}

std::pair<double,double> Model::playTillNextTurn(Gamestate* state, std::mt19937& rng){
    double reward = 0;
    int num_plays = 0;
    double prob = 1.0;
    while (state->ground_state->turn != player && !state->terminal){
        int action = agents[state->ground_state->turn]->getAction(original_model, state->ground_state,  rng);
        auto [rewards, sample_prob] = original_model->applyAction(state->ground_state, action, rng, nullptr);
        state->terminal |= state->ground_state->terminal;
        reward += std::pow(discount,num_plays++) *  rewards[player];
        prob *= sample_prob;
    }
    return {reward,prob};
}

ABS::Gamestate* Model::getInitialState(int num) {
    ABS::Gamestate* ostate = original_model->getInitialState(num);
    auto state = new Gamestate();
    state->ground_state = ostate;
    state->opponent_seed = num;
    std::mt19937 rng(static_cast<unsigned int>(num + state->hash()));
    playTillNextTurn(state, rng);
    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    ABS::Gamestate* ostate = original_model->getInitialState(rng);
    auto state = new Gamestate();
    state->ground_state = ostate;
    std::uniform_int_distribution<int> dist(0, 10000000);
    state->opponent_seed = dist(rng);
    std::mt19937 agents_rng(static_cast<unsigned int>(state->opponent_seed + (deterministic_opponents? state->hash() : 0))); // : 0 only for computational speedup
    playTillNextTurn(state, deterministic_opponents? agents_rng : rng);
    return state;
}

int Model::getNumPlayers() {
    return 1;
}

bool Model::hasTransitionProbs(){
    return original_model->hasTransitionProbs() && deterministic_opponents;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto copy = new Gamestate();
    copy->ground_state = original_model->copyState(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
    copy->terminal = dynamic_cast<Gamestate*>(uncasted_state)->terminal;
    copy->opponent_seed = dynamic_cast<Gamestate*>(uncasted_state)->opponent_seed;
    return copy;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return original_model->getActions(dynamic_cast<Gamestate*>(uncasted_state)->ground_state);
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto casted_state = dynamic_cast<Gamestate*>(uncasted_state);
    assert (casted_state->ground_state->turn == player && decision_outcomes == nullptr); //decision outcomes not supported
    auto [rewards, prob] = original_model->applyAction(casted_state->ground_state, action, rng, nullptr);
    casted_state->terminal |= casted_state->ground_state->terminal;

    std::mt19937 agents_rng(static_cast<unsigned int>(casted_state->opponent_seed + deterministic_opponents? uncasted_state->hash() : 0)); // : 0 only for computational speedup
    auto [additional_rews, additional_prob] = playTillNextTurn(casted_state, deterministic_opponents? agents_rng : rng);

    return {{rewards[player] + discount * additional_rews}, prob * additional_prob};
}