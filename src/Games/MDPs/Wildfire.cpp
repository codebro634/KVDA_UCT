#include "../../../include/Games/MDPs/Wildfire.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>

using namespace std;
using namespace WF;

inline int encode_cell(int x, int y, int width) {
    return x + y*width;
}

std::vector<int> Model::obsShape() const {
    return {width*height*3};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int msize = width*height;
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            int pos = encode_cell(x,y,width);
            obs[0*msize + pos] = state->burning[x][y]? 1 : 0;
            obs[1*msize + pos] = state->out_of_fuel[x][y]? 1 : 0;
            obs[2*msize + pos] = is_target[pos]? 1 : 0;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {width*height*2+1};
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0];
}

inline std::pair<int,int> decode_cell(int cell, int width) {
    return {cell % width, cell / width};
}

Model::Model(const std::string& fileName)
{
    std::ifstream file(fileName);  // Open the file

    if (!file) {
        std::cerr << "Unable to open file: " << fileName << std::endl;
        return;
    }

    file >> width;
    file >> height;

    std::string line;
    std::getline(file, line); //skip empty line
    std::getline(file, line); //skip empty line

    //read init burns until we hit an empty line
    while (std::getline(file, line)) {  // Read each line
        if(line.empty())
            break;
        std::istringstream iss(line);
        int x,y;
        iss >> x;
        iss >> y;
        init_burns.emplace_back(encode_cell(x-1,y-1,width));
    }

    //read targets until we hit an empty line
    is_target = std::vector<bool>(width*height, false);
    while (std::getline(file, line)) {  // Read each line
        if(line.empty())
            break;
        std::istringstream iss(line);
        int x,y;
        iss >> x;
        iss >> y;
        is_target[encode_cell(x-1,y-1,width)] = true;
    }

    cut_connections = std::vector<std::vector<bool>>(height*width, std::vector<bool>(height*width, false));
    while (std::getline(file, line)) {  // Read each line
        std::istringstream iss(line);
        int x1,y1,x2,y2;
        iss >> x1;
        iss >> y1;
        iss >> x2;
        iss >> y2;
        cut_connections[encode_cell(x1-1,y1-1,width)][encode_cell(x2-1,y2-1,width)] = true;
    }

    for(int i = 0; i < width*height*2+1; i++) {
        actions.push_back(i);
    }
}


bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto other_state = dynamic_cast<const Gamestate*>(&other);
    return other_state->burning == burning && other_state->out_of_fuel == out_of_fuel;
}

size_t Gamestate::hash() const
{
    return num_burning | (num_out_of_fuel << 16);
}

void Model::printState(ABS::Gamestate* state) {
    auto* WFState = dynamic_cast<Gamestate*>(state);
    if (!WFState) return;

    std::vector<std::vector<char>> grid(13*width, std::vector<char>(4*height, ' '));
    for(int y = 0; y <height; y++) {
        for(int x = 0; x < height; x++){
            if(WFState->burning[x][y]) {
                grid[x*13][y*4] = 'B';
                for(int dx = -1; dx <= 1; dx++) {
                    for(int dy = -1; dy <= 1; dy++) {
                        if(dx == 0 && dy == 0)
                            continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if(nx < 0 || nx >= width || ny < 0 || ny >= height || cut_connections[encode_cell(x,y,width)][encode_cell(nx,ny,width)])
                            continue;

                        char end = abs(dy) == 1? (dy == 1? 'v' : '^') : (dx == 1? '>' : '<');
                        char mid = abs(dx) == 1 && abs(dy) == 1? (dx*dy < 0? '/' : '\\') : (abs(dx) == 1? '-' : '|');
                        grid[13*x+6*dx][4*y+2*dy] = mid;
                        grid[13*x+7*dx][4*y+3*dy] = end;
                    }
                }
            }
            else if(WFState->out_of_fuel[x][y])
                grid[x*13][y*4] = 'F';
            else if(is_target[encode_cell(x,y,width)])
                grid[x*13][y*4] = 'T';
            else
                grid[x*13][y*4] = '.';
        }
        cout << endl;
    }

    for(int y = 0; y < 4*height; y++) {
        for(int x = 0; x < 13*width; x++) {
            cout << grid[x][y];
        }
        cout << endl;
    }

}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new WF::Gamestate();
    state->out_of_fuel = std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));
    state->burning = std::vector<std::vector<bool>>(width, std::vector<bool>(height, false));
    for(int burning_cell : init_burns) {
        auto pos = decode_cell(burning_cell, width);
        state->burning[pos.first][pos.second] = true;
        state->num_burning++;
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
    return actions;
}

