
#include "../../../include/Games/MDPs/ToySoccer.h"
#include <iostream>
#include <cassert>
using namespace std;

using namespace TS;


bool Gamestate::operator==(const ABS::Gamestate& other) const {
    return state == dynamic_cast<const Gamestate&>(other).state && terminal == dynamic_cast<const Gamestate&>(other).terminal;
}

size_t Gamestate::hash() const {
    return std::hash<int>()(state);
}

void Model::printState(ABS::Gamestate* state) {
    std::cout << "state: " << dynamic_cast<TS::Gamestate*>(state)->state << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)  {
    auto* state = new TS::Gamestate();
    state->state = 0;
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
    switch (dynamic_cast<Gamestate*>(uncasted_state)->state) {
        case 0:
            return {0,1,2};
        case 1:
            return {0};
        case 2:
            return {1};
        case 3:
            return {1};
        default:
            assert(false);
    }
    throw std::runtime_error("Invalid state");
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
        auto* state = dynamic_cast<TS::Gamestate*>(uncasted_state);
        size_t decision_point = 0;

        switch(state->state) {
            case 0:
                if (action == 0) {
                    state->state = 1;
                    return {  {0}  ,1  };
                } else if (action == 1) {
                    state->state = 4;
                    state->terminal = true;
                    return {  {0}  ,1  };
                } else if (action == 2) {
                    std::discrete_distribution<int> dist({0.5, 0.5});
                    int sampled = decision_outcomes == nullptr? dist(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes);
                    state->state = 2 + sampled;
                    return {  {0}  ,0.5  };
                }else{
                    throw std::runtime_error("Invalid action");
                }
            case 1:
                state->state = 4;
                state->terminal = true;
                return {  {0}  ,1  };
            case 2:
                state->state = 4;
                state->terminal = true;
                return {  {0}  ,1  };
            case 3:
                state->state = 4;
                state->terminal = true;
                return {  {0}  ,1  };
            default:
                throw std::runtime_error("Invalid state");
        }


}