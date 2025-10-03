#include "../../../include/Games/MDPs/JoinFive.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <map>

using namespace J5;

inline void cell_index_to_pos(int index, int& row, int& col) {
    row = index / SIZE;
    col = index % SIZE;
}

void decode_action(int action, int& row_start, int &col_start, int& row_end, int& col_end) {
    int encoded_start_pos = action % (SIZE*SIZE);
    cell_index_to_pos(encoded_start_pos, row_start, col_start);
    int encoded_end_pos = action / (SIZE*SIZE);
    cell_index_to_pos(encoded_end_pos, row_end, col_end);
}

inline int encode_action(int row_start, int col_start, int row_end, int col_end) {
    return (row_start*SIZE + col_start) + (row_end*SIZE + col_end)*SIZE*SIZE;
}

inline int pos_to_cell_index(int row, int col) {
    return row*SIZE + col;
}

std::vector<int> Model::obsShape() const {
    return {8,SIZE,SIZE};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    /*
    Layers:
    1. Crosses
    2. Horizontals
    3. Verticals
    4. Diagonals left-tilted
    5. Diagonals right-tilted
    6. Marking of the currently selected endpoint
    7. Layer to mark whether we are in an intermediate step
    8. Layer to mark legal moves
    */

    int bsize = J5::SIZE * J5::SIZE;
    for (int i = 0; i < J5::SIZE; i++) {
        for (int j = 0; j < J5::SIZE; j++) {
            int pos = pos_to_cell_index(i,j);
            obs[pos] = state->crosses.test(pos);
            obs[bsize + pos] = state->horizontals.test(pos);
            obs[2*bsize + pos] = state->verticals.test(pos);
            obs[3*bsize + pos] = state->diagonals_left.test(pos);
            obs[4*bsize + pos] = state->diagonals_right.test(pos);
            obs[5*bsize + pos] = state->first_stone_idx == pos;
            obs[6*bsize + pos] = state->first_stone_idx == -1;
            obs[7*bsize + pos] = 0;
        }
    }

    for (int legal : getActions(state)) {
        if (!decoupled_action_space) {
            int row_start, col_start, row_end, col_end;
            decode_action(legal, row_start, col_start, row_end, col_end);
            int pos1 = pos_to_cell_index(row_start, col_start);
            int pos2 = pos_to_cell_index(row_end, col_end);
            obs[7*bsize + pos1] = 1;
            obs[7*bsize + pos2] = 1;
        }else
            obs[7*bsize + legal] = 1;
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    if (decoupled_action_space)
        return {SIZE*SIZE};
    else
        return {SIZE*SIZE,SIZE*SIZE};
}

int Model::encodeAction(int* decoded_action) {
    if (decoupled_action_space)
        return decoded_action[0];
    else {
        int row_start, col_start, row_end, col_end;
        cell_index_to_pos(decoded_action[0], row_start, col_start);
        cell_index_to_pos(decoded_action[1], row_end, col_end);
        return encode_action(row_start,col_start,row_end,col_end);
    }
}

inline std::bitset<SIZE*SIZE>* get_line_set(Gamestate* state, int dir_code) {
    switch (dir_code) {
        case 0: return &state->horizontals;
        case 1: return &state->verticals;
        case 2: return &state->diagonals_left;
        case 3: return &state->diagonals_right;
        default: assert(false);
    }
    return nullptr;
}

inline void increment_line_num(Gamestate* state, int dir_code) {
    switch (dir_code) {
        case 0: state->num_horizontals++; break;
        case 1: state->num_verticals++; break;
        case 2: state->num_diagonals_left++; break;
        case 3: state->num_diagonals_right++; break;
        default: assert(false);
    }
}

inline std::unordered_map<int,std::vector<int>>* line_to_actions(Gamestate* state, int dir_code) {
    switch (dir_code) {
        case 0: return &state->horizontals_to_actions;
        case 1: return &state->verticals_to_actions;
        case 2: return &state->diagonals_left_to_actions;
        case 3: return &state->diagonals_right_to_actions;
        default: assert(false);
    }
    return nullptr;
}

inline bool Model::out_of_bounds(int row, int col){
    return row < 0 || row >= SIZE || col < 0 || col >= SIZE;
}

Model::Model(bool joint, bool exit_on_out_of_bounds, bool decoupled_action_space) {
    this->joint = joint;
    this->exit_on_out_of_bounds = exit_on_out_of_bounds;
    this->decoupled_action_space = decoupled_action_space;

    //calculate the 512 entries of the action lookup table
    for (int i = 0; i < 512; i++) {
        std::bitset<9> crosses(i);
        std::vector<std::tuple<int, int, int> > line_actions;
        int hole_pos = -1;
        for (int start = 0; start <= 4; start++) {
            //check for each possible start/end point combination
            int end = start + 4 ;
            int num_holes = 0;
            for (int pos = start; pos <= end; pos ++) {
                if (!crosses[pos]) {
                    hole_pos = pos;
                    num_holes++;
                }
            }
            if (num_holes == 1)
                line_actions.emplace_back(start, end, hole_pos);
        }
        actions_lookup_table.push_back(line_actions);
    }

    //Action codes and index offset
    action_to_dir_code_and_offset = {
        {{1, 0}, {0, -1, 0}}, //horizontal
        {{-1, 0}, {0, 0, 0}},

        {{0, 1}, {1, 0, -1}}, //vertical
        {{0, -1}, {1, 0, 0}},

        {{1, 1}, {2, 0, 0}}, //diagonal left
        {{-1, -1}, {2, -1, -1}},

        {{1, -1}, {3, -1, 0}}, //diagonal right
        {{-1, 1}, {3, 0, -1}},
    };
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{

    //For speedup make a size check first
    if (first_stone_idx != dynamic_cast<const Gamestate*>(&other)->first_stone_idx ||
        num_verticals != dynamic_cast<const Gamestate*>(&other)->num_verticals ||
        num_horizontals != dynamic_cast<const Gamestate*>(&other)->num_horizontals ||
        num_diagonals_left != dynamic_cast<const Gamestate*>(&other)->num_diagonals_left ||
        num_diagonals_right != dynamic_cast<const Gamestate*>(&other)->num_diagonals_right)
        return false;

    //If size test failed, do heavy but exact comparison
    return crosses ==  dynamic_cast<const Gamestate*>(&other)->crosses &&
    horizontals == dynamic_cast<const Gamestate*>(&other)->horizontals &&
    verticals == dynamic_cast<const Gamestate*>(&other)->verticals &&
    diagonals_left == dynamic_cast<const Gamestate*>(&other)->diagonals_left &&
    diagonals_right == dynamic_cast<const Gamestate*>(&other)->diagonals_right;
}

size_t Gamestate::hash() const
{
    size_t crosses_hash = hash_crosses(crosses);
    size_t horizontals_hash = hash_horizontals(horizontals);
    size_t verticals_hash = hash_verticals(verticals);
    size_t diagonals_left_hash = hash_diagonals_left(diagonals_left);
    size_t action_hash = 0;
    for (int action : avail_actions)
        action_hash = (action_hash << 2) ^ action;

    return crosses_hash ^ horizontals_hash ^ verticals_hash ^ diagonals_left_hash ^ action_hash ^ first_stone_idx;
}

std::vector<double> Model::heuristicsValue(ABS::Gamestate* uncasted_state) {
    return {(double)dynamic_cast<Gamestate*>(uncasted_state)->avail_actions.size()};
}

void Model::printState(ABS::Gamestate* uncasted_state) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            std::cout << (state->crosses.test(i*SIZE + j) ? "X " : ". ");
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

ABS::Gamestate* Model::getInitialState(int num) {
        std::pair<int,int> offset = {SIZE/2 - 4, SIZE/2 - 4};
        std::vector<std::pair<int,int>> initial_crosses;

        for (int i = 0; i < 4; i++) {
            //vertical lines
            initial_crosses.emplace_back(3+i, 0);
            initial_crosses.emplace_back(3+i, 9);
            initial_crosses.emplace_back(i, 3);
            initial_crosses.emplace_back(i, 6);
            initial_crosses.emplace_back(6+i, 3);
            initial_crosses.emplace_back(6+i, 6);

            //horizontal lines
            initial_crosses.emplace_back(0, 3+i);
            initial_crosses.emplace_back(9, 3+i);
            initial_crosses.emplace_back(3, i);
            initial_crosses.emplace_back(6, i);
            initial_crosses.emplace_back(3, 6+i);
            initial_crosses.emplace_back(6, 6+i);
        }

        //Add offset to initial crosses
        for (auto& pos : initial_crosses) {
            pos.first += offset.first;
            pos.second += offset.second;
        }

        //Set crosses
        auto state = new Gamestate();

        for (auto& pos : initial_crosses)
            state->crosses.set(pos_to_cell_index(pos.first,pos.second), true);

        //Brute force determine actions
        auto dirs = std::vector<std::pair<int,int>>{{1,0},{0,1},{1,1},{1,-1}};
        for (int i = 0; i < SIZE; i++) {
            for (int j = 0; j < SIZE; j++) {

                std::vector<int> holes = {0,0,0,0};
                auto potential_line_indices = std::vector(dirs.size(), std::vector<int>());
                std::vector<int> hole_positions = {0,0,0,0};

                for (int k = 0; k < 5; k++) {
                    for (size_t h = 0; h<holes.size(); h++) {
                        if (holes[h] > 1)
                            continue;
                        int new_row = i + k*dirs[h].first;
                        int new_col = j + k*dirs[h].second;
                        potential_line_indices[h].push_back(line_segment_to_index(new_row - dirs[h].first, new_col - dirs[h].second, new_row, new_col));
                        if (out_of_bounds(new_row,new_col))
                            holes[h] = 2;
                        else if (!state->crosses.test(pos_to_cell_index(new_row,new_col))) {
                            holes[h]++;
                            hole_positions[h] = pos_to_cell_index(new_row,new_col);
                        }
                    }
                }
                for (size_t h = 0; h<holes.size(); h++) {
                    if (holes[h] == 1) {
                        int end_row = i + 4*dirs[h].first;
                        int end_col = j + 4*dirs[h].second;
                        int action_code = encode_action(i,j,end_row,end_col);
                        state->avail_actions.insert( action_code);
                        state->holes_to_actions[hole_positions[h]].push_back(action_code);
                        auto [dir_code, row_offset, col_offset] = action_to_dir_code_and_offset[dirs[h]];
                        for (int idx : potential_line_indices[h])
                            (*line_to_actions(state,dir_code))[idx].push_back(action_code);
                    }
                }
            }
        }

        return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng){
    return getInitialState(0);
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

inline int sign(int x) {
    return (x > 0) - (x < 0);
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {

    if (decoupled_action_space){
        Gamestate* state = dynamic_cast<Gamestate*>(uncasted_state);
        if (state->first_stone_idx == -1){
            std::set<int> first_poss = {};
            for (int a : state->avail_actions){
                int r1,c1,r2,c2;
                decode_action(a,r1,c1,r2,c2);
                first_poss.insert(pos_to_cell_index(r1,c1));
                first_poss.insert(pos_to_cell_index(r2,c2));
            }
            std::vector<int> first_poss_vec = {first_poss.begin(), first_poss.end()};
            return first_poss_vec;
        }else{
            std::vector<int> actions;
            for (int a : state->avail_actions){
                int r1,c1,r2,c2;
                decode_action(a,r1,c1,r2,c2);
                if (state->first_stone_idx == pos_to_cell_index(r1,c1))
                    actions.push_back(pos_to_cell_index(r2,c2));
                if (state->first_stone_idx == pos_to_cell_index(r2,c2))
                    actions.push_back(pos_to_cell_index(r1,c1));
            }
            return actions;
        }
    }
    else
        return std::vector<int>(dynamic_cast<Gamestate*>(uncasted_state)->avail_actions.begin(), dynamic_cast<Gamestate*>(uncasted_state)->avail_actions.end());

    /*
     * Can be used for debug to compare if the smart action calculation yields the same results as the trivial brute force variant
     */
    // std::set<int> actions = {};
    // auto state = dynamic_cast<Gamestate*>(uncasted_state);
    // auto dirs = std::vector<std::pair<int,int>>{{1,0},{0,1},{1,1},{1,-1}};
    // for (int i = 0; i < SIZE; i++) {
    //     for (int j = 0; j < SIZE; j++) {
    //
    //         std::vector<int> holes = {0,0,0,0};
    //
    //         for (int k = 0; k < 5; k++) {
    //             for (size_t h = 0; h<holes.size(); h++) {
    //                 if (holes[h] > 1)
    //                     continue;
    //                 int new_row = i + k*dirs[h].first;
    //                 int new_col = j + k*dirs[h].second;
    //                 if (out_of_bounds(new_row,new_col))
    //                     holes[h] = 2;
    //                 else if (!state->crosses.test(pos_to_cell_index(new_row,new_col))) {
    //                     holes[h]++;
    //                 }
    //             }
    //         }
    //         for (size_t h = 0; h<holes.size(); h++) {
    //             if (holes[h] == 1) {
    //                 int end_row = i + 4*dirs[h].first;
    //                 int end_col = j + 4*dirs[h].second;
    //
    //                 std::set<std::pair<int,int>> used_tiles;
    //                 for (int k = 0; k < 5; k++) {
    //                     int new_row = i + k*dirs[h].first;
    //                     int new_col = j + k*dirs[h].second;
    //                     used_tiles.insert({new_row,new_col});
    //                 }
    //                 //test if any lines are retraced
    //                 bool intersection = false;
    //                 for (auto line : state->drawn_lines)
    //                 {
    //                     std::pair<int,int> dir = {sign(line.second.first - line.first.first), sign(line.second.second - line.first.second)};
    //                     if (dir != dirs[h])
    //                         continue;
    //                     std::set<std::pair<int,int>> cmp_used_tiles = {};
    //                     int cmp_start_row = line.first.first;
    //                     int cmp_start_col = line.first.second;
    //                     int cmp_end_row = line.second.first;
    //                     int cmp_end_col = line.second.second;
    //                     //test if lines intersect
    //                     for (int k = 0; k < 5; k++) {
    //                         int new_row = cmp_start_row + k*dir.first;
    //                         int new_col = cmp_start_col + k*dir.second;
    //                         cmp_used_tiles.insert({new_row,new_col});
    //                     }
    //
    //                     std::vector<int> intersection_result;
    //                     for (auto& tile : used_tiles) {
    //                         if (cmp_used_tiles.find(tile) != cmp_used_tiles.end() && ((tile != std::pair{cmp_start_row,cmp_start_col} && tile != std::pair{cmp_end_row,cmp_end_col}) || (tile != std::pair{i,j} && tile != std::pair{end_row,end_col})))
    //                             intersection_result.push_back(1);
    //                     }
    //                     if (!intersection_result.empty()){
    //                         intersection = true;
    //                         break;
    //                     }
    //
    //                 }
    //
    //                 if (!intersection){
    //                     int action_code = encode_action(i,j,end_row,end_col);
    //                     actions.insert(action_code);
    //                 }
    //             }
    //         }
    //     }
    // }
    //
    // std::vector<int> a_vec = {actions.begin(), actions.end()};
    //
    // auto b_vec = std::vector<int>(dynamic_cast<Gamestate*>(uncasted_state)->avail_actions.begin(), dynamic_cast<Gamestate*>(uncasted_state)->avail_actions.end());
    //
    // if (a_vec != b_vec)
    // {
    //     printState(state);
    //     std::cout << "A: ";
    //     for (int a : a_vec)
    //         std::cout << a << " ";
    //     std::cout << std::endl;
    //     std::cout << "B: ";
    //     for (int a : b_vec)
    //         std::cout << a << " ";
    //     std::cout << std::endl;
    //     int a,b,c,d;
    //     decode_action(117012,a,b,c,d);
    //     std::cout << a << " " << b << " " << c << " " << d << std::endl;
    //     exit(1);
    // }
    //
    // return a_vec;
}

inline int Model::line_segment_to_index(int row_from, int col_from, int row_to, int col_to) {
    std::pair<int,int> dir = {sign(row_to - row_from), sign(col_to - col_from)};
    auto [dir_code, row_offset, col_offset] = action_to_dir_code_and_offset[dir];
    return (row_to + row_offset)* SIZE + (col_to + col_offset);
}

void decode_cell(int cell, int &row, int &col) {
    row = cell / SIZE;
    col = cell % SIZE;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);

    if (decoupled_action_space && state->first_stone_idx == -1){
        state->first_stone_idx = action;
        return {{0}, 1};
    }

    int row_end, col_end, col_start, row_start;
    if (decoupled_action_space){
        decode_cell(state->first_stone_idx, row_start, col_start);
        decode_cell(action, row_end, col_end);
        if(!state->avail_actions.contains(encode_action(row_start,col_start,row_end,col_end))){
            int tmp = row_start;
            row_start = row_end;
            row_end = tmp;
            tmp = col_start;
            col_start = col_end;
            col_end = tmp;
        }
        state->first_stone_idx = -1;
    }else
        decode_action(action, row_start, col_start, row_end, col_end);

    //FOR DEBUG
    //state->drawn_lines.push_back({{row_start,col_start},{row_end,col_end}});

    //update state
    int empty_row = -1;
    int empty_col = -1;
    std::pair<int,int> action_dir = std::make_pair(sign(row_end - row_start), sign(col_end - col_start));
    auto [dir_code, row_offset, col_offset] = action_to_dir_code_and_offset[action_dir];
    for (int i = 0; i < (joint? 5 : 6); i++) {

        int new_row = row_start + i*action_dir.first;
        int new_col = col_start + i*action_dir.second;

        //set crosses
        int cell_idx = pos_to_cell_index(new_row,new_col);
        if (i != 5 && !state->crosses.test(cell_idx)) {
            assert (empty_row == -1 && empty_col == -1);
            empty_row = new_row;
            empty_col = new_col;
            for (int a : state->holes_to_actions.at(cell_idx))
                state->avail_actions.erase(a);
            state->holes_to_actions.erase(cell_idx);
            state->crosses.set(cell_idx, true);
        }

        //update tiles

        if ( joint && (i == 0))
            continue;

        int idx = line_segment_to_index(new_row - action_dir.first, new_col - action_dir.second, new_row, new_col);
        if (i != 0 && i != 5) { //in this case, we only want to remove neighboring lines from the action set
            increment_line_num(state, dir_code);
            get_line_set(state, dir_code)->set(idx, true);
        }
        auto l_to_actions = line_to_actions(state, dir_code);
        if ( (i != 0 && i != 5) || l_to_actions->contains(idx)){
            for (int a : l_to_actions->at(idx))
                state->avail_actions.erase(a);
            l_to_actions->erase(idx);
        }
    }

    assert (empty_row != -1 && empty_col != -1);

    //Add new actions
    auto dirs = std::vector<std::pair<int,int>>{{1,0},{0,1},{1,1},{1,-1}};
    for (auto& dir : dirs) {

        std::bitset<9> crosses;
        crosses.set(4, true);
        for (int i = 1; i < 5; i++) {
            int new_row_pos = empty_row + i*dir.first;
            int new_col_pos = empty_col + i*dir.second;
            crosses.set(4+i,out_of_bounds(new_row_pos,new_col_pos)? false : state->crosses.test(pos_to_cell_index(new_row_pos,new_col_pos)));

            int new_row_neg = empty_row - i*dir.first;
            int new_col_neg = empty_col - i*dir.second;
            crosses.set(4-i,out_of_bounds(new_row_neg,new_col_neg)? false : state->crosses.test(pos_to_cell_index(new_row_neg,new_col_neg)));
        }

        auto line_actions = actions_lookup_table[crosses.to_ullong()];

        auto [dir_code, row_offset, col_offset] = action_to_dir_code_and_offset[dir]; //precomputed to satisfy the 1-hole condition

        //Filter for legal actions by testing if they retrace any line portion
        for (auto& line_action : line_actions) {

            //translation from 1D line space to 2D grid space
            auto [start,end, hole_pos] = line_action;
            assert (hole_pos >= start && hole_pos <= end);
            int start_row = empty_row + (start-4) * dir.first;
            int start_col = empty_col + (start-4) * dir.second;
            int end_row = empty_row + (end-4) * dir.first;
            int end_col = empty_col + (end-4) * dir.second;
            int hole_row = empty_row + (hole_pos-4) * dir.first;
            int hole_col = empty_col + (hole_pos-4) * dir.second;

            bool oob_start = out_of_bounds(start_row,start_col);
            bool oob_end = out_of_bounds(end_row,end_col);
            if ( (oob_start || oob_end) && exit_on_out_of_bounds){
                printState(state);
                throw std::runtime_error("Encountered an action that is legal on an infinite board but not legal in this finite setting.");
            }
            if (oob_start || oob_end)
                continue;

            //Retrace check
            int dir_row = sign(end_row - start_row);
            int dir_col = sign(end_col - start_col);
            bool valid_action = true;
            std::vector<int> line_indices = {};
            for (int i = (joint? 1 : 0); i < (joint? 5 : 6); i++) {
                int row_it = start_row + i*dir_row;
                int col_it = start_col + i*dir_col;
                if ((i == 0 || i == 5) && (out_of_bounds(row_it,col_it) || out_of_bounds(row_it - dir_row,col_it - dir_col)))
                    continue;
                int idx = line_segment_to_index(row_it - dir_row, col_it - dir_col, row_it, col_it);
                if (i!= 0 && i != 5)
                    line_indices.push_back(idx);
                if (get_line_set(state, dir_code)->test(idx)) {
                    valid_action = false;
                    break;
                }
            }

            if (valid_action) {
                int action_code = encode_action(start_row, start_col, end_row, end_col);
                state->avail_actions.insert( action_code);
                state->holes_to_actions[pos_to_cell_index(hole_row,hole_col)].push_back(action_code);
                for (int idx : line_indices)
                    (*line_to_actions(state, dir_code))[idx].push_back(action_code);
            }
        }
    }

    //Terminal check
    if (state->avail_actions.empty())
        state->terminal = true;

    return {{1.0}, 1.0};
}
