
#include "../../../include/Games/MDPs/CrossingTraffic.h"
#include <iostream>
#include <cassert>

using namespace std;
using namespace CT;

inline bool obstacle_in_lane(int lane_state, int pos) {
    return (lane_state & (1 << pos)) != 0;
}

std::vector<int> Model::obsShape() const {
    return {2, width, height};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            obs[i * height + j] = obstacle_in_lane(dynamic_cast<CT::Gamestate*>(uncasted_state)->obstacles[j], i) ? 1 : 0;
            obs[width * height + i * height + j] = (dynamic_cast<CT::Gamestate*>(uncasted_state)->x == i && dynamic_cast<CT::Gamestate*>(uncasted_state)->y == j) ? 1 : 0;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return idle_action? std::vector<int>({5}) : std::vector<int>({4});
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0] - idle_action;
}

Model::Model() {
    //Default params
    width = 6;
    height = 6;
    spawn_rate = 0.2;
    idle_action = false;
}

Model::Model(int width, int height, float spawn_rate, bool idle_action){
    this->width = width;
    this->height = height;
    this->spawn_rate = spawn_rate;
    this->idle_action = idle_action;
}

inline void advance_lane(int& lane, float spawn_rate, int width, bool new_car) {
    lane = ((lane << 1) | new_car) & ((1 << width) - 1);
}

void Model::printState(ABS::Gamestate* state) {
    auto* CTstate = dynamic_cast<CT::Gamestate*>(state);
    //print lanes
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++)
            cout << (CTstate->x == j && CTstate->y == i ? "X" : obstacle_in_lane(CTstate->obstacles[i],j) ? "O" : ".");
        cout << endl;
    }
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)  {
    auto* state = new CT::Gamestate();

    state->x = 0;
    state->y = 0;
    state->obstacles = std::vector<int>(height, 0);
    auto dis = std::uniform_real_distribution<>(0, 1);
    for(int i = 0; i < width; i++) {
        for(int j = 1; j < height-1; j++) {
            bool spawn = dis(rng) < spawn_rate;
            advance_lane(state->obstacles[j], spawn_rate, width, spawn);
        }
    }

    return state;
}

int Model::getNumPlayers() {
    return 1;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return x == other_state->x && y == other_state->y && obstacles == other_state->obstacles;
}

//no collision if there are <= 3 lanes
size_t Gamestate::hash() const{
    size_t obstacle_hash = 0;
    for (size_t i = 1; i < obstacles.size()-1; i++) {
        obstacle_hash |= obstacles[i] << (i * 8);
    }
    return (x+1) | ((y+1) << 4) | obstacle_hash;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}


std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return idle_action? std::vector<int>({-1,0,1,2,3}) : std::vector<int>({0,1,2,3});
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
        auto* state = dynamic_cast<CT::Gamestate*>(uncasted_state);

        Gamestate old_state = *state;
        size_t decision_point = 0;

        //absorbing goal state
        if (state->x == 0 && state->y == height-1) {
            return { {0}, 1.0 };
        }

        //move robot
        if(state->x != -1 && obstacle_in_lane(state->obstacles[state->y], state->x)) {
            state->x = -1; //robot is removed from the map if it collides with an obstacle
            state->y = -1;
        }else if(state->x != -1) {
            state->x += action == 0 ? 1 : action == 2 ? -1 : 0;
            state->y += action == 1 ? 1 : action == 3 ? -1 : 0;
            state->x = std::max(0, std::min(width-1, state->x));
            state->y = std::max(0, std::min(height-1, state->y));
        }

        //advance lanes
        auto dis = std::uniform_real_distribution<>(0, 1);
        double p = 1;
        for(int j = 1; j < height-1; j++) { //ignore top and bottom lanes
            bool spawn = (decision_outcomes == nullptr && dis(rng) < spawn_rate) || (decision_outcomes != nullptr && (spawn_rate == 1 || (spawn_rate != 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))));
            advance_lane(state->obstacles[j], spawn_rate, width, spawn);
            if(spawn) {
                p *= spawn_rate;
            }else
                p *= 1 - spawn_rate;
        }

        return {  {-1}  ,p  };
}