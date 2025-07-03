#include "../../../include/Games/MDPs/Elevators.h"

#include <cassert>
#include <iostream>
#include <fstream>

using namespace ELE;

std::vector<int> Model::obsShape() const {
    return {num_floors * (7+ + num_elevators)};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for (int i = 0; i < num_floors; ++i) {
        obs[i] = state->person_waiting_up[i] ? 1 : 0;
        obs[num_floors + i] = state->person_waiting_down[i] ? 1 : 0;
        obs[2*num_floors + i] = state->person_in_elevator_going_up[i] ? 1 : 0;
        obs[3*num_floors + i] = state->person_in_elevator_going_down[i] ? 1 : 0;
        obs[4*num_floors + i] = state->elevator_dir_up[i] ? 1 : 0;
        obs[5*num_floors + i] = state->elevator_closed[i] ? 1 : 0;
        for (int j = 0;j < num_elevators; j++)
            obs[(6+j)*num_floors + i] = 0;
    }

    for (int i = 0; i < num_elevators; ++i)
        obs[(6+i)*num_floors + state->elevator_floor[i]] = 1;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    auto shape = std::vector<int>();
    for (int i = 0; i < num_elevators; ++i)
        shape.push_back(5);
    return shape;
}

int Model::encodeAction(int* decoded_action) {
    int action = 0;
    int pow = 1;
    for (int i = 0; i < num_elevators; ++i) {
        action += decoded_action[i] * pow;
        pow *= 5;
    }
    return action;
}

std::string Gamestate::toString() const {
    std::string s = "ElevatorsGamestate:\n";
    for (size_t f = 0; f < person_waiting_up.size(); ++f) {
        s += "Floor " + std::to_string(f) + ": waiting_up=" + std::to_string(person_waiting_up[f]) +
             ", waiting_down=" + std::to_string(person_waiting_down[f]) + "\n";
    }
    for (size_t e = 0; e < elevator_dir_up.size(); ++e) {
        s += "Elevator " + std::to_string(e) + ": dir_up=" + std::to_string(elevator_dir_up[e]) +
             ", closed=" + std::to_string(elevator_closed[e]) +
             ", in_elevator_going_up=" + std::to_string(person_in_elevator_going_up[e]) +
             ", in_elevator_going_down=" + std::to_string(person_in_elevator_going_down[e])  +
            ", floor=" + std::to_string(elevator_floor[e]) + "\n";
    }
    return s;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const {
    const auto* o = dynamic_cast<const Gamestate*>(&other);
    return (person_waiting_up == o->person_waiting_up) && (person_waiting_down == o->person_waiting_down)
    &&  (person_in_elevator_going_up == o->person_in_elevator_going_up)
    &&  (person_in_elevator_going_down == o->person_in_elevator_going_down)
    &&  (elevator_dir_up == o->elevator_dir_up)
    &&  (elevator_closed == o->elevator_closed)
    && (elevator_floor == o->elevator_floor);
}

/**
 Bit assignment (no-collision if <= 2 elevators and <= 8 floors)
        1-6 waiting_up
        7-12 waiting down
        13-15 person_in_elev going up
        16-18 person in elev goin down
        19-21 dir elev up?
        22-24 elev closed?
        25-27 elev1 floor
        28-31 elev2 floor
        32-34 elev3 floor
        ...
 **/
size_t Gamestate::hash() const {
    return state_hash;
}

Model::Model(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file at " << filePath << std::endl;
        std::exit(1);
    }

    file >> num_elevators;
    file >> num_floors;
    ARRIVE_PARAM.resize(num_floors,0.0);
    for (int i = 0; i < num_floors; ++i) {
        file >> ARRIVE_PARAM[i];
    }

    file.close();

    //setup action list
    action_list = {0};
    int pow = 1;
    for(int e = 0; e < num_elevators; ++e){
        auto tmp_list = action_list;
        action_list.clear();
        for(int action : tmp_list){
            for(int i = 0; i < 5; ++i){
                action_list.push_back(action + i*pow);
            }
        }
        pow*=5;

    }
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return __builtin_popcount( state_a->hash() ^ state_b->hash());
}

void Model::printState(ABS::Gamestate* uncasted_state) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);
    if (state) {
        std::cout << state->toString() << std::endl;
    } else {
        std::cerr << "Invalid state type" << std::endl;
    }
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    return action_list;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto* state = new Gamestate();

    state->person_waiting_up.resize(num_floors, false);
    state->person_waiting_down.resize(num_floors, false);
    state->person_in_elevator_going_up.resize(num_elevators, false);
    state->person_in_elevator_going_down.resize(num_elevators, false);
    state->elevator_dir_up.resize(num_elevators, true);
    state->elevator_closed.resize(num_elevators, true);
    state->elevator_floor.resize(num_elevators, 0);

    state->state_hash = 0;
    state->state_hash |= ((1 << num_elevators) -1) << 18; //all elevators go up initially
    state->state_hash |= ((1 << num_elevators) -1) << 21; //all elevators closed initially

    return state;
}

int Model::getNumPlayers() {
    return 1;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);
    if (!state)
        return nullptr;

    auto* new_state = new Gamestate(*state);
    return new_state;
}

