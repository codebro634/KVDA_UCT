
#include "../../../include/Games/TwoPlayerGames/Constrictor.h"
#include <iostream>
#include <cassert>
#include <queue>
using namespace std;

using namespace CON;

std::vector<int> Model::obsShape() const {
    return {3, arena_size, arena_size};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int n = state->arena.size();
    assert (n == (int)state->arena[0].size());
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            obs[i*n+j] = state->arena[i][j] == BODY;
            obs[n*n + i*n+j] = state->turn == 0? (state->head0.first == i && state->head0.second == j) : (state->head1.first == i && state->head1.second == j);
            obs[2*n*n + i*n+j] = state->turn == 0? (state->head1.first == i && state->head1.second == j) : (state->head0.first == i && state->head0.second == j);
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {4};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    return other_casted->head0 == head0 && other_casted->head1 == head1 && other_casted->turn == turn &&  other_casted->arena == arena && other_casted->terminal == terminal;
}

size_t Gamestate::hash() const{
    size_t hash = head0.first;
    hash = (hash << 4) | (head0.second);
    hash = (hash << 4) | (head1.first);
    hash = (hash << 4) | (head1.second);
    return hash;
}

Model::Model(int arena_size, bool zero_sum) :  WLG::Model(zero_sum), arena_size(arena_size) {}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();

    state->arena = vector<vector<int>>(arena_size, vector<int>(arena_size));
    for (auto & i : state->arena) {
        for (int & j : i) {
            j=EMPTY;
        }
    }
    //place heads
    state->head0 = {0,0};
    state->head1 = {arena_size-1, arena_size-1};
    state->arena[0][0] = BODY;
    state->arena[arena_size-1][arena_size-1] = BODY;

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
    for (int i = 0; i < arena_size; i++) {
        for (int j = 0; j < arena_size; j++) {
            if(state->head1.first == i && state->head1.second==j)
                cout << "1 ";
            else if (state->head0.first == i && state->head0.second==j)
                cout <<"0 ";
            else if(state->arena[i][j]==BODY)
                cout << "X ";
            else
                cout << "_" << " ";
        }
        cout << endl;
    }
}

inline bool Model::isValid(Gamestate* state, int x, int y) const{
    return x >= 0 && x < arena_size && y >= 0 && y < arena_size && state->arena[x][y] == EMPTY;
}

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<int> actions;
    auto current_head = state->turn==0 ? state->head0 : state->head1;
    int dx[4] = {1,0,-1,0};
    int dy[4] = {0,1,0,-1};
    for (int i = 0; i < 4; i++) {
        int x = current_head.first + dx[i];
        int y = current_head.second + dy[i];
        if(isValid(state,x,y)){
            actions.push_back(i);
        }
    }
    return actions;
}

void floodFill(std::vector<std::vector<int>>& distance, std::vector<std::vector<int>>& arena, int x, int y){
    std::queue<std::pair<int, int>> q;

    // Directions: up, down, left, right
    int dx[4] = {-1, 1, 0, 0};
    int dy[4] = {0, 0, -1, 1};

    q.emplace(x, y);
    distance[x][y] = 0;

    while (!q.empty()) {
        auto [x, y] = q.front();
        q.pop();

        for (int dir = 0; dir < 4; ++dir) {
            int nx = x + dx[dir];
            int ny = y + dy[dir];

            // Check bounds
            if (nx >= 0 && nx < (int) distance.size() && ny >= 0 && ny < (int) distance.size()) {
                // Check if tile is empty and not visited
                if (arena[nx][ny] == -1 && distance[nx][ny] == std::numeric_limits<int>::max()) {
                    distance[nx][ny] = distance[x][y] + 1;
                    q.emplace(nx, ny);
                }
            }
        }
    }
}

std::vector<double> Model::heuristicsValue(ABS::Gamestate* uncasted_state){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int max = std::numeric_limits<int>::max();
    std::vector<std::vector<int>> distancep1(arena_size, std::vector<int>(arena_size, max));
    std::vector<std::vector<int>> distancep2(arena_size, std::vector<int>(arena_size, max));
    floodFill(distancep1, state->arena, state->head0.first, state->head0.second);
    floodFill(distancep2, state->arena, state->head1.first, state->head1.second);

    //count number of tiles that p0 can reach before p1
    int count1 = 0, count2 = 0;
    for (int i = 0; i < arena_size; i++) {
        for (int j = 0; j < arena_size; j++) {
            if (distancep1[i][j] < distancep2[i][j])
                count1++;
            else if (distancep1[i][j] > distancep2[i][j])
                count2++;
        }
    }

    std::vector<double> value = {(double)count1, (double)count2};
    if (state->terminal){
        if (state->turn == 1)
            value[0] += arena_size * arena_size;
        else
            value[1] += arena_size * arena_size;
    }

    return value;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    //move head
    auto current_head = state->turn==0 ? state->head0 : state->head1;
    int dx[4] = {1,0,-1,0};
    int dy[4] = {0,1,0,-1};
    int x = current_head.first + dx[action];
    int y = current_head.second + dy[action];
    state->arena[x][y] = BODY;
    if(state->turn==0)
        state->head0 = {x,y};
    else
        state->head1 = {x,y};

    //check for game over
    state->turn = 1 - state->turn;
    if(getActions(state).empty()){
        state->terminal = true;
        if(state->turn == 0)
            return {getRewards(WLG::GameResult::P1_WIN), 1};
        else
            return {getRewards(WLG::GameResult::P0_WIN), 1};
    }

    return {getRewards(WLG::GameResult::NOT_OVER), 1};

}