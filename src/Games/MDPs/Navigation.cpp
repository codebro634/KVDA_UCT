//
// Created by chris on 09.10.2024.
//

#include <iostream>
#include <fstream>
#include <sstream>

#include "../../../include/Games/MDPs/Navigation.h"

using namespace Navigation;

std::vector<int> Model::obsShape() const {
    return {2}; // x and y coordinate
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    obs[0] = state->position.first;
    obs[1] = state->position.second;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {static_cast<int>(4 + (idle_action ? 1 : 0))}; // 4 directions + idle action
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0] - (static_cast<int>(idle_action));
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return std::abs(state_a->position.first - state_b->position.first) + std::abs(state_a->position.second - state_b->position.second);
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_state = dynamic_cast<const Gamestate*>(&other);
    return position.first == other_state->position.first && position.second == other_state->position.second && terminal == other_state->terminal;
}

size_t Gamestate::hash() const
{
    // Order of XOR should matter
    return (static_cast<size_t>(position.first) << 16) | position.second; //more conversative shift to avoid overflow for 32-bit systems
}

void Model::printState(ABS::Gamestate* state){
    auto nav_state = dynamic_cast<Gamestate*>(state);
    auto position = nav_state->position;
    for (int y = 0; y < size_.second; y++)
    {
        for (int x = 0; x < size_.first; x++)
        {
            const float prob = map[y][x];
            if (x == position.first && y == position.second) std::cout << "o ";
            else if (prob <= 0) std::cout << "_ ";
            else if (prob <= 0.3333) std::cout << "- ";
            else if (prob <= 0.666) std::cout << "+ ";
            else std::cout << "x ";

        }
        std::cout << std::endl;
    }

    std::cout << "0 Right, 1 Down, 2 Left, 3 Up" << std::endl;
}

inline bool Model::isAllowedAction(ABS::Gamestate* uncasted_state, int action) const{
    // 0 (1, 0); 1 (0, 1); 2 (-1, 0); 3 (0, -1)
    const auto state = dynamic_cast<Gamestate*>(uncasted_state);
    std::vector<int> actions;
    auto [x, y] = state->position;
    auto [mapX, mapY] = size_;
    switch (action)
    {
        case 0: return x < mapX - 1;
        case 1: return y < mapY - 1;
        case 2: return x > 0;
        case 3: return y > 0;
        default: return false;
    }
}


std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state){
    std::vector<int> actions;

    // 0 (1, 0); 1 (0, 1); 2 (-1, 0); 3 (0, -1)
    // -1 Idle
    // 0 Right
    // 1 Down
    // 2 Left
    // 3 Up

    if(idle_action)
        actions.push_back(-1);

    for (int action = 0; action < 4; action++)
    {
        if (isAllowedAction(uncasted_state, action)) actions.push_back(action);
    }
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    if (!isAllowedAction(uncasted_state, action)) return {{-1},1.0};
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    size_t decision_point = 0;

    switch (action)
    {
        case 0: state->position.first += 1; break;
        case 1: state->position.second += 1; break;
        case 2: state->position.first -= 1; break;
        case 3: state->position.second -= 1; break;
        default: break; // Unreachable
    }

    if (state->position == goal){
        state->terminal = true;
        return {{0}, 1.0};
    }

    double reward = -1 + (state_dependent_rewards? (state->position.first * 10 + state->position.second) / 1000.0 : 0);

    if (state->position == spawn){
        return {{reward}, 1.0};
    }

    double prob = map[state->position.second][state->position.first];
    if ( (decision_outcomes == nullptr && dist(rng) < prob) || (decision_outcomes != nullptr && (prob == 1 || (prob != 0 && getDecisionPoint(decision_point, 0, 1, decision_outcomes) == 0))))
    {
        state->position = spawn;
        return {{reward},  prob};
    }

    return {{reward}, 1 - prob};
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();
    state->position = spawn;
    return state;
}

Model::Model(const std::string& fileName, bool idle_action, bool state_dependent_rewards) : dist(0.0, 1.0), idle_action(idle_action), state_dependent_rewards(state_dependent_rewards){
    std::ifstream file(fileName);
    if (!file.is_open()){
        std::cerr << "Could not open file " << fileName << std::endl;
        exit(1);
    }

    std::string line;

    // Get start and goal
    if (std::getline(file, line)){
        std::istringstream iss(line);
        std::string number;
        std::vector<int> start;
        while (std::getline(iss, number, ',')) start.push_back(std::stoi(number));
        spawn = {start[0], start[1]};
    }

    if (std::getline(file, line)){
        std::istringstream iss(line);
        std::string number;
        std::vector<int> goal;
        while (std::getline(iss, number, ',')) goal.push_back(std::stoi(number));
        this->goal = {goal[0], goal[1]};
    }

    while (std::getline(file, line)){
        std::vector<double> row;
        std::istringstream iss(line);
        std::string number;

        while (std::getline(iss, number, ',')) row.push_back(std::stod(number));
        map.push_back(row);
    }

    size_ = {map[0].size(), map.size()};

    assert (map[spawn.second][spawn.first] == 0);
}



ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}


int Model::getNumPlayers(){
    return 1;
}