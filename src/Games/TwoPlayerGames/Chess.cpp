#include "../../../include/Games/TwoPlayerGames/Chess.h"
#include <iostream>
#include <cassert>
using namespace std;

using namespace CHE;

std::vector<int> Model::obsShape() const {
    return {12,8,8};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int piece = state->board[i][j][0];
            if (state->board[i][j][1] == state->turn){
                obs[i*8+j] = piece == PAWN;
                obs[i*8+j+64] = piece == KNIGHT;
                obs[i*8+j+128] = piece == BISHOP;
                obs[i*8+j+3*64] = piece == ROOK;
                obs[i*8+j+4*64] = piece == QUEEN;
                obs[i*8+j+5*64] = piece == KING;
            }else{
                obs[i*8+j+6*64] = piece == PAWN;
                obs[i*8+j+7*64] = piece == KNIGHT;
                obs[i*8+j+8*64] = piece == BISHOP;
                obs[i*8+j+9*64] = piece == ROOK;
                obs[i*8+j+10*64] = piece == QUEEN;
                obs[i*8+j+11*64] = piece == KING;
            }
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {64,64};
}

int Model::encodeAction(int* decoded_action) {
    return 64 * decoded_action[0] + decoded_action[1];
}

Model::Model(bool zero_sum) : WLG::Model(zero_sum) {}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return other_casted->board == board && turn == other_casted->turn && terminal == other_casted->terminal;
}

