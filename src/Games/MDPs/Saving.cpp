#include "../../../include/Games/MDPs/Saving.h"

#include <cassert>

using namespace SAVING;

std::vector<int> Model::obsShape() const {
    return {3};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state= dynamic_cast<SAVING::Gamestate*>(uncasted_state);
    obs[0] = state->steps_since_borrow;
    obs[1] = state->steps_since_invest;
    obs[2] = state->price;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {4};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

Model::Model(int pmin, int pmax, int tinvest, int tborrow) : pmin(pmin), pmax(pmax), tinvest(tinvest), tborrow(tborrow){
    assert(pmin <= pmax);
}

//Default values
Model::Model(){
    pmin = -4;
    pmax = 4;
    tinvest = 4;
    tborrow = 4;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto other_state = dynamic_cast<const Gamestate*>(&other);
    return price == other_state->price && steps_since_borrow == other_state->steps_since_borrow && steps_since_invest == other_state->steps_since_invest;
}

size_t Gamestate::hash() const{
    return (static_cast<size_t> (std::abs(steps_since_borrow)) | (std::abs(steps_since_invest) << 12) | (std::abs(price) << 24));
}

void Model::printState(ABS::Gamestate* state) {
    auto* SAState = dynamic_cast<SAVING::Gamestate*>(state);
    if (!SAState) return;

    std::cout << "Last borrowed: " << SAState->steps_since_borrow << std::endl;
    std::cout << "Last invested: " << SAState->steps_since_invest << std::endl;
    std::cout << "Price: " << SAState->price << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new SAVING::Gamestate();
    state->steps_since_borrow = tborrow;
    state->steps_since_invest = tinvest;
    std::uniform_int_distribution<int> dist(pmin, pmax);
    state->price = dist(rng);
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
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    bool can_borrow = state->steps_since_borrow >= tborrow;
    bool can_invest = state->steps_since_invest >= tinvest;
    std::vector<int> actions = {0}; //save
    if (can_borrow) actions.push_back(1); //borrow
    if (can_invest) actions.push_back(2); //invest
    else actions.push_back(3); //sell
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
        auto* state = dynamic_cast<SAVING::Gamestate*>(uncasted_state);
        size_t decision_point = 0;

        state->steps_since_borrow++;
        state->steps_since_invest++;

        double reward = 0;
        switch(action) {
            case 0: //save
                reward = SAVE_REWARD;
                break;
            case 1: //borrow
                state->steps_since_borrow = 0;
                reward = BORROW_INSTANT_REWARD;
                break;
            case 2: // invest
                state->steps_since_invest = 0;
                reward = 0;
                break;
            case 3: //sell
                state->steps_since_invest += tinvest + 1;
                reward = state->price;
                break;
            default:
                throw std::runtime_error("Unknown action");
        }

        if(state->steps_since_borrow == tborrow)
            reward -= BORROW_REPAY_COST;

        //sample new price
        std::uniform_int_distribution<int> dist(pmin, pmax);
        state->price = decision_outcomes == nullptr? dist(rng) : getDecisionPoint(decision_point, pmin, pmax, decision_outcomes);
        return {std::vector<double>{reward}, 1.0 / (double)(1+pmax-pmin)};
}
