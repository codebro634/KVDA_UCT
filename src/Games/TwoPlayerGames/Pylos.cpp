#include "../../../include/Games/TwoPlayerGames/Pylos.h"
#include <iostream>
#include <cassert>
using namespace std;

using namespace PYL;

std::vector<int> Model::obsShape() const {
    return {4,NUM_LEVELS, NUM_LEVELS, NUM_LEVELS};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for(int k = 0; k < NUM_LEVELS; k++) {
        for(int i = 0; i < NUM_LEVELS-k; i++) {
            for(int j = 0; j < NUM_LEVELS-k; j++) {
                obs[k*NUM_LEVELS*NUM_LEVELS + i*NUM_LEVELS + j] = state->levels[k][i][j] == state->turn;
                obs[NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + k*NUM_LEVELS*NUM_LEVELS + i*NUM_LEVELS + j] = state->levels[k][i][j] == 1-state->turn;
                obs[2*NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + k*NUM_LEVELS*NUM_LEVELS + i*NUM_LEVELS + j] = state->turn == 1? state->stones_p1 : state->stones_p0;
                obs[3*NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + k*NUM_LEVELS*NUM_LEVELS + i*NUM_LEVELS + j] = state->turn == 1? state->stones_p0 : state->stones_p1;
            }
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + 1, NUM_LEVELS*NUM_LEVELS*NUM_LEVELS}; //from has additional stack position
}

int Model::encodeAction(int* decoded_action) {
    int total_pos = NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + 1;
    return decoded_action[0] * total_pos + decoded_action[1];
}

Model::Model(bool zero_sum) : WLG::Model(zero_sum) {}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return other_casted->levels == levels && other_casted->turn == turn && terminal == other_casted->terminal;
}

size_t Gamestate::hash() const{
    int hash =0;
    for(int k = 0; k < NUM_LEVELS; k++) {
        hash = hash << 8;
        for(int i = 0; i < NUM_LEVELS-k; i++) {
            for(int j = 0; j < NUM_LEVELS-k; j++) {
                hash += levels[k][i][j] != EMPTY? i+3*j : 0;
            }
        }
    }
    return hash;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();
    int total_stones = 0;
    for(int k = 0; k < NUM_LEVELS; k++) {
        for (auto & i : state->levels[k]) {
            for (auto & j : i) {
                j=EMPTY;
            }
        }
        total_stones += (NUM_LEVELS-k)*(NUM_LEVELS-k);
    }
    state->stones_p0 = total_stones/2;
    state->stones_p1 = total_stones/2;
    return state;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

int Model::getNumPlayers() {
    return 2;
}

void Model::printState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    std::cout << "Player 0 stones: " << state->stones_p0 << " | Player 1 stones: " << state->stones_p1 <<  std::endl;
    std::cout << std::endl;

    for(int k = 0; k < NUM_LEVELS; k++) {
        for(int i = 0; i < NUM_LEVELS-k; i++) {
            for(int j = 0; j < NUM_LEVELS-k; j++) {
                if(state->levels[k][i][j]==EMPTY)
                    cout << "_" << " ";
                else {
                    cout << state->levels[k][i][j] << " ";
                }
            }
            cout << endl;
        }
        cout << endl;
    }
}

int Model::actionToHash(int from_l, int from_x, int from_y, int to_l, int to_x, int to_y) {
    int total_pos = NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + 1; //+1 for stack
    int hash_from = from_l == STACK_LEVEL_IDX? 0: 1+from_l*NUM_LEVELS*NUM_LEVELS + from_x*NUM_LEVELS + from_y;
    int hash_to = to_l*NUM_LEVELS*NUM_LEVELS + to_x*NUM_LEVELS + to_y;
    return hash_from*total_pos + hash_to;
}

std::tuple<int,int,int, int,int,int> Model::hashToAction(int hash) {
    int total_pos = NUM_LEVELS*NUM_LEVELS*NUM_LEVELS + 1;
    int hash_to = hash % total_pos;
    int hash_from = hash / total_pos;
    int to_l = hash_to / (NUM_LEVELS*NUM_LEVELS);
    int to_x = (hash_to / NUM_LEVELS) % NUM_LEVELS;
    int to_y = hash_to % NUM_LEVELS;

    if(hash_from > 0) {
        hash_from--;
        int from_l = hash_from / (NUM_LEVELS*NUM_LEVELS);
        int from_x = (hash_from / NUM_LEVELS) % NUM_LEVELS;
        int from_y = hash_from % NUM_LEVELS;
        return std::make_tuple(from_l,from_x,from_y,to_l,to_x,to_y);
    }else {
        return std::make_tuple(STACK_LEVEL_IDX,0,0,to_l,to_x,to_y);
    }

}


vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<int> actions;
    for(int k = 0; k < NUM_LEVELS; k++) {
        for(int i = 0; i < NUM_LEVELS-k; i++) {
            for(int j = 0; j < NUM_LEVELS-k; j++) {
                if(state->levels[k][i][j] == EMPTY) {
                    if(k == 0)
                        actions.push_back(actionToHash(STACK_LEVEL_IDX,0,0,k,i,j));
                    else if(state->levels[k-1][i][j] != EMPTY && state->levels[k-1][i+1][j] != EMPTY && state->levels[k-1][i][j+1] != EMPTY && state->levels[k-1][i+1][j+1] != EMPTY){
                        actions.push_back(actionToHash(STACK_LEVEL_IDX,0,0,k,i,j));
                        for(auto stone : state->top_level_stones) {
                            auto [l,x,y] = stone;
                            if(state->levels[l][x][y] == state->turn && (l < k && x != i && x != i+1 && y != j && y != j+1)) //only move upwards and dont move the to-be-foundation
                                actions.push_back(actionToHash(l,x,y,k,i,j));
                        }
                    }
                }
            }
        }
    }
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    //place stone
    auto [from_l,from_x,from_y,to_l,to_x,to_y] = hashToAction(action);
    state->levels[to_l][to_x][to_y] = state->turn;
    state->top_level_stones.insert(std::make_tuple(to_l,to_x,to_y));
    if(from_l != STACK_LEVEL_IDX) {
        state->top_level_stones.erase(std::make_tuple(from_l,from_x,from_y));
        state->levels[from_l][from_x][from_y] = EMPTY;
    }
    else {
        if (state->turn == 0)
            state->stones_p0--;
        else
            state->stones_p1--;
    }
    if(to_l > 0) {
        state->top_level_stones.erase(std::make_tuple(to_l-1,to_x,to_y));
        state->top_level_stones.erase(std::make_tuple(to_l-1,to_x+1,to_y));
        state->top_level_stones.erase(std::make_tuple(to_l-1,to_x,to_y+1));
        state->top_level_stones.erase(std::make_tuple(to_l-1,to_x+1,to_y+1));
    }

    //check for win
    if(to_l == NUM_LEVELS-1) {
        state->terminal = true;
        if(state->turn == 0){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P0_WIN),1};
        }
        else if(state->turn == 1){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P1_WIN),1};
        }
    }else if (state->stones_p0 == 0 || state->stones_p1 == 0) {
        state->terminal = true;
        if(state->turn == 1){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P0_WIN),1};
        }
        else if(state->turn == 0){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P1_WIN),1};
        }
    }

    state->turn = 1 - state->turn;
    return {getRewards(WLG::GameResult::NOT_OVER), 1};

}