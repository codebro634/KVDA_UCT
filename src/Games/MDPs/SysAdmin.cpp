#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/SysAdmin.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>

using namespace std;
using namespace SA;

inline bool getIthBit(int n, int i){
    return (n & (1 << i)) != 0;
}

std::vector<int> Model::obsShape() const {
    return {static_cast<int>(connections.size())};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    for (size_t i = 0; i < connections.size(); ++i)
        obs[i] = getIthBit(dynamic_cast<SA::Gamestate*>(uncasted_state)->machine_statuses, i) ? 1 : 0;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {static_cast<int>(connections.size() + 1)};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

Model::Model(){
    connections = DEFAULT_CONNECTIONS;
    actions = {};
    for(size_t i = 0; i < connections.size() + 1; i++)
        actions.push_back(i);
}

Model::Model(const std::string& fileName){
    std::ifstream file(fileName);  // Open the file

    if (!file) {
        std::cerr << "Unable to open file: " << fileName << std::endl;
        return;
    }

    file >> reboot_prob;  // Read the reboot probability
    int num_nodes;
    file >> num_nodes;  // Read the number of nodes

    std::string line;
    while (std::getline(file, line)) {  // Read each line
        if (line.empty())
            continue;
        if (line == "empty")
            connections.emplace_back();
        else{
            std::vector<int> neighbors;
            std::istringstream iss(line);
            int node;
            while (iss >> node) {  // Extract each integer (node) from the line
                neighbors.push_back(node);
            }
            connections.push_back(neighbors);  // Add the node's neighbors to the adjacency list
        }
    }

    assert (static_cast<int>(connections.size()) == num_nodes);  // Check that the number of nodes is correct


    //print connections
     // for (int i = 0; i < connections.size(); ++i) {
     //     std::cout << i << "-> ";
     //     for (int j = 0; j < connections[i].size(); ++j) {
     //         std::cout << connections[i][j] << " ";
     //     }
     //     std::cout << std::endl;
     // }

    for(size_t i = 0; i < connections.size() + 1; i++)
        actions.push_back(i);

    assert (connections.size() <= 32); // we use a 32-bit int to store the state, hence we can't have more than 32 machines
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return __builtin_popcount( state_a->machine_statuses ^ state_b->machine_statuses);
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_state = dynamic_cast<const Gamestate*>(&other);
    return machine_statuses == other_state->machine_statuses;
}

size_t Gamestate::hash() const
{
    return machine_statuses;
}

inline void setIthBit(int& n, int i, bool val){
    if(val)
        n |= (1 << i);
    else
        n &= ~(1 << i);
}

void Model::printState(ABS::Gamestate* state) {
    auto* SAState = dynamic_cast<SA::Gamestate*>(state);
    if (!SAState) return;

    for (size_t i = 0; i < connections.size(); ++i) {
        std::cout << "Machine " << i << ": " << (getIthBit(SAState->machine_statuses,i) ? "ON" : "OFF") << std::endl;
    }
}

ABS::Gamestate* Model::getInitialState(int num){
    auto* state = new SA::Gamestate();
    state->machine_statuses = num;
    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    return getInitialState((1 << connections.size()) - 1);
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
    return actions;
}

std::pair<int,int> num_neighbors(int i,  std::vector<std::vector<int>>& connections, int& statuses) {
    int num = connections[i].size();
    int num_on = 0;

    for (size_t j = 0; j < connections[i].size(); ++j) {
        if (getIthBit(statuses,connections[i][j]))
            num_on++;
    }
    return {num, num_on};
}



std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
            auto* state = dynamic_cast<SA::Gamestate*>(uncasted_state);
            auto old_statuses = state->machine_statuses;
            size_t decision_point = 0;

            double p = 1;
            double reward = (action != (int)connections.size())? -REBOOT_COST : 0;
            std::uniform_real_distribution<double> dist(0, 1);

            for(size_t i = 0; i < connections.size(); i++){
                if(action == static_cast<int>(i)){
                    // n-th action is no-reboot action{
                    setIthBit(state->machine_statuses,i,true);
                }
                else if(getIthBit(state->machine_statuses,i)){
                    auto [num, num_on] = num_neighbors(i, connections, old_statuses);
                    float stay_active_prob = 0.45 + 0.5 * (1.0 + (float) num_on) / (1.0 + (float) num);
                    if( (decision_outcomes == nullptr && dist(rng) < stay_active_prob) || (decision_outcomes != nullptr && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0))
                        p *= stay_active_prob;
                    else{
                        p *= 1 - stay_active_prob;
                        setIthBit(state->machine_statuses,i,false);
                    }
                }else{
                    //reboot with REBOOT_PROB
                    if((decision_outcomes == nullptr && dist(rng) < reboot_prob) || (decision_outcomes != nullptr && (reboot_prob == 1 || (reboot_prob != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))){
                        setIthBit(state->machine_statuses,i,true);
                        p *= reboot_prob;
                    }
                    else
                        p *= 1 - reboot_prob;
                }
                reward += getIthBit(old_statuses,i)? 1 : 0;
            }

            return {{reward}, p};
        }