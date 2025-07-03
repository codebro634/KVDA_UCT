#include "../../../include/Games/TwoPlayerGames/Connect4.h"
#include <iostream>
#include <cassert>
using namespace std;

using namespace C4;

std::vector<int> Model::obsShape() const {
    return {2,ROWS,COLS};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            obs[i*COLS+j] = state->board[i][j] == state->turn;
            obs[i*COLS+j+ROWS*COLS] = state->board[i][j] == 1 - state->turn;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {COLS};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

Model::Model(bool zero_sum) : WLG::Model(zero_sum) {}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return other_casted->board == board && turn == other_casted->turn && terminal == other_casted->terminal;
}

size_t Gamestate::hash() const{
    size_t hash = 0;
    for (int i = ROWS-1; i > ROWS-1 - 32 / COLS; i--){
        for (int j : board[i]){
            hash = (hash << 1) | (j == EMPTY? 0 : 1);
        }
    }
    return hash;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();
    return state;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

std::vector<double> Model::heuristicsValue(ABS::Gamestate* uncasted_state) {

    std::vector<int> twos_count = {0,0};
    std::vector<int> threes_count = {0,0};
    std::vector<int> fours_count = {0,0};

    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int directions[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            int turn = state->board[row][col];
            if (turn == EMPTY)
                continue;
            for (int i = 0; i < 4; i++) {
                int dx = directions[i][0];
                int dy = directions[i][1];
                int count = 1;
                for (int j = 1; j < 4; j++) {
                    int x = col + j*dx;
                    int y = row + j*dy;
                    if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
                        break;
                    if (state->board[y][x] == turn)
                        count++;
                    else
                        break;

                }
                for (int j = 1; j < 4; j++) {
                    int x = col - j*dx;
                    int y = row - j*dy;
                    if (x < 0 || x >= COLS || y < 0 || y >= ROWS) {
                        break;
                    }
                    if (state->board[y][x] == turn) {
                        count++;
                    } else {
                        break;
                    }
                }

                if (count == 2) {
                    twos_count[turn]++;
                } else if (count == 3) {
                    threes_count[turn]++;
                }else if (count == 4) {
                    fours_count[turn]++;
                }
            }

        }
    }

    twos_count[0] /= 2;
    twos_count[1] /= 2;
    threes_count[0] /= 3;
    threes_count[1] /= 3;
    fours_count[0] /= 4;
    fours_count[1] /= 4;

    return {twos_count[0] + 5.0*threes_count[0] + 25.0 * fours_count[0], twos_count[1] + 5.0*threes_count[1] + 25.0 * fours_count[1]};

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

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<int> actions;
    for (int i = 0; i < COLS; i++) {
        if (state->board[0][i] == EMPTY) {
            actions.push_back(i);
        }
    }
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    //place stone
    int placed_row=-1;
    for (int i = ROWS - 1; i >= 0; i--) {
        if (state->board[i][action] == EMPTY) {
            state->board[i][action] = state->turn;
            placed_row=i;
            break;
        }
    }
    assert (placed_row!=-1);
    state->num_stones_on_board++;

    auto rewards = vector<double>(2,0);
    //check for game over
    int directions[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    for (int i = 0; i < 4; i++) {
        int dx = directions[i][0];
        int dy = directions[i][1];
        int count = 1;
        for (int j = 1; j < 4; j++) {
            int x = action + j*dx;
            int y = placed_row + j*dy;
            if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
                break;
            if (state->board[y][x] == state->turn)
                count++;
            else
                break;

        }
        for (int j = 1; j < 4; j++) {
            int x = action - j*dx;
            int y = placed_row - j*dy;
            if (x < 0 || x >= COLS || y < 0 || y >= ROWS) {
                break;
            }
            if (state->board[y][x] == state->turn) {
                count++;
            } else {
                break;
            }
        }
        if (count >= 4) {
            state->terminal = true;
            if (state->turn == 0) {
                return {getRewards(WLG::GameResult::P0_WIN), 1};
            } else {
                return {getRewards(WLG::GameResult::P1_WIN), 1};
            }

        }
    }

    if(!state->terminal && state->num_stones_on_board==ROWS*COLS){
        state->terminal=true;
        return {getRewards(WLG::GameResult::DRAW),1};
    }


    //change turn
    state->turn = 1 - state->turn;
    return {getRewards(WLG::GameResult::NOT_OVER),1};

}