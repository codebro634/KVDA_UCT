#include "../../../include/Games/TwoPlayerGames/TicTacToe.h"

#include <cstring>


using namespace TTT;

std::vector<int> Model::obsShape() const {
    return {2,3, 3};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            obs[i*3+j] = state->board[i][j] == state->turn;
            obs[9+i*3+j] = state->board[i][j] == 1-state->turn;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {9};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

Model::Model(bool zero_sum) : WLG::Model(zero_sum) {}

bool Gamestate::operator==(const ABS::Gamestate& other) const {
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return board == other_casted->board && turn == other_casted->turn && terminal == other_casted->terminal;
}

size_t Gamestate::hash() const {
    size_t hash = 0;
    for (auto & row : board) {
        for (int player : row) {
            hash = hash * 2 + player + 1;
        }
    }
    return hash;
}


void Model::printState(ABS::Gamestate* state){
    auto ttt_state = dynamic_cast<Gamestate*>(state);
    for (auto & i : ttt_state->board) {
        for (int j : i) {
            if(j==EMPTY)
                std::cout << "_" << " ";
            else
                std::cout << j << " ";
        }
        std::cout << std::endl;
    }
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    std::vector<int> actions;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (state->board[i][j] == EMPTY) {
                actions.push_back(i*3+j);
            }
        }
    }
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){

    //Apply action
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int row = action/3;
    int col = action%3;
    if(state->board[row][col]==EMPTY)
        state->board[row][col]=state->turn;

    //Check for game over
    bool win = false;
    //check horizontally and vertically
    for (int i = 0; i < 3; i++) {
        if (state->board[i][0] == state->turn && state->board[i][1] == state->turn && state->board[i][2] == state->turn) {
            win = true;
            break;
        }
        if (state->board[0][i] == state->turn && state->board[1][i] == state->turn && state->board[2][i] == state->turn) {
            win = true;
            break;
        }
    }

    //check diagonals
    if (!win && state->board[0][0] == state->turn && state->board[1][1] == state->turn && state->board[2][2] == state->turn)
        win = true;
    if (!win && state->board[0][2] == state->turn && state->board[1][1] == state->turn && state->board[2][0] == state->turn)
        win = true;

    if (win) {
        state->terminal = true;
        if(state->turn == 0){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P0_WIN),1};
        }
        else if(state->turn == 1){
            state->terminal = true;
            return {getRewards(WLG::GameResult::P1_WIN),1};
        }
    }else{
        bool empty_tile = false;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (state->board[i][j] == EMPTY) {
                    empty_tile = true;
                    break;
                }
            }
        }
        if (!empty_tile) {
            state->terminal=true;
            return {getRewards(WLG::GameResult::DRAW), 1};
        }
    }

    //change turn
    state->turn = 1 - state->turn;
    return {getRewards(WLG::GameResult::NOT_OVER), 1};
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();
    return state;
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