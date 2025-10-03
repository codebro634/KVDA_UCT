#include "../../../include/Games/MDPs/TriangleTireworld.h"
#include <fstream>
#include <sstream>
#include <cmath>

using namespace TRT;

std::vector<int> Model::obsShape() const {
    return {2*(int)connections.size()};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for(int i=0;i< (int)connections.size();i++){
        obs[i] = state->vehicle_pos == (int)i? 1:0;
        obs[i+connections.size()] = state->spare_tires[i]? 1:0;
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    int size = idle_action? 1:0;
    size += connections.size()+2;
    return {size};
}

int Model::encodeAction(int* decoded_action) {
    int action = decoded_action[0];
    if (idle_action)
        action--;
    return action;
}

bool Gamestate::operator==(const ABS::Gamestate& o) const {
    const auto* other = dynamic_cast<const Gamestate*>(&o);
    return vehicle_pos == other->vehicle_pos && notflattire == other->notflattire
    && hasspare == other->hasspare && spare_tires == other->spare_tires && terminal == other->terminal;
}

size_t Gamestate::hash() const {
    size_t hash = 0;
    for(bool b : spare_tires)
        hash += b? 1:0;
   return vehicle_pos | (notflattire << 5) | (hasspare << 6) | (hash << 7);
}

Model::Model(const std::string& filePath, bool idle_action, bool reduced_action_space, bool big_payoff){
    this->reduced_action_space = reduced_action_space;
    this->idle_action = idle_action;
    this->big_payoff = big_payoff;

    std::ifstream in(filePath);
    if(!in.is_open()){
        std::cerr << "Could not open file: " << filePath << std::endl;
        exit(1);
    }

    in >> flat_prob;

    int roads, m;
    in >> roads;
    in >> m;
    connections.resize(roads, std::vector<int>());
    for(int i=0;i<m;i++){
        int from,to;
        in >> from >> to;
        connections[from].push_back(to);
    }

    in >> goal_loc;
    in >> init_pos;
    int num_spares;
    in >> num_spares;
    for(int i=0;i<num_spares;i++){
        int loc;
        in >> loc;
        init_spare.push_back(loc);
    }
}

void Model::printState(ABS::Gamestate* uncasted_state){
    auto* s = dynamic_cast<Gamestate*>(uncasted_state);
    std::cout << "Vehicle pos:" << s->vehicle_pos << std::endl;
    std::cout << "Flat tire:" << !s->notflattire << std::endl;
    std::cout << "Goal loc:" << goal_loc << std::endl;
    std::cout << "Has spare:" << s->hasspare << std::endl;
    std::cout << "Spare tires:";
    for(bool b : s->spare_tires)
        std::cout << b << " ";
    std::cout << std::endl;
    std::cout << "Connections:" << std::endl;
    for(size_t i=0;i<connections.size();i++){
        std::cout << i << ":";
        for(int j : connections[i])
            std::cout << j << " ";
        std::cout << std::endl;
    }
}

ABS::Gamestate* Model::getInitialState(std::mt19937&){
    auto* s = new Gamestate();
    s->vehicle_pos = init_pos;
    s->notflattire = true;
    s->hasspare = false;
    s->spare_tires = std::vector<bool>(connections.size(), false);
    for(int i : init_spare)
        s->spare_tires[i] = true;
    return s;
}

int Model::getNumPlayers(){
    return 1;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state){
    auto* sOld = dynamic_cast<Gamestate*>(uncasted_state);
    auto* sNew = new Gamestate();
    *sNew = *sOld;
    return sNew;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state){
    std::vector<int> actions ={0,1}; //load tire, change tire
    if(idle_action)
        actions.push_back(-1);
    if(reduced_action_space) {
        auto state = dynamic_cast<Gamestate*>(uncasted_state);
        for(int i : connections[state->vehicle_pos])
            actions.push_back(2+i);
    }else {
        for(size_t i = 0; i < connections.size(); i++)
            actions.push_back(2+i);
    }
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){

    auto newState = dynamic_cast<Gamestate*>(uncasted_state);
    auto oldState = *newState;
    size_t decision_point = 0;

    int move_loc = -1;
    bool loadtire = action == 0;
    bool changetire = action == 1;
    if(action > 1)
        move_loc = action - 2;

    double outcomeProb=1.0;

    if(oldState.notflattire && move_loc != -1) {
        if (reduced_action_space)
            newState->vehicle_pos = move_loc;
        else {
            bool valid = false;
            for (int i : connections[oldState.vehicle_pos]) {
                if (i == move_loc) {
                    valid = true;
                    break;
                }
            }
            if (valid)
                newState->vehicle_pos = move_loc;
        }
    }

    if(loadtire && oldState.spare_tires[oldState.vehicle_pos])
        newState->spare_tires[oldState.vehicle_pos] = false;

    if(oldState.notflattire && move_loc != -1) {
        std::bernoulli_distribution d(flat_prob);
        newState->notflattire = decision_outcomes == nullptr || flat_prob == 0 || flat_prob == 1? !d(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0; //The RDDL version negates this, we believe this to be a mistake.
        outcomeProb = newState->notflattire ? 1-flat_prob : flat_prob;
    }else if(changetire && oldState.hasspare)
        newState->notflattire = true;

    if(changetire && oldState.hasspare)
        newState->hasspare = false;
    else if(loadtire && oldState.spare_tires[oldState.vehicle_pos]) {
        newState->hasspare = true;
        newState->spare_tires[oldState.vehicle_pos] = false;
    }

    double reward;
    if(oldState.vehicle_pos == goal_loc) {
        newState->terminal = true;
        reward = big_payoff? 100 : 1;
    }else
        reward = -1;


    std::vector<double> rew(1, reward);
    return std::make_pair(rew, outcomeProb);
}

