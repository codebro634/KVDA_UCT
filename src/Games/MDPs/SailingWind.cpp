
#include "../../../include/Games/MDPs/SailingWind.h"
#include <iostream>
#include <cassert>

using namespace std;
using namespace SW;

//From https://github.com/dair-iitd/oga-uct/blob/master/OGA/sailing/sailing.h
std::pair<int, int> getDirection(int a)  {
    switch( a ) {
    case 0: return std::make_pair(0, 1);
    case 1: return std::make_pair(1, 1);
    case 2: return std::make_pair(1, 0);
    case 3: return std::make_pair(1, -1);
    case 4: return std::make_pair(0, -1);
    case 5: return std::make_pair(-1, -1);
    case 6: return std::make_pair(-1, 0);
    case 7: return std::make_pair(-1, 1);
    default: return std::make_pair(-1, -1);
    }
}

inline bool inLake(int x, int y, int rows, int cols) {
    return (x >= 0) && (x < rows) && (y >= 0) && (y < cols);
}


std::vector<int> Model::obsShape() const {
    return {2, ROWS, COLS};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state= dynamic_cast<SW::Gamestate*>(uncasted_state);
    int msize = ROWS * COLS;
    for (int i = 0; i < COLS; i++) {
        for (int j = 0; j < ROWS; j++) {
            obs[0*msize + i * ROWS + j] = state->x == i && state->y == j ? 1 : 0;
            obs[1*msize + i * ROWS + j] = state->wind_dir;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {8};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

[[nodiscard]] std::string Gamestate::toString() const {
    return "((" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(wind_dir) + ")" + "," + ABS::Gamestate::toString() + ")";
}

ABS::Gamestate* Model::deserialize(std::string &ostring) const {
    auto* state = new Gamestate();
    int x, y, wind_dir, turn, terminal;
    sscanf(ostring.c_str(), "((%d,%d,%d),(%d,%d))", &x, &y, &wind_dir, &turn, &terminal);
    state->x = x;
    state->y = y;
    state->wind_dir = wind_dir;
    state->turn = turn;
    state->terminal = terminal;
    return state;
}

Model::Model(const int rows, const int cols, const bool deterministic) {
    ROWS = rows;
    COLS = cols;
    wind_probs = deterministic ? DETERMINISTIC_WIND_PROBS : STOCHASTIC_WIND_PROBS;
}

Model::Model(const bool deterministic) {
    ROWS = DEFAULT_ROWS;
    COLS = DEFAULT_COLS;
    wind_probs = deterministic ? DETERMINISTIC_WIND_PROBS : STOCHASTIC_WIND_PROBS;
}

std::vector<double> Model::heuristicsValue (ABS::Gamestate* state) {
    auto* swState = dynamic_cast<SW::Gamestate*>(state);
    int dx = std::abs(ROWS - swState->x -1);
    int dy = std::abs(COLS - swState->y -1);
    return {(double) - (dx + dy)};
}

void Model::printState(ABS::Gamestate* state) {
    auto* swState = dynamic_cast<SW::Gamestate*>(state);
    if (!swState) return;
    std::cout << "x: " << swState->x << " y: " << swState->y << " wind_dir: " << swState->wind_dir << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)  {
    auto* state = new SW::Gamestate();

    state->x = 0;
    state->y = 0;
    state->wind_dir = 0;

    return state;
}

int Model::getNumPlayers() {
    return 1;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return x == other_state->x && y == other_state->y && wind_dir == other_state->wind_dir && terminal == other_state->terminal;
}

size_t Gamestate::hash() const
{
    return (static_cast<size_t> (x) | (y << 12) | (wind_dir << 24));
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

int windFaceVal(int a, int w)  {
    int d = a - w;
    d = d < 0 ? -d : d;
    return d < 8 - d ? d : 8 - d;
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return std::abs(state_a->x - state_b->x) + std::abs(state_a->y - state_b->y) + windFaceVal(state_a->wind_dir, state_b->wind_dir);
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto wind_direction  = getDirection(state->wind_dir);
    std::vector<int> actions;
    for (int i = 0; i < 8; i++) {
        auto dir = getDirection(i);
        if (inLake(state->x + dir.first, state->y + dir.second, ROWS, COLS) && (wind_direction.first != -dir.first || wind_direction.second != -dir.second))
            actions.push_back(i);
    }
    return actions;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
        auto* state = dynamic_cast<SW::Gamestate*>(uncasted_state);
        size_t decision_point = 0;

        auto dir = getDirection(action);
        state->x += dir.first;
        state->y += dir.second;
        double reward = -(windFaceVal(action, state->wind_dir) + 1);
        std::discrete_distribution<> dist(wind_probs[state->wind_dir].begin(), wind_probs[state->wind_dir].end());
        int old_wind_dir = state->wind_dir;
        int wind_dir;
        if (decision_outcomes == nullptr)
            wind_dir = dist(rng);
        else {
            std::vector<int> non_zero;
            for(size_t i = 0; i < wind_probs[state->wind_dir].size(); i++) {
                if(wind_probs[state->wind_dir][i] > 0)
                    non_zero.push_back(i);
            }
            wind_dir = non_zero[getDecisionPoint(decision_point,0,non_zero.size()-1,decision_outcomes)];
        }
        state->wind_dir = wind_dir;

        if (state->x == ROWS-1 && state->y == COLS-1)
            state->terminal = true;
        return {  {reward}  ,wind_probs[old_wind_dir][state->wind_dir]};
}