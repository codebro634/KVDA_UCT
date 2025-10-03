
#include "../../../include/Games/TwoPlayerGames/KillTheKing.h"
#include <iostream>
#include <fstream>
#include <cassert>

using namespace std;
using namespace KTK;

Model::Model(bool zero_sum, const std::string& map_path) : WLG::Model(zero_sum) {

    //init map
    std::ifstream file(map_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file " << map_path << std::endl;
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
                map.back().push_back(false );
            else if(ch == '.')
                map.back().push_back(true);
            else if(ch == ' ')
                continue;
            else{
                std::cerr << "Error: Invalid character in map file " << map_path << std::endl;
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

    //init units
    //first line contains number of all unit assignments
    std::getline(file, line);
    int num_units = std::stoi(line);
    int current_unit = 0;
    std::getline(file, line);

    std::vector<std::vector<int>> init_unit_data_instance = {};
    while (std::getline(file, line)) {
        if(line.empty()) {
            current_unit++;
            init_unit_data.push_back(init_unit_data_instance);
            init_unit_data_instance.clear();
            if (current_unit == num_units)
                break;
            continue;
        }
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
        int player = std::stoi(tokens[3]);
        int type = std::stoi(tokens[2]);
        init_unit_data_instance.push_back({x,y,player,type});
    }
    file.close();
}

size_t Gamestate::hash() const{
    size_t pos_hash = 0;
    size_t unit_attr_mix = 0;

    for (auto &unit : pos_to_unit) {
        unit_attr_mix ^= unit.second->x ^ unit.second->y ^ unit.second->player ^ unit.second->health ^ unit.second->ability_id ^ unit.second->ability_range ^ unit.second->ability_amount ^ unit.second->king;
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

Unit::Unit(int x, int y, int health, int player, int ability_id, int ability_range, int ability_amount, bool king){
    this->x = x;
    this->y = y;
    this->health = health;
    this->player = player;
    this->ability_id = ability_id;
    this->ability_range = ability_range;
    this->ability_amount = ability_amount;
    this->king = king;
}

static Unit make_warr(int x, int y, int player){
    return Unit(x,y,100,player,0,1,70,false);
}

static Unit make_archer(int x, int y, int player){
    return Unit(x,y,100,player,0,2,50,false);
}

static Unit make_king(int x, int y, int player){
    return Unit(x,y,200,player,0,1,50,true);
}

static Unit make_heal(int x, int y, int player){
    return Unit(x,y,50,player,1,2,50,false);
}

ABS::Gamestate* Model::getInitialState(int num) {
    auto state = new Gamestate();

    for(auto unit_data : init_unit_data[num]) {
        Unit unit = Unit(0,0,0,0,0,0,0,false); //default init
        if(unit_data[3] == 0) {
            unit = make_warr(unit_data[0], unit_data[1], unit_data[2]);
        } else if(unit_data[3] == 1) {
            unit = make_heal(unit_data[0], unit_data[1], unit_data[2]);
        } else if(unit_data[3] == 2) {
            unit = make_archer(unit_data[0], unit_data[1], unit_data[2]);
        }
        else if(unit_data[3] == 3) {
            unit = make_king(unit_data[0], unit_data[1], unit_data[2]);
        }
        else {
            std::cerr << "Invalid unit type: " << unit_data[3] << std::endl;
            exit(1);
        }
        if(unit_data[2] == 0)
            state->p0_units.push_back(unit);
        else
            state->p1_units.push_back(unit);
    }

    for(auto &unit : state->p0_units) {
        state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    for(auto &unit : state->p1_units){
        state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }

    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    return getInitialState(std::uniform_int_distribution<int>(0,init_unit_data.size()-1)(rng));
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state;
    for(auto &unit : new_state->p0_units) {
        if(unit.health > 0)
            new_state->pos_to_unit[std::make_pair(unit.x,unit.y)] = &unit;
    }
    for(auto &unit : new_state->p1_units){
        if(unit.health > 0)
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
                cout << (map[i][j]? "_": "X") << " ";
            }
        }
        cout << endl;
    }

    //Print units states
    cout << "Player 0 active units:" << endl;
    for (auto& unit : state->p0_units){
        if(unit.health > 0)
        cout << "x: " << unit.x << " y: " << unit.y << " health: " << unit.health << " ability_id: " << unit
        .ability_id << " ability_range: " << unit.ability_range << " ability_amount: " << unit.ability_amount << endl;
    }
    cout << "Player 1 active units:" << endl;
    for (auto& unit : state->p1_units){
        if(unit.health > 0)
        cout << "x: " << unit.x << " y: " << unit.y << " health: " << unit.health << " ability_id: " << unit
        .ability_id << " ability_range: " << unit.ability_range << " ability_amount: " << unit.ability_amount << endl;
    }
}

std::pair<int,int> KTK::hashToDir(int hash){
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
    throw std::runtime_error("Invalid hash");
}

int KTK::dirToHash(std::pair<int,int> dir)
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
    vector<std::pair<int,int>> targetable_units_pos = {}; //may include healing and attacking over an obstacle
    if(cur_unit->ability_id == 0){ //attack
        for(auto & target : state->turn==0 ? state->p1_units : state->p0_units){
            if(target.health > 0 && abs(target.x - cur_unit->x) <= cur_unit->ability_range+1 && abs(target.y - cur_unit->y) <= cur_unit->ability_range+1)
                targetable_units_pos.emplace_back(target.x,target.y);
        }
    }
    else if(cur_unit->ability_id == 1){ //heal
        for(auto & target : state->turn==0 ? state->p0_units : state->p1_units){
            if(target.health> 0 && abs(target.x - cur_unit->x) <= cur_unit->ability_range+1 && abs(target.y - cur_unit->y) <= cur_unit->ability_range+1)
                targetable_units_pos.emplace_back(target.x,target.y);
        }
    }else{
        std::cerr << "Invalid ability id: " << cur_unit->ability_id << std::endl;
        exit(1);
    }

    //calculate actions
    for(int i = 0; i < NUM_MOVES; i++){
        auto dir = hashToDir(i);
        int new_x = cur_unit->x + dir.first;
        int new_y = cur_unit->y + dir.second;
        if ((dir.first==0 && dir.second==0) || (new_x >= 0 && new_x < MAP_WIDTH && new_y >= 0 && new_y < MAP_HEIGHT && map[new_x][new_y] && state->pos_to_unit.find(std::make_pair(new_x,new_y)) == state->pos_to_unit.end())) {
            //now find targetable units with special attack
            for(auto target_pos : targetable_units_pos){
                //handle special case of self
                if(target_pos.first == cur_unit->x && target_pos.second == cur_unit->y){
                    int special_action_hash = MAP_HEIGHT * new_x + new_y;
                    actions.push_back(NUM_MOVES * special_action_hash + i);
                }
                else{
                    if(abs(target_pos.first - new_x) <= cur_unit->ability_range && abs(target_pos.second - new_y) <= cur_unit->ability_range){
                        int special_action_hash = MAP_HEIGHT * target_pos.first + target_pos.second;
                        actions.push_back(NUM_MOVES * special_action_hash + i);
                    }
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
    //std::cout << "Moving unit " << cur_unit->x << " " << cur_unit->y << " to " << cur_unit->x + move_dir.first << " " << cur_unit->y + move_dir.second << std::endl;
    int new_x = cur_unit->x + move_dir.first;
    int new_y = cur_unit->y + move_dir.second;
    if(move_dir.first!=0 || move_dir.second!=0){
        state->pos_to_unit.erase(std::make_pair(cur_unit->x,cur_unit->y));
        cur_unit->x = new_x;
        cur_unit->y = new_y;
        state->pos_to_unit[std::make_pair(new_x,new_y)] = cur_unit;
    }

    //special action
    int special_action = action / NUM_MOVES;
    if(special_action != MAP_HEIGHT * MAP_WIDTH){
        int tar_x = special_action / MAP_HEIGHT;
        int tar_y = special_action % MAP_HEIGHT;
        Unit* target = state->pos_to_unit[std::make_pair(tar_x,tar_y)];

        target->health += (cur_unit->ability_id == 0? -1 : 1) * cur_unit->ability_amount;
        if(target->health <= 0){
            state->pos_to_unit.erase(std::make_pair(tar_x,tar_y));
            if(target->king){
                //Check for win condition
                state->terminal = true;
                if(state->turn == 0){
                    state->terminal = true;
                    return {getRewards(WLG::GameResult::P0_WIN),1};
                }
                if(state->turn == 1){
                    state->terminal = true;
                    return {getRewards(WLG::GameResult::P1_WIN),1};
                }
            }
        }
    }

    //Change turn and next unit
    Unit* unit = nullptr;
    while(unit == nullptr || unit->health <= 0){
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

    return {getRewards(WLG::GameResult::NOT_OVER), 1};
}