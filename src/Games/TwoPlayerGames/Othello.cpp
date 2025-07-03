#include "../../../include/Games/TwoPlayerGames/Othello.h"
#include <iostream>
#include <cassert>
using namespace std;

using namespace OTH;

std::vector<int> Model::obsShape() const {
    return {2, ROWS, COLS};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            obs[i * COLS + j] = state->board[i][j] == state->turn;
            obs[i * COLS + j + ROWS * COLS] = state->board[i][j] == 1 - state->turn;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {ROWS,COLS};
}

int Model::encodeAction(int* decoded_action) {
    int row = decoded_action[0];
    int col = decoded_action[1];
    return row * COLS + col;
}

std::vector<double> Model::heuristicsValue(ABS::Gamestate* uncasted_state){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    //Weighted sum that puts most weight on the corner stones
    double p1_value = 0;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            int parity = state->board[i][j] == 1? 1 : -1;
            double stone_weight = 1/(1.0+(std::min(i,ROWS-1-i) + std::min(j,COLS-1-j))) * 10;
            if (state->board[i][j] != EMPTY)
                p1_value += parity * stone_weight;
        }
    }

    if (state->terminal)
        p1_value += state->turn == 1? 100 : -100;

    return {p1_value,-p1_value};
}

Model::Model(bool zero_sum) : WLG::Model(zero_sum) {}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return other_casted->board == board && turn == other_casted->turn && terminal == other_casted->terminal;
}

size_t Gamestate::hash() const{
    int num1=0, num2=0;
    for (auto & i : board) {
        for (int j : i) {
            num1+= j == 0? 1 : 0;
            num2+= j == 1? 1 : 0;
        }
    }
    return num1 | (num2 << 16);
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();

    //place inner square of stones
    state->board[ROWS/2][COLS/2]=1;
    state->board[ROWS/2-1][COLS/2-1]=1;
    state->board[ROWS/2][COLS/2-1]=0;
    state->board[ROWS/2-1][COLS/2]=0;
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
    for (auto & i : state->board) {
        for (int j : i) {
            if(j==EMPTY)
                cout << "_" << " ";
            else
                cout << j << " ";
        }
        cout << endl;
    }
}

std::set<std::pair<int,int>> Model::actionToLegalDirs(const Gamestate* state, int row, int col){
    std::vector<std::pair<int,int>> directions = {{1,0},{0,1},{1,1},{1,-1},{-1,0},{0,-1},{-1,-1},{-1,1}};
    std::set<std::pair<int,int>> legal_dirs;
    for(auto direction : directions){
        int dx = direction.first;
        int dy = direction.second;
        int x = row + dx;
        int y = col + dy;
        if (x < 0 || x >= ROWS || y < 0 || y >= COLS || state->board[x][y] == EMPTY || state->board[x][y] == state->turn)
            continue;
        while(true) {
            x += dx;
            y += dy;
            if (x < 0 || x >= ROWS || y < 0 || y >= COLS || state->board[x][y] == EMPTY)
                break;
            if (state->board[x][y] == state->turn) {
                legal_dirs.insert(direction);
                break;
            }
        }
    }
    return legal_dirs;
}

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    if(state->actions_calculated) {
        vector<int> actions;
        for(auto & i : state->legal_action_to_dirs)
            actions.push_back(i.first);
        return actions;
    }

    vector<int> actions;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (state->board[i][j] != EMPTY)
                continue;
            auto legal_dirs = actionToLegalDirs(state, i,j);
            if(!legal_dirs.empty()) {
                actions.push_back(i*COLS+j);
                state->legal_action_to_dirs[i*COLS+j] = legal_dirs;
            }
        }
    }
    state->actions_calculated = true;
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    //place stone and flip
    int init_row = action/COLS;
    int init_col = action%COLS;
    state->board[init_row][init_col] = state->turn;
    auto dirs = state->actions_calculated? state->legal_action_to_dirs[action] : actionToLegalDirs(state, init_row, init_col);
    for(auto dir : dirs) {
        int row = init_row + dir.first;
        int col = init_col + dir.second;
        while(state->board[row][col] != state->turn){
            state->board[row][col] = state->turn;
            row += dir.first;
            col += dir.second;
        }
    }
    state->actions_calculated = false;
    state->legal_action_to_dirs.clear();
    state->turn = 1 - state->turn;

    //check for game over
    auto actions = getActions(state);
    if(actions.empty()){
        state->turn = 1 - state->turn;
        actions = getActions(state);
        if(actions.empty()){
            state->terminal = true;
            int score = 0;
            for (auto & i : state->board) {
                for (int j : i) {
                    if(j==0)
                        score++;
                    else if(j==1)
                        score--;
                }
            }
            if(score > 0){
                 return {getRewards(WLG::GameResult::P0_WIN),1};
            }else{
                if(score < 0)
                    return {getRewards(WLG::GameResult::P1_WIN),1};
                else
                    return {getRewards(WLG::GameResult::DRAW),1};
            }

        }
    }
    return {getRewards(WLG::GameResult::NOT_OVER),1};
}