inline int num_burning_neighbors(int x, int y, int width, int height, const std::vector<std::vector<bool>>& burning, const std::vector<std::vector<bool>>& cut) {
    int num_burning = 0;
    int cell_code = encode_cell(x,y,width);
    //iterate over 8-neighborhood
    for(int dx = -1; dx <= 1; dx++) {
        for(int dy = -1; dy <= 1; dy++) {
            if(dx == 0 && dy == 0)
                continue;
            int nx = x + dx;
            int ny = y + dy;
            if(nx < 0 || nx >= width || ny < 0 || ny >= height || cut[encode_cell(nx,ny,width)][cell_code])
                continue;
            if(burning[nx][ny])
                num_burning++;
        }
    }

    return num_burning;
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    double dist = 0;
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            if(state_a->burning[x][y] != state_b->burning[x][y])
                dist++;
            if(state_a->out_of_fuel[x][y] != state_b->out_of_fuel[x][y])
                dist++;
        }
    }
    return dist;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<WF::Gamestate*>(uncasted_state);
    size_t decision_point = 0;

    //parse action
    int atype = action >= width*height? (action == width*height*2? 2 : 1 ) : 0; //0 = cutout, 1 = putout, 2 = noop
    if(atype == 1)
        action -= width*height;
    auto decoded_action = decode_cell(action, width);
    int ax = decoded_action.first;
    int ay = decoded_action.second;

    double p = 1;
    std::uniform_real_distribution<double> dist(0, 1);

    std::vector<std::pair<int,bool>> burn_updates = {};
    std::vector<std::pair<int,bool>> fuel_updates = {};
    int targets_burning = 0, nontargets_burning = 0;

    //iterate over all cells
    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {

            //update targets_burning and nontargets_burning
            if(state->burning[x][y]) {
                if(is_target[encode_cell(x,y,width)])
                    targets_burning++;
                else
                    nontargets_burning++;
            }else if(state->out_of_fuel[x][y] && is_target[encode_cell(x,y,width)]) //this cell must have burned down as we do not allow cutoffs on targets
                targets_burning++;

            //update burns
            bool new_burning = state->burning[x][y];
            if(atype==1 && ax == x && ay == y)
                new_burning = false;

            else if(!state->out_of_fuel[x][y] && !state->burning[x][y]) {
                int burning_neighbors = num_burning_neighbors(x,y,width,height,state->burning,cut_connections);
                if(!is_target[encode_cell(x,y,width)] || burning_neighbors >= 1) {
                    double ignition_prob = 1.0 / ( 1.0 + exp(4.5 - burning_neighbors) );
                    if((decision_outcomes == nullptr && dist(rng) < ignition_prob) || (decision_outcomes != nullptr && (ignition_prob == 1 || (ignition_prob != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))) { //spontaneous ignition
                        new_burning = true;
                        p*=ignition_prob;
                    }else {
                        p*= 1 - ignition_prob;
                    }
                }
            }

            if(state->burning[x][y] != new_burning)
                burn_updates.emplace_back(encode_cell(x,y,width),new_burning);

            //update out-of-fuels
            bool new_out_of_fuel = state->out_of_fuel[x][y] | state->burning[x][y] | (!is_target[encode_cell(x,y,width)] && atype==0 && ax == x && ay == y);
            if(state->out_of_fuel[x][y] != new_out_of_fuel)
                fuel_updates.emplace_back(encode_cell(x,y,width),new_out_of_fuel);

        }
    }

    for(auto& [cell,burning] : burn_updates) {
        auto [x,y] = decode_cell(cell,width);
        if(burning) {
            state->burning[x][y] = true;
            state->num_burning++;
        } else {
            state->burning[x][y] = false;
            state->num_burning--;
        }
    }

    for(auto& [cell,oof]: fuel_updates) {
        auto [x,y] = decode_cell(cell,width);
        if(oof) {
            state->out_of_fuel[x][y] = true;
            state->num_out_of_fuel++;
        } else {
            state->out_of_fuel[x][y] = false;
            state->num_out_of_fuel--;
        }
    }

    //Reward caulcation
    double reward;
    switch(atype) {
        case 0:
            reward = -COST_CUTOUT;
            break;
        case 1:
            reward = -COST_PUTOUT;
            break;
        case 2:
            reward = 0;
            break;
        default:
            assert(false);
    }
    reward -= targets_burning * COST_TARGET_BURN;
    reward -= nontargets_burning * COST_NONTARGET_BURN;

    return {{reward}, p};
}