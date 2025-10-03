#include "../../../include/Games/TwoPlayerGames/Quarto.h"
#include <iostream>
#include <cassert>
using namespace std;

using namespace QUA;


std::tuple<int,int,int,int> intToStone(int stone) {
    int a = stone % 2;
    int b = (stone / 2) % 2;
    int c = (stone / 4) % 2;
    int d = (stone / 8) % 2;
    return {a,b,c,d};
}

std::vector<int> Model::obsShape() const {
    return {4,4,4};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int size = state->board.size() * state->board[0].size();
    for (int j = 0; j < (int)state->board.size(); j++) {
        for (int k = 0; k < (int)state->board[j].size(); k++) {
            obs[0*size + state->board[j].size() * j + k] = std::get<0>(state->board[j][k]);
            obs[1*size + state->board[j].size() * j + k] = std::get<1>(state->board[j][k]);
            obs[2*size + state->board[j].size() * j + k] = std::get<2>(state->board[j][k]);
            obs[3*size + state->board[j].size() * j + k] = std::get<3>(state->board[j][k]);
            obs[3*size + state->board[j].size() * j + k] = state->stone_sel_turn;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {16};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

Model::Model(bool zero_sum) : WLG::Model(zero_sum) {}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return stone_sel_turn == other_casted->stone_sel_turn && other_casted->selected_stone == selected_stone &&
        other_casted->turn == turn && other_casted->avail_stones == avail_stones && other_casted->board == board && other_casted->terminal == terminal;
}

size_t Gamestate::hash() const
{
    size_t hash = std::get<0>(selected_stone) | (std::get<1>(selected_stone) << 1) | (std::get<2>(selected_stone) << 2) | (std::get<3>(selected_stone) << 3);
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

int Model::getNumPlayers() {
    return 2;
}

void Model::printState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for (auto & i : state->board) {
        for (auto j : i) {
            if(j==EMPTY)
                cout << "____" << " ";
            else {
                std::string s1 = std::get<0>(j) ? "T" : "S";
                std::string s2 = std::get<1>(j)? "R" : "B";
                std::string s3 = std::get<2>(j)? "S" : "C";
                std::string s4 = std::get<3>(j)? "H" : "S";
                cout << s1 << s2 << s3 << s4 << " ";
            }
        }
        cout << endl;
    }
}

int stoneToInt(std::tuple<int,int,int,int> stone) {
    return std::get<0>(stone) + std::get<1>(stone)*2 + 4* std::get<2>(stone) + 8*std::get<3>(stone);
}

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<int> actions;
    if(state->stone_sel_turn) { //choose opponents tile
        for(auto stone: state->avail_stones)
            actions.push_back(stoneToInt(stone));
    }else { //placement move
        for (int i = 0; i < 4; i++) {
            for(int j = 0; j < 4; j++) {
                if(state->board[i][j] == EMPTY)
                    actions.push_back(4*i + j);
            }
        }
    }
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){

    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    if(state->stone_sel_turn) { //select opponent stone
        state->selected_stone = intToStone(action);
        state->stone_sel_turn = false;
        state->turn = 1 - state->turn; //only switch turn stone is prompted
    }else {
        state->stone_sel_turn = true;
        //place stone
        int row = action / 4;
        int col = action % 4;
        state->board[row][col] = state->selected_stone;
        state->avail_stones.erase(state->selected_stone);

        //check for game over
        int directions[4][2] = {{1,0},{0,1},{1,1},{1,-1}};

        //Check win condition
        for (int i = 0; i < 4; i++) {
            int dx = directions[i][0];
            int dy = directions[i][1];
            int count[4] = {1,1,1,1};
            for (int j = 1; j < 4; j++) {
                int x = col + j*dx;
                int y = row + j*dy;
                if (x < 0 || x >= 4 || y < 0 || y >= 4)
                    break;
                if (std::get<0>(state->board[y][x]) == std::get<0>(state->board[row][col]))
                    count[0]++;
                if (std::get<1>(state->board[y][x]) == std::get<1>(state->board[row][col]))
                    count[1]++;
                if (std::get<2>(state->board[y][x]) == std::get<2>(state->board[row][col]))
                    count[2]++;
                if (std::get<3>(state->board[y][x]) == std::get<3>(state->board[row][col]))
                    count[3]++;

            }
            for (int j = 1; j < 4; j++) {
                int x = col - j*dx;
                int y = row - j*dy;
                if (x < 0 || x >= 4 || y < 0 || y >= 4) {
                    break;
                }
                if (std::get<0>(state->board[y][x]) == std::get<0>(state->board[row][col]))
                    count[0]++;
                if (std::get<1>(state->board[y][x]) == std::get<1>(state->board[row][col]))
                    count[1]++;
                if (std::get<2>(state->board[y][x]) == std::get<2>(state->board[row][col]))
                    count[2]++;
                if (std::get<3>(state->board[y][x]) == std::get<3>(state->board[row][col]))
                    count[3]++;
            }
            if (count[0] >= 4 || count[1] >= 4 || count[2] >= 4 || count[3] >= 4) {
                state->terminal = true;
                if(state->turn == 0){
                    state->terminal = true;
                    return {getRewards(WLG::GameResult::P0_WIN),1};
                }
                else if(state->turn == 1){
                    state->terminal = true;
                    return {getRewards(WLG::GameResult::P1_WIN),1};
                }
            }
        }

        if(!state->terminal && state->avail_stones.empty()) {
            state->terminal = true;
            return {getRewards(WLG::GameResult::DRAW),1};
        }
    }

    return {getRewards(WLG::GameResult::NOT_OVER),1};

}