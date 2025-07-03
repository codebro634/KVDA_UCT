
#include "../../../include/Games/TwoPlayerGames/Pusher.h"
#include <iostream>
#include <cassert>
#include <fstream>
using namespace std;

using namespace PUS;

Model::Model(bool zero_sum, std::string map) : WLG::Model(zero_sum) {
    this->MAP_PATH = map;
}

Unit::Unit(int x, int y, int player, bool active){
    this->x = x;
    this->y = y;
    this->player = player;
    this->active = active;
}

size_t Gamestate::hash() const{
    size_t pos_hash = 0;
    size_t unit_attr_mix = 0;

    for (auto &unit : pos_to_unit) {
        unit_attr_mix ^= unit.second->x ^ unit.second->y ^ unit.second->player;
        pos_hash = (pos_hash << 2) | (unit.second->x & 1) | ((unit.second->y & 1) << 1);
    }

    return pos_hash + unit_attr_mix;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_casted = dynamic_cast<const Gamestate*>(&other);
    if (other_casted->pos_to_unit.size() != pos_to_unit.size() || turn != other_casted->turn || terminal != other_casted->terminal
        || (turn == 0 && p0_units[next_unit_idx] != other_casted->p0_units[other_casted->next_unit_idx])
        || (turn == 1 && p1_units[next_unit_idx] != other_casted->p1_units[other_casted->next_unit_idx])
    )
        return false;
    for(auto &unit : pos_to_unit){
        if(!other_casted->pos_to_unit.contains(unit.first))
            return false;
        if(*unit.second != *other_casted->pos_to_unit.at(unit.first))
            return false;
    }
    return true;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto state = new Gamestate();

    //init map
    if(map.empty()){
        std::ifstream file(MAP_PATH);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open the file " << MAP_PATH << std::endl;
            exit(1);
        }
        std::string line;
        std::getline(file, line);
        MAP_WIDTH = std::stoi(line);
        std::getline(file, line);
        MAP_HEIGHT = std::stoi(line);

        for (int i = 0; i < MAP_HEIGHT; i++) {
            std::getline(file, line);
            map.emplace_back();
            for(char ch: line) {
                if(ch == 'H')
                    map.back().push_back(true);
                else if(ch == '.')
                    map.back().push_back(false);
                else if(ch == ' ')
                    continue;
                else{
                    std::cerr << "Error: Invalid character in map file " << MAP_PATH << ":" << ch << std::endl;
                    exit(1);
                }
            }
        }

        //transpose
        std::vector<std::vector<bool>> new_map(MAP_WIDTH,std::vector<bool>(MAP_HEIGHT,false));
        for(int i = 0; i < MAP_WIDTH; i++){
            for(int j = 0; j < MAP_HEIGHT; j++){
                new_map[i][j] = map[j][i];
            }
        }
        map = new_map;

        while (std::getline(file, line)) {
            map.emplace_back();
            //split line by ' '
            std::string delimiter = " ";
            size_t pos = 0;
            std::string token;
            std::vector<std::string> tokens;
            while ((pos = line.find(delimiter)) != std::string::npos) {
                token = line.substr(0, pos);
                tokens.push_back(token);
                line.erase(0, pos + delimiter.length());
            }
            tokens.push_back(line);
            int x = std::stoi(tokens[0]);
            int y = std::stoi(tokens[1]);
            int player = std::stoi(tokens[2]);
            init_unit_data.push_back({x,y,player});
        }
        file.close();
    }

    state->p0_active_units = 0;
    state->p1_active_units = 0;
    for(auto unit_data : init_unit_data) {
        if(unit_data[2] == 0) {
            state->p0_units.emplace_back(unit_data[0], unit_data[1], 0, true);
            state->p0_active_units++;
        }else{
            state->p1_units.emplace_back(unit_data[0], unit_data[1], 1, true);
            state->p1_active_units++;
        }
    }

    for(auto &unit : state->p0_units) {
        state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    for(auto &unit : state->p1_units){
        state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }

    return state;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state;
    for(auto &unit : new_state->p0_units) {
        if(unit.active)
            new_state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    for(auto &unit : new_state->p1_units){
        if(unit.active)
            new_state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    return new_state;
}

int Model::getNumPlayers() {
    return 2;
}

std::vector<double> Model::heuristicsValue(ABS::Gamestate* uncasted_state) {

    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    std::vector<double> heuristics(2,0);
    heuristics[0] = state->p0_active_units - state->p1_active_units;
    heuristics[1] = state->p1_active_units - state->p0_active_units;

    //get center of masse for p0
    double p0_x = 0;
    double p0_y = 0;
    double p0_count = 0;
    for(auto& unit : state->p0_units){
        if(unit.active){
            p0_x += unit.x;
            p0_y += unit.y;
            p0_count++;
        }
    }

    //get center of masse for p1
    double p1_x = 0;
    double p1_y = 0;
    double p1_count = 0;
    for(auto& unit : state->p1_units){
        if(unit.active){
            p1_x += unit.x;
            p1_y += unit.y;
            p1_count++;
        }
    }
    if(p1_count > 0){
        p1_x /= p1_count;
        p1_y /= p1_count;
    }

    double normed_avg_distance = (std::fabs(p0_x - p1_x) + std::fabs(p0_y - p1_y)) / (MAP_WIDTH + MAP_HEIGHT);
    heuristics[0] -= normed_avg_distance;
    heuristics[1] -= normed_avg_distance;

    if (state->terminal){
        if (state->p0_active_units == 0)
            heuristics[1] += 100;
        if (state->p1_active_units == 0)
            heuristics[0] += 100;
    }

    return heuristics;

}

void Model::printState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    //Print map occupation
    for (int j = 0; j < MAP_HEIGHT; j++) {
        for (int i = 0; i < MAP_WIDTH; i++) {
            if (state->pos_to_unit.find(std::make_pair(i,j)) != state->pos_to_unit.end()) {
                auto unit = state->pos_to_unit[std::make_pair(i,j)];
                cout << unit->player << " ";
            } else {
                cout << (map[i][j]? "H":"_") << " ";
            }
        }
        cout << endl;
    }

    //Print units states
    cout << "Player 0 active units:" << endl;
    for (auto& unit : state->p0_units){
        if(unit.active)
        cout << "x: " << unit.x << " y: " << unit.y << endl;
    }
    cout << "Player 1 active units:" << endl;
    for (auto& unit : state->p1_units){
        if(unit.active)
        cout << "x: " << unit.x << " y: " << unit.y<< endl;
    }
}

std::pair<int,int> PUS::hashToDir(int hash){
    if(hash==0)
        return {0,0};
    else if(hash==1)
        return {1,0};
    else if(hash==2)
        return {-1,0};
    else if(hash==3)
        return {0,1};
    else if(hash==4)
        return {0,-1};
    else
        assert(false);
    throw std::runtime_error("Invalid direction");
}

int PUS::dirToHash(std::pair<int,int> dir)
{
    if(dir.first==0 && dir.second==0)
        return 0;
    else if(dir.first==1 && dir.second==0)
        return 1;
    else if(dir.first==-1 && dir.second==0)
        return 2;
    else if(dir.first==0 && dir.second==1)
        return 3;
    else if(dir.first==0 && dir.second==-1)
        return 4;

    throw std::runtime_error("Invalid direction");
}

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)
{
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<int> actions = {};

    Unit* cur_unit = state->turn == 0 ? &(state->p0_units[state->next_unit_idx]) : &(state->p1_units[state->next_unit_idx]);

    //ful sweep to see which units are targetable in any direction. This is only efficient if the total unit count is less than ~10
    vector<std::pair<int,int>> targetable_units_pos = {}; //may include healing and attacking over an obstacle
    for(auto & target : state->turn==0 ? state->p1_units : state->p0_units){
        if(target.active && abs(target.x - cur_unit->x) + abs(target.y - cur_unit->y) <= 2)
            targetable_units_pos.emplace_back(target.x,target.y);
    }

    //calculate actions
    for(int i = 0; i < NUM_MOVES; i++){
        auto dir = hashToDir(i);
        int new_x = cur_unit->x + dir.first;
        int new_y = cur_unit->y + dir.second;
        if ((dir.first==0 && dir.second==0)  || (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT && !map[new_x][new_y] && state->pos_to_unit.find(std::make_pair(new_x,new_y)) == state->pos_to_unit.end() )) {
            //now find targetable units with special attack
            for(auto target_pos : targetable_units_pos){
                int dx = target_pos.first - new_x;
                int dy = target_pos.second - new_y;
                if(abs(dx) + abs(dy) <= 1 && state->pos_to_unit.find(std::make_pair(new_x + 2*dx,new_y + 2*dy)) == state->pos_to_unit.end()){
                    int special_action_hash = MAP_HEIGHT * target_pos.first + target_pos.second;
                    actions.push_back(NUM_MOVES * special_action_hash + i);
                }
            }
            actions.push_back(NUM_MOVES * MAP_HEIGHT * MAP_WIDTH + i);
        }
    }

    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {

    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto cur_unit = state->turn == 0 ? &state->p0_units[state->next_unit_idx] : &state->p1_units[state->next_unit_idx];

    //move unit
    auto move_dir = hashToDir(action % NUM_MOVES);
    int new_x = cur_unit->x + move_dir.first;
    int new_y = cur_unit->y + move_dir.second;
    if(move_dir.first!=0 || move_dir.second!=0){
        state->pos_to_unit.erase(std::make_pair(cur_unit->x,cur_unit->y));
        cur_unit->x = new_x;
        cur_unit->y = new_y;
        state->pos_to_unit[std::make_pair(new_x,new_y)] = cur_unit;
    }

    //push action
    int push_action = action / NUM_MOVES;
    if(push_action != MAP_HEIGHT * MAP_WIDTH){
        int tar_x = push_action / MAP_HEIGHT;
        int tar_y = push_action % MAP_HEIGHT;
        Unit* target = state->pos_to_unit[std::make_pair(tar_x,tar_y)];

        int delta_x = tar_x - cur_unit->x;
        int delta_y = tar_y - cur_unit->y;
        state->pos_to_unit.erase(std::make_pair(tar_x,tar_y));
        target->x += delta_x;
        target->y += delta_y;
        if(target->x < 0 || target->x >= MAP_WIDTH || target->y < 0 || target->y >= MAP_HEIGHT || map[target->x][target->y]) {
            state->p1_active_units -= target->player;
            state->p0_active_units -= 1 - target->player;
            target->active = false;
        }else
            state->pos_to_unit[std::make_pair(target->x,target->y)] = target;

        if(state->p1_active_units == 0 || state->p0_active_units == 0){
            state->terminal = true;
            if(state->p1_active_units == 0)
                return {getRewards(WLG::GameResult::P0_WIN),1};
            else
                return {getRewards(WLG::GameResult::P1_WIN),1};
        }
    }

    //Change turn and next unit
    Unit* unit = nullptr;
    while(unit == nullptr || !unit->active){
        state->next_unit_idx++;
        if(state->turn == 0){
            if(state->next_unit_idx == static_cast<int>(state->p0_units.size())){
                state->next_unit_idx = 0;
                state->turn = 1 - state->turn;
            }
        }else{
            if(state->next_unit_idx == static_cast<int>(state->p1_units.size())){
                state->next_unit_idx = 0;
                state->turn = 1 - state->turn;
            }
        }
        unit = state->turn == 0? &state->p0_units[state->next_unit_idx] : &state->p1_units[state->next_unit_idx];
    }

    return {getRewards(WLG::GameResult::NOT_OVER),1};
}