std::pair<std::vector<double>, double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);
    if (!state)
        throw std::runtime_error("Invalid state type");

    Gamestate old_state = *state;
    double p = 1.0;
    size_t decision_point = 0;
    
    std::vector<bool> move_current_dir(num_elevators, false);
    std::vector<bool> open_door_going_up(num_elevators, false);
    std::vector<bool> open_door_going_down(num_elevators, false);
    std::vector<bool> close_door(num_elevators, false);
    std::uniform_real_distribution<> dis(0.0, 1.0);

    if(action != -1) { // -1 is idle action
        for (int e = 0; e < num_elevators; ++e) {
            int action_e = action % 5;
            action /= 5;
            if (action_e == 0) {
                move_current_dir[e] = true;
            } else if (action_e == 1) {
                open_door_going_up[e] = true;
            } else if (action_e == 2) {
                open_door_going_down[e] = true;
            } else if (action_e == 3) {
                close_door[e] = true;
            } //4 is idle
        }
    }

    for(int e = 0; e < num_elevators; ++e){
        int f = old_state.elevator_floor[e];
        bool open_door = open_door_going_up[e] || open_door_going_down[e] || (!old_state.elevator_closed[e] && !close_door[e]);

        //Update elevator state
        state->elevator_closed[e] = !open_door;
        state->elevator_dir_up[e] = (!open_door && old_state.elevator_dir_up[e]) || open_door_going_up[e];
        if(state->elevator_closed[e])
            state->state_hash |= 1 << (21+e); //elevator closed
        else
            state->state_hash &= ~(1 << (21+e)); //elevator closed
        if (state->elevator_dir_up[e])
            state->state_hash |= 1 << (18+e); //elevator dir up
        else
            state->state_hash &= ~(1 << (18+e)); //elevator dir up

        if(!open_door && move_current_dir[e]){
            state->state_hash &= ~(f << (24+3*e)); //elevator floor
            if(old_state.elevator_dir_up[e]){
                state->elevator_floor[e] = std::min(f+1,num_floors-1);
            }else{
                state->elevator_floor[e] = std::max(0,f-1);
            }
            state->state_hash |= state->elevator_floor[e] << (24+3*e); //elevator floor
        }

        // Board passengers
        if(!old_state.elevator_closed[e] && old_state.elevator_dir_up[e]){
            state->person_in_elevator_going_up[e] = old_state.person_waiting_up[f] || old_state.person_in_elevator_going_up[e];
            state->person_waiting_up[f] = false;
            state->state_hash &= ~(1 << f); //waiting up
            if(state->person_in_elevator_going_up[e]){
                state->state_hash |= 1 << (e+12); //in elevator going up
            }else {
                state->state_hash &= ~(1 << (12+e)); //in elevator going up
            }
        }
        if(!old_state.elevator_closed[e] && !old_state.elevator_dir_up[e]){
            state->person_in_elevator_going_down[e] = old_state.person_waiting_down[f] || old_state.person_in_elevator_going_down[e];
            state->person_waiting_down[f] = false;
            state->state_hash &= ~(1 << (6+f)); //waiting down
            if(state->person_in_elevator_going_down[e]){
                state->state_hash |= 1 << (15+e); //in elevator going down
            }else {
                state->state_hash &= ~(1 << (15+e)); //in elevator going down
            }
        }

        // Deboard passengers
        if(old_state.elevator_floor[e] == num_floors-1){
            state->person_in_elevator_going_up[e] = false;
            state->state_hash &= ~(1 << (12+e)); //in elevator going up
        }
        if(old_state.elevator_floor[e] == 0){
            state->person_in_elevator_going_down[e] = false;
            state->state_hash &= ~(1 << (15+e)); //in elevator going down
        }

    }

    //Add random arrivals
    for(int f = 0; f < num_floors; ++f){
        if (!state->person_waiting_up[f]) {
            if( (decision_outcomes == nullptr && dis(rng) < ARRIVE_PARAM[f]) || (decision_outcomes != nullptr && (ARRIVE_PARAM[f] == 1 || (ARRIVE_PARAM[f] != 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))){
                state->person_waiting_up[f] = true;
                p *= ARRIVE_PARAM[f];
                state->state_hash |= 1 << f; //waiting up
            }else
                p *= 1-ARRIVE_PARAM[f];
        }
        if (!state->person_waiting_down[f]) {
            if( (decision_outcomes == nullptr && dis(rng) < ARRIVE_PARAM[f]) || (decision_outcomes != nullptr && (ARRIVE_PARAM[f] == 1 || (ARRIVE_PARAM[f] != 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))){
                state->person_waiting_down[f] = true;
                p *= ARRIVE_PARAM[f];
                state->state_hash |= 1 << (6+f); //waiting down
            }else
                p *= 1-ARRIVE_PARAM[f];
        }
    }

    // Compute reward
    double reward = 0.0;
    for (int e = 0; e < num_elevators; ++e) {
        if (old_state.person_in_elevator_going_up[e]) {
            reward -= old_state.elevator_dir_up[e] ? ELEVATOR_PENALTY_RIGHT_DIR : ELEVATOR_PENALTY_WRONG_DIR;
        }
        if (old_state.person_in_elevator_going_down[e]) {
            reward -= !old_state.elevator_dir_up[e] ? ELEVATOR_PENALTY_RIGHT_DIR : ELEVATOR_PENALTY_WRONG_DIR;
        }
    }

    for (int f = 0; f < num_floors; ++f) {
        if (old_state.person_waiting_up[f]) {
            reward -= PENALTY_WAITING;
        }
        if (old_state.person_waiting_down[f]) {
            reward -= PENALTY_WAITING;
        }
    }

    return {{reward},p};
}