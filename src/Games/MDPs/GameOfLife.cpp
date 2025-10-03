#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/GameOfLife.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;

using namespace GOL;

inline bool getCell(int& board, int x, int y, int width){
    return (board & (1 << (x * width + y))) != 0;
}

std::vector<int> Model::obsShape() const {
    return {1, map_width, map_height};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    for (int i = 0; i < map_width; ++i) {
        for (int j = 0; j < map_height; ++j) {
            obs[i * map_height + j] = getCell(dynamic_cast<GOL::Gamestate*>(uncasted_state)->board, i, j, map_width) ? 1 : 0;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {1+map_height*map_width};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0]-1;
}


[[nodiscard]] std::string Gamestate::toString() const {
    return "((" + std::to_string(board) + ")" + "," + ABS::Gamestate::toString() + ")";
}

ABS::Gamestate* Model::deserialize(std::string &ostring) const {
    auto* state = new Gamestate();
    int board, turn,  terminal;
    sscanf(ostring.c_str(), "((%d),(%d,%d))", &board, &turn, &terminal);
    state->board = board;
    state->turn = turn;
    state->terminal = terminal;
    return state;
}

Model::Model(const std::string& fileName, ActionMode action_space){
    std::ifstream file(fileName);

    if (!file.is_open()){
        std::cerr << "Could not open file " << fileName << std::endl;
        exit(1);
    }

    std::string line;
    std::vector<std::vector<bool>> temp_board;
    std::vector<std::vector<float>> temp_noise_map;

    // Read the first matrix of booleans (0 or 1)
    while (std::getline(file, line) && !(line.empty() || line == "\r")){
        std::vector<bool> row;
        std::istringstream iss(line);
        std::string value;

        while (std::getline(iss, value, ' ')){
            row.push_back(value == "1" || value == "1\r"); // Convert "1" to true, "0" to false
        }
        temp_board.push_back(row);
    }

    // Skip the blank line if not already skipped
    while ((line.empty() || line == "\r") && std::getline(file, line)) { }

    // Read the second matrix of floats (noise map)
    do {
        if (line.empty() || line == "\r") continue;  // Skip any additional blank lines

        std::vector<float> row;
        std::istringstream iss(line);
        std::string value;

        while (std::getline(iss, value, ' ')){
            row.push_back(std::stof(value));  // Convert string to float
        }
        temp_noise_map.push_back(row);
    } while (std::getline(file, line));

    // Assign temp vectors to the member variables
    loaded_map = temp_board;
    map_width = temp_board.size();
    map_height = temp_board[0].size();
    loaded_map_hash = 0;
    for (int i = 0; i < map_width; ++i) {
        for (int j = 0; j <map_height; ++j) {
            loaded_map_hash += (1 << (i * map_height + j)) * temp_board[i][j];
        }
    }
    noise_map = temp_noise_map;
    this->action_mode = action_space;

    assert (map_width * map_height <= 32); // Limit the size of the board to 5x5 as we save it in a 32-bit integer. If one wants to increase this limit, modify == operator
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    return board ==  dynamic_cast<const Gamestate*>(&other)->board;
}

size_t Gamestate::hash() const{
    return board;
}

inline void setCell(int& board, int x, int y, int width, bool value){
    if (value)
        board |= (1 << (x * width + y));
    else
        board &= ~(1 << (x * width + y));
}

void Model::printState(ABS::Gamestate* state) {
    auto* golState = dynamic_cast<GOL::Gamestate*>(state);
    if (!golState) return;

    for (size_t i = 0; i < noise_map.size(); ++i) {
        for (size_t j = 0; j < noise_map[0].size(); ++j) {
            std::cout << (getCell(golState->board,i,j,map_height) ? 'x' : '.') << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "State code: " << golState->board << std::endl;
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return __builtin_popcount( state_a->board ^ state_b->board);
}

ABS::Gamestate* Model::getInitialState(int num) {
        std::vector<int> state_codes = {
            // 197,
            30169386,
            //281 //manually chosen state numbers in which aupo gets neither 100% nor 0% accuraccy and the best contenders can be grouped
        };
        auto state_string = "(("+std::to_string(state_codes[num % state_codes.size()]) + "),(0,0,0))";
        return deserialize(state_string);
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new GOL::Gamestate();

    if(!loaded_map.empty()) {
        state->board = loaded_map_hash;
    }
    else
    {
        // Randomly initialize the board
        state->board = 0;
        std::uniform_int_distribution<int> dist(0, 1);
         for (int i = 0; i < map_width; ++i) {
             for (int j = 0; j < map_height; ++j) {
                 setCell(state->board,i,j,map_width,dist(rng));
             }
         }
    }

    return state;
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    std::vector<int> actions;
    for (size_t i = 0; i < noise_map.size() * noise_map[0].size(); ++i) {
        bool legal_action = (action_mode == ActionMode::SAVE_ONLY && getCell(state->board, i / noise_map[0].size(), i % noise_map[0].size(), map_height)) ||
            (action_mode == ActionMode::REVIVE_ONLY && !getCell(state->board, i / noise_map[0].size(), i % noise_map[0].size(), map_height)) ||
            action_mode == ActionMode::ALL;
        if (legal_action)  // If cell is alive
            actions.push_back(i);
    }
    if(actions.empty())
        actions.push_back(-1); //no-op action
    return actions;
}


//Helper func
int countAliveNeighbors(int& board, int board_size, int x, int y) {
    int count = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            if (i == 0 && j == 0) continue;  // Skip the cell itself
            int nx = x + i;
            int ny = y + j;
            if (nx >= 0 && nx < board_size && ny >= 0 && ny < board_size) {
                count += getCell(board, nx, ny, board_size);
            }
        }
    }
    return count;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
            auto* state = dynamic_cast<GOL::Gamestate*>(uncasted_state);
            size_t decision_point = 0;

            // Map the action (index) to a grid coordinate
            int row = action / noise_map[0].size();
            int col = action % noise_map[0].size();

            if (action == -1)
                row = -1; //makes sure the action has no effect

            // Temporary copy of the board to compute the next state
            int old_board = state->board;

            // Apply standard Game of Life rules with stochastic override
            int num_living_cells = 0;
            double p = 1;
            for (size_t i = 0; i < noise_map.size(); ++i) {
                for (size_t j = 0; j < noise_map[0].size(); ++j) {
                    int alive_neighbors = countAliveNeighbors(old_board, map_height, i, j);

                    // Standard Game of Life rules
                    bool next_alive = getCell(old_board,i,j,map_height);
                    if (next_alive)
                        num_living_cells++;

                    if (getCell(old_board,i,j,map_height)) {
                        if ( (static_cast<int>(i) != row || static_cast<int>(j) != col) && (alive_neighbors < 2 || alive_neighbors > 3)) {
                            next_alive = false;  // Cell dies
                        }
                    } else {
                        if (alive_neighbors == 3 || (static_cast<int>(i) == row && static_cast<int>(j) == col)) {
                            next_alive = true;  // Cell becomes alive
                        }
                    }

                    // Apply stochastic override
                    std::uniform_real_distribution<double> dist(0.0, 1.0);
                    size_t outcome = decision_outcomes == nullptr || noise_map[i][j] == 0 || noise_map[i][j] == 1? (dist(rng) < noise_map[i][j]) : getDecisionPoint(decision_point, 0, 1, decision_outcomes);
                    if (outcome == 1) {
                        next_alive = !next_alive;  // Override the transition
                        p *= noise_map[i][j];
                    }else {
                        p *= 1 - noise_map[i][j];
                    }

                    setCell(state->board,i,j,map_width,next_alive);
                }
            }

            return {{(double)num_living_cells}, p};
        }