
#include "../../../include/Games/TwoPlayerGames/CaptureTheFlag.h"
#include <iostream>
#include <cassert>
#include <fstream>
using namespace std;

using namespace CTF;

Model::Model(bool zero_sum, const std::string &map) : WLG::Model(zero_sum) {
    this->MAP_PATH = map;
}

Unit::Unit(int x, int y, int player){
    this->x = x;
    this->y = y;
    this->health = UHEALTH;
    this->player = player;
    this->spawn_x = x;
    this->spawn_y = y;
    this->has_flag = false;
}

size_t Gamestate::hash() const{
    size_t pos_hash = 0;
    size_t unit_attr_mix = 0;

    for (auto &unit : pos_to_unit) {
        unit_attr_mix ^= unit.second->x ^ unit.second->y ^ unit.second->player ^ unit.second->health ^ unit.second->has_flag ^ unit.second->spawn_x ^ unit.second->spawn_y;
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
        || (p0_flag_pos != other_casted->p0_flag_pos || p1_flag_pos != other_casted->p1_flag_pos)
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
                if(ch == 'W')
                    map.back().push_back(WALL);
                else if(ch == '.')
                    map.back().push_back(EMPTY);
                else if(ch == ' ')
                    continue;
                else if(ch == 'A')
                    map.back().push_back(P0_BASE);
                else if(ch == 'B')
                    map.back().push_back(P1_BASE);
                else{
                    std::cerr << "Error: Invalid character in map file " << MAP_PATH << ": " << ch << std::endl;
                    exit(1);
                }
            }
        }
        //transpose
        std::vector<std::vector<int>> new_map(MAP_WIDTH,std::vector<int>(MAP_HEIGHT,EMPTY));
        for(int i = 0; i < MAP_WIDTH; i++){
            for(int j = 0; j < MAP_HEIGHT; j++){
                new_map[i][j] = map[j][i];
                if(map[j][i] == P0_BASE)
                    state->p0_flag_pos = std::make_pair(i,j);
                if(map[j][i] == P1_BASE)
                    state->p1_flag_pos = std::make_pair(i,j);
            }
        }
        map = new_map;

        //init unit data
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

    for(auto unit_data : init_unit_data) {
        if(unit_data[2] == 0)
            state->p0_units.emplace_back(unit_data[0], unit_data[1], 0);
        else
            state->p1_units.emplace_back(unit_data[0], unit_data[1], 1);
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
        new_state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    for(auto &unit : new_state->p1_units){
        new_state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    return new_state;
}

int Model::getNumPlayers() {
    return 2;
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
                cout << (map[i][j] == EMPTY? "_": (map[i][j] == WALL? "W" : std::to_string(map[i][j]))) << " ";
            }
        }
        cout << endl;
    }

    //Print units states
    cout << "Player 0 active units:" << endl;
    for (auto& unit : state->p0_units){
        cout << "x: " << unit.x << " y: " << unit.y << " health: " << unit.health << " has flag: " << unit.has_flag << endl;
    }
    cout << "Player 1 active units:" << endl;
    for (auto& unit : state->p1_units){
        cout << "x: " << unit.x << " y: " << unit.y << " health: " << unit.health << " has flag: " << unit.has_flag << endl;
    }
    //print flag status
    cout << "Flag status:" << endl;
    cout << "Flag of P0: " << state->p0_flag_pos.first << " " << state->p0_flag_pos.second << endl;
    cout << "Flag of P1: " << state->p1_flag_pos.first << " " << state->p1_flag_pos.second << endl;
}

std::pair<int,int> CTF::hashToDir(int hash){
    if(hash == 0)
        return {0,0};
    else if(hash == 1)
        return {1,0};
    else if(hash == 2)
        return {-1,0};
    else if(hash == 3)
        return {0,1};
    else if(hash == 4)
        return {0,-1};
    else if(hash == 5)
        return {1,1};
    else if(hash == 6)
        return {1,-1};
    else if(hash == 7)
        return {-1,1};
    else if(hash == 8)
        return {-1,-1};
    else if(hash == 9)
        return {2,0};
    else if(hash == 10)
        return {-2,0};
    else if(hash == 11)
        return {0,2};
    else if(hash == 12)
        return {0,-2};
    else
        assert(false);
    throw std::runtime_error("Invalid hash");
}

bool CTF::dirLegal(int x, int y, int dir, std::map<std::pair<int,int>,Unit*>& pos_to_unit) {
    auto new_pos = hashToDir(dir);
    if(new_pos.first == 0 && new_pos.second == 0)
        return true;
    int goal_x = x + new_pos.first;
    int goal_y = y + new_pos.second;
    if(goal_x < 0 || goal_x >= MAP_WIDTH || goal_y < 0 || goal_y >= MAP_HEIGHT || map[goal_x][goal_y] == WALL || pos_to_unit.find(std::make_pair(goal_x,goal_y)) != pos_to_unit.end())
        return false;

    if(abs(new_pos.first) + abs(new_pos.second) <= 1) {
        return true;
    }else { //case dist=2
        std::vector<std::pair<int,int>> intermediate_pos = {};
        if(new_pos.first == 2)
            intermediate_pos = {{1,0}};
        else if(new_pos.first == -2)
            intermediate_pos = {{-1,0}};
        else if(new_pos.second == 2)
            intermediate_pos = {{0,1}};
        else if(new_pos.second == -2)
            intermediate_pos = {{0,-1}};
        else if(new_pos.first == 1 && new_pos.second == 1) {
            intermediate_pos = {{1,0},{0,1}};
        } else if(new_pos.first == 1 && new_pos.second == -1) {
            intermediate_pos = {{1,0},{0,-1}};
        } else if(new_pos.first == -1 && new_pos.second == 1) {
            intermediate_pos = {{-1,0},{0,1}};
        } else if(new_pos.first == -1 && new_pos.second == -1) {
            intermediate_pos = {{-1,0},{0,-1}};
        }
        bool legal = false;
        for(auto inter_pos : intermediate_pos){
            int inter_x = x + inter_pos.first;
            int inter_y = y + inter_pos.second;
            if(! (map[inter_x][inter_y] == WALL || pos_to_unit.find(std::make_pair(inter_x,inter_y)) != pos_to_unit.end())) {
                legal=true;
                break;
            }
        }
        return legal;
    }
}