size_t Gamestate::hash() const{
    size_t hash = 0;
    for (const auto & i : board){
        for (int j = 0; j < 8; j+=2)
            hash = (hash << 1) | (i[j][0] == EMPTY? 0 : 1);
    }
    return hash;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();
    state->board = vector<vector<vector<int>>>(8,vector<vector<int>>(8,vector<int>(2,EMPTY)));
    state->occupied = {};

    //initial figure placement for Player 0
    state->board[0][0][0]=ROOK;
    state->board[0][1][0]=KNIGHT;
    state->board[0][2][0]=BISHOP;
    state->board[0][3][0]=QUEEN;
    state->board[0][4][0]=KING;
    state->board[0][5][0]=BISHOP;
    state->board[0][6][0]=KNIGHT;
    state->board[0][7][0]=ROOK;
    for (int i = 0; i < 8; i++){
        state->board[1][i][0]=PAWN;
        state->board[1][i][1] = 0;
        state->board[0][i][1] = 0;
        state->occupied.insert({1,i});
        state->occupied.insert({0,i});
    }
    //initial figure placement for Player 1
    state->board[7][0][0]=ROOK;
    state->board[7][1][0]=KNIGHT;
    state->board[7][2][0]=BISHOP;
    state->board[7][3][0]=QUEEN;
    state->board[7][4][0]=KING;
    state->board[7][5][0]=BISHOP;
    state->board[7][6][0]=KNIGHT;
    state->board[7][7][0]=ROOK;
    for (int i = 0; i < 8; i++){
        state->board[6][i][0]=PAWN;
        state->board[6][i][1] = 1;
        state->board[7][i][1] = 1;
        state->occupied.insert({6,i});
        state->occupied.insert({7,i});
    }
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
            if(j[0]==EMPTY)
                cout << "__" << " ";
            else
                cout << (j[1] == 0? "W":"B") << j[0] << " ";
        }
        cout << endl;
    }
}

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<std::pair<std::pair<int,int>,std::pair<int,int>>> actions;
    for(auto pos: state->occupied){
        auto piece = state->board[pos.first][pos.second];
        if(piece[1] != state->turn)
            continue;
        if(piece[0] == PAWN){
            //regular move
            int dir = piece[1] == 0? 1:-1;
            if(pos.first + dir < 8 && pos.first + dir >= 0 && state->board[pos.first + dir][pos.second][0] == EMPTY)
                actions.push_back({pos,{pos.first + dir,pos.second}});
            //start move
            if(( (pos.first == 1 && piece[1] == 0) || (pos.first == 6 && piece[1] == 1)) && state->board[pos.first + 2*dir][pos.second][0] == EMPTY)
                actions.push_back({pos,{pos.first + 2*dir,pos.second}});
            //attack move
            if(pos.second + 1 < 8 && state->board[pos.first + dir][pos.second + 1][0] != EMPTY && state->board[pos.first + dir][pos.second + 1][1] != piece[1])
                actions.push_back({pos,{pos.first + dir,pos.second + 1}});
            if(pos.second - 1 >= 0  && state->board[pos.first + dir][pos.second - 1][0] != EMPTY && state->board[pos.first + dir][pos.second - 1][1] != piece[1])
                actions.push_back({pos,{pos.first + dir,pos.second - 1}});

        }else if(piece[0] == KNIGHT){
            for(int i = -2; i <= 2; i++){
                for(int j = -2; j <= 2; j++){
                    if(i == 0 || j == 0 || i == j || i == -j)
                        continue;
                    if(pos.first + i < 8 && pos.first + i >= 0 && pos.second + j < 8 && pos.second + j >= 0 && (state->board[pos.first + i][pos.second + j][1] != piece[1] || state->board[pos.first + i][pos.second + j][0] == EMPTY)){
                        actions.push_back({pos,{pos.first + i,pos.second + j}});
                    }
                }
            }
        } else if(piece[0] == ROOK){
            auto dirs = vector<pair<int,int>>{{1,0},{0,1},{-1,0},{0,-1}};
            for(auto dir : dirs)
            {
               for(int i = 1; i < 8; i++) {
                    if(pos.first + i*dir.first < 8 && pos.first + i*dir.first >= 0 && pos.second + i*dir.second >= 0 && pos.second + i*dir.second < 8 && (state->board[pos.first + i*dir.first][pos.second + i*dir.second][0] == EMPTY  || state->board[pos.first + i*dir.first][pos.second + i*dir.second][1] != piece[1])){
                        actions.push_back({pos,{pos.first + i*dir.first,pos.second + i*dir.second}});
                        if(state->board[pos.first + i*dir.first][pos.second + i*dir.second][0] != EMPTY)
                            break;
                    }else
                        break;
                }
            }
        } else if(piece[0] == BISHOP){
            auto dirs = vector<pair<int,int>>{{1,1},{1,-1},{-1,1},{-1,-1}};
            for(auto dir : dirs){
                for(int i = 1; i < 8; i++){
                    if(pos.first + i*dir.first < 8 && pos.first + i*dir.first >= 0 && pos.second + i*dir.second >= 0 && pos.second + i*dir.second < 8 && (state->board[pos.first + i*dir.first][pos.second + i*dir.second][0] == EMPTY || state->board[pos.first + i*dir.first][pos.second + i*dir.second][1] != piece[1])){
                        actions.push_back({pos,{pos.first + i*dir.first,pos.second + i*dir.second}});
                        if(state->board[pos.first + i*dir.first][pos.second + i*dir.second][0] != EMPTY)
                            break;
                    }else
                        break;
                }
            }
        } else if(piece[0] == QUEEN){
            auto dirs = vector<pair<int,int>>{{1,1},{1,-1},{-1,1},{-1,-1},{1,0},{0,1},{-1,0},{0,-1}};
            for(auto dir : dirs){
                for(int i = 1; i < 8; i++){
                    if(pos.first + i*dir.first < 8 && pos.first + i*dir.first >= 0 && pos.second + i*dir.second >= 0 && pos.second + i*dir.second < 8 && (state->board[pos.first + i*dir.first][pos.second + i*dir.second][0] == EMPTY || state->board[pos.first + i*dir.first][pos.second + i*dir.second][1] != piece[1])){
                        actions.push_back({pos,{pos.first + i*dir.first,pos.second + i*dir.second}});
                        if(state->board[pos.first + i*dir.first][pos.second + i*dir.second][0] != EMPTY)
                            break;
                    }else
                        break;
                }
            }
        } else if(piece[0] == KING){
            auto dirs = vector<pair<int,int>>{{1,1},{1,-1},{-1,1},{-1,-1},{1,0},{0,1},{-1,0},{0,-1}};
            for(auto dir : dirs){
                if(pos.first + dir.first < 8 && pos.first + dir.first >= 0 && pos.second + dir.second >= 0 && pos.second + dir.second < 8 && (state->board[pos.first + dir.first][pos.second + dir.second][0] == EMPTY || state->board[pos.first + dir.first][pos.second + dir.second][1] != piece[1])){
                    actions.push_back({pos,{pos.first + dir.first,pos.second + dir.second}});
                }
            }
        }
    }

    std::vector<int> encoded_actions = {};
    for(auto action: actions){
        int idx1 = action.second.first*8 + action.second.second;
        int idx2 = action.first.first*8 + action.first.second;
        encoded_actions.push_back(idx1*64 + idx2);
    }

    return encoded_actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    std::pair<int,int> pos1 = {action % 64 / 8, action % 64 % 8};
    std::pair<int,int> pos2 = {action / 64 / 8, action / 64 % 8};

    if(state->board[pos2.first][pos2.second][0] == KING){
        state->terminal = true;
        if (state->turn == 0)
            return {getRewards(WLG::GameResult::P0_WIN),1};
        else
            return {getRewards(WLG::GameResult::P1_WIN),1};
    }else{
        state->occupied.erase(pos1);
        state->occupied.insert(pos2);
        state->board[pos2.first][pos2.second][0] = state->board[pos1.first][pos1.second][0];
        state->board[pos2.first][pos2.second][1] = state->turn;
        state->board[pos1.first][pos1.second][0] = EMPTY;
        if(state->board[pos2.first][pos2.second][0] == PAWN && (pos2.first == 0 || pos2.first == 7))
            state->board[pos2.first][pos2.second][0] = QUEEN;
        state->turn = 1 - state->turn;
    }

    return {getRewards(WLG::GameResult::NOT_OVER),1};

}