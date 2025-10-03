#include "../../../include/Games/TwoPlayerGames/NumbersRace.h"

using namespace NUM;

std::vector<int> Model::obsShape() const {
    if (one_hot_obs)
        return {goal + 1};
    else
        return {1};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    if (one_hot_obs) {
        for (int i = 0; i <= goal; i++)
            obs[i] = 0;
        if (dynamic_cast<Gamestate*>(uncasted_state)->sum <= goal)
            obs[dynamic_cast<Gamestate*>(uncasted_state)->sum] = 1;
    }
    else
        obs[0] = dynamic_cast<Gamestate*>(uncasted_state)->sum;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {(int)actions.size()};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0] + 1;
}

Model::Model(int goal, int max_action, bool zero_sum, bool one_hot_obs) : WLG::Model(zero_sum), goal(goal)
{
    this->one_hot_obs = one_hot_obs;
    for (int i = 1; i <= max_action; i++)
        actions.push_back(i);
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return other_casted->sum == sum && other_casted->turn == turn && other_casted->terminal == terminal;
}

size_t Gamestate::hash() const
{
    return sum;
}

void Model::printState(ABS::Gamestate* state){
    auto ttt_state = dynamic_cast<Gamestate*>(state);
    std::cout << "Sum: " << ttt_state->sum << std::endl;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state){
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){
    //Apply action
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    state->sum += action;
    if(state-> sum == goal){
        state->terminal = true;
        if(state->turn == 0){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P0_WIN),1};
        }
        if(state->turn == 1){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P1_WIN),1};
        }
    }
    else if(state->sum > goal){
        state->terminal = true;
        if(state->turn == 1){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P0_WIN),1};
        }
        if(state->turn == 0){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P1_WIN),1};
        }
    }

    state->turn = 1 - state->turn;
    return {getRewards(WLG::GameResult::NOT_OVER),1};
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    return new Gamestate();
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}


int Model::getNumPlayers(){
    return 2;
}