int CTF::dirToHash(std::pair<int,int> dir)
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
    else if(dir.first==1 && dir.second==1)
        return 5;
    else if(dir.first==1 && dir.second==-1)
        return 6;
    else if(dir.first==-1 && dir.second==1)
        return 7;
    else if(dir.first==-1 && dir.second==-1)
        return 8;
    else if(dir.first == 2 && dir.second == 0)
        return 9;
    else if(dir.first == -2 && dir.second == 0)
        return 10;
    else if(dir.first == 0 && dir.second == 2)
        return 11;
    else if(dir.first == 0 && dir.second == -2)
        return 12;
    else
        assert(false);
    throw std::runtime_error("Invalid direction");
}

vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)
{
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    vector<int> actions = {};

    Unit* cur_unit = state->turn == 0 ? &(state->p0_units[state->next_unit_idx]) : &(state->p1_units[state->next_unit_idx]);

    //ful sweep to see which units are targetable in any direction. This is only efficient if the total unit count is less than ~10
    vector<std::pair<int,int>> targetable_units_pos = {};
    for(auto & target : state->turn==0 ? state->p1_units : state->p0_units){
        if(target.health > 0 && abs(target.x - cur_unit->x) <= 2 && abs(target.y - cur_unit->y) <= 2)
            targetable_units_pos.emplace_back(target.x,target.y);
    }

    //calculate actions
    for(int i = 0; i < NUM_MOVES; i++){
        auto dir = hashToDir(i);
        if(cur_unit->has_flag && abs(dir.first) + abs(dir.second) > 1)
            continue;
        if(dirLegal(cur_unit->x,cur_unit->y,i,state->pos_to_unit)){
            int new_x = cur_unit->x + dir.first;
            int new_y = cur_unit->y + dir.second;
            //now find targetable units with special attack
            for(auto target_pos : targetable_units_pos){
                    if(abs(target_pos.first - new_x) <= 1 && abs(target_pos.second - new_y) <= 1){
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
        //capture the flag from enemy base
        if(state->turn == 0 && state->p1_flag_pos == std::make_pair(new_x,new_y)) {
            cur_unit->has_flag = true;
            state->p1_flag_pos = std::make_pair(-42,-42); //just place it somewhere outside the map
        }
        if(state->turn == 1 && state->p0_flag_pos == std::make_pair(new_x,new_y)) {
            cur_unit->has_flag = true;
            state->p0_flag_pos = std::make_pair(-42,-42); //just place it somewhere outside the map
        }
        //place flag in own base
        if(cur_unit->has_flag){
            if(state->turn == 0 && map[new_x][new_y] == P0_BASE){
                state->terminal = true;
                return {getRewards(WLG::GameResult::P0_WIN),1};
            }
            if(state->turn == 1 && map[new_x][new_y] == P1_BASE){
                state->terminal = true;
                return {getRewards(WLG::GameResult::P1_WIN),1};
            }
        }
    }

    //special action
    int special_action = action / NUM_MOVES;
    if(special_action != MAP_HEIGHT * MAP_WIDTH){
        int tar_x = special_action / MAP_HEIGHT;
        int tar_y = special_action % MAP_HEIGHT;
        Unit* target = state->pos_to_unit[std::make_pair(tar_x,tar_y)];

        target->health -= UATTACK;
        if(target->health <= 0){
            state->pos_to_unit.erase(std::make_pair(tar_x,tar_y));
            if(target->has_flag){
                if(state->turn == 0)
                    state->p0_flag_pos = std::make_pair(tar_x,tar_y);
                else
                    state->p1_flag_pos = std::make_pair(tar_x,tar_y);
            }
            //respawn unit
            int spawn_dist = 0;
            int spawn_x = -1;
            int spawn_y = -1;
            while(spawn_x == -1){
                for(int k = -spawn_dist; k <= spawn_dist; k++) {
                    std::vector<std::pair<int,int>> dirs = {{k,spawn_dist}, {k,-spawn_dist}, {spawn_dist,k}, {-spawn_dist,k}};
                    for(auto dir : dirs) {
                        int test_x = target->spawn_x + dir.first;
                        int test_y = target->spawn_y + dir.second;
                        bool oob = test_x < 0 || test_x >= MAP_WIDTH || test_y < 0 || test_y >= MAP_HEIGHT;
                        bool on_flag = target->player == 0? std::make_pair(test_x,test_y) == state->p1_flag_pos : std::make_pair(test_x,test_y) == state->p0_flag_pos;
                        bool on_unit = state->pos_to_unit.find(std::make_pair(test_x,test_y)) != state->pos_to_unit.end();
                        if (!oob  && !on_flag && !on_unit) {
                            spawn_x = test_x;
                            spawn_y = test_y;
                            break;
                        }
                    }
                    if(spawn_x != -1)
                        break;
                }
                spawn_dist++;
            }
            target->x = spawn_x;
            target->y = spawn_y;
            target->health = UHEALTH;
            state->pos_to_unit[std::make_pair(spawn_x, spawn_y)] = target;
        }
    }

    //Change turn and next unit
    Unit* unit = nullptr;
    while(unit == nullptr){
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