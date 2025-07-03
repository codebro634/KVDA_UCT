#include "../../../include/Games/MDPs/Tamarisk.h"
#include <sstream>
#include <functional>
#include <cmath>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <fstream>

using namespace TAM;

inline bool getPlant(int plant_at, int reach, int slot, int max_slots_per_reach) {
    return (plant_at & (1 << (reach * max_slots_per_reach + slot))) != 0;
}

std::vector<int> Model::obsShape() const {
    return {num_slots * 2};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs){
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    int slot_num = 0;
    for (int r = 0; r < num_reaches; ++r) {
        for (int s = 0; s < slots_at_reach[r]; ++s) {
            obs[slot_num] = getPlant(state->tamarisk_at, r, s, state->max_slots_per_reach);
            obs[slot_num + num_slots] = getPlant(state->native_at, r, s, state->max_slots_per_reach);
            slot_num++;
        }
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    assert (!COMBINATORIAL_ACTION_SPACE);
    return {num_reaches * 2 + 1};
}

int Model::encodeAction(int* decoded_action) {
    assert (!COMBINATORIAL_ACTION_SPACE);
    return decoded_action[0];
}

inline void placePlant(int& plant_at, int reach, int slot, int max_slots_per_reach) {
    plant_at |= 1 << (reach * max_slots_per_reach + slot);
}

inline void removePlant(int& plant_at, int reach, int slot, int max_slots_per_reach) {
    plant_at &= ~(1 << (reach * max_slots_per_reach + slot));
}

void Model::init_model_from_file(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file at " << filePath << std::endl;
        std::exit(1);
    }

    file >> num_reaches;
    num_slots = 0;
    int max_slots_per_reach = 0;
    init_native_at = 0;
    init_tamarisk_at = 0;
    for(int i = 0; i < num_reaches; i++)
    {
        int slots;
        file >> slots;
        slots_at_reach.push_back(slots);
        num_slots += slots;
        max_slots_per_reach = std::max(max_slots_per_reach, slots);
    }
    
    //read next lines until eof
    int reach, slot, plant;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        sscanf(line.c_str(), "%d %d %d", &reach, &slot, &plant);

        if(plant == 0)
            placePlant(init_native_at, reach, slot,max_slots_per_reach );
        else
            placePlant(init_tamarisk_at, reach, slot,max_slots_per_reach);
    }

    if(COMBINATORIAL_ACTION_SPACE) {
        action_list = std::vector<int>(1 << (num_reaches * 2));
        for (int i = 0; i < (1 << (num_reaches * 2)); ++i) {
            action_list[i] = i;
        }
    }
    else {
        action_list = {};
        for (int i = 0; i <= 2*num_reaches; ++i) {
            action_list.push_back(i);
        }
    }

    file.close();

    assert (max_slots_per_reach* num_reaches <= 32); // we save the plants matrix in a 32-bit int
}

Model::Model()
{
    init_model_from_file("../resources/TamariskMaps/1.txt");
}

Model::Model(const std::string& filePath)
{
    init_model_from_file(filePath);
}


std::string Gamestate::toString() const {
    std::ostringstream oss;
    oss << ABS::Gamestate::toString() << "\n";
    oss << "Tamarisk at:\n";
    for (size_t r = 0; r < num_tamarisk_reach.size(); ++r) {
        oss << "Reach " << r << ": ";
        for (int i = 0; i < slots_at_reach[r]; ++i) {
            oss << getPlant(tamarisk_at,r,i,max_slots_per_reach) << " ";
        }
        oss << "\n";
    }
    oss << "Native at:\n";
    for (size_t r = 0; r < num_tamarisk_reach.size(); ++r) {
        oss << "Reach " << r << ": ";
        for (int i = 0; i < slots_at_reach[r]; ++i) {
            oss << getPlant(native_at,r,i,max_slots_per_reach) << " ";
        }
        oss << "\n";
    }
    return oss.str();
}


bool Gamestate::operator==(const ABS::Gamestate& other) const {
    const auto* o = dynamic_cast<const Gamestate*>(&other);
    if (!o) return false;
    return tamarisk_at == o->tamarisk_at && native_at == o->native_at;
}

//no collisions if there are fewer than 16 reaches
size_t Gamestate::hash() const {
    return (tamarisk_at << 16) | native_at;
}


ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto* state = new Gamestate();
    state->tamarisk_at = init_tamarisk_at;
    state->native_at = init_native_at;
    state->num_tamarisk_reach = std::vector<int>(num_reaches, 0);
    state->num_tamarisk_reach = std::vector<int>(num_reaches, 0);
    for (int r = 0; r < num_reaches; ++r) {
        for (int s = 0; s < slots_at_reach[r]; ++s) {
            state->num_tamarisk_reach[r] += getPlant(init_tamarisk_at, r, s, state->max_slots_per_reach);
        }
    }
    state->num_slots = num_slots;
    state->slots_at_reach = slots_at_reach;
    state->max_slots_per_reach = *std::ranges::max_element(slots_at_reach);
    return state;
}


int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);
    if (!state) throw std::runtime_error("Invalid state type in copyState");
    return new Gamestate(*state);
}


void Model::printState(ABS::Gamestate* uncasted_state) {
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);
    if (!state) throw std::runtime_error("Invalid state type in printState");
    std::cout << state->toString() << std::endl;
}


std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    return action_list;
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return __builtin_popcount( state_a->tamarisk_at ^ state_b->tamarisk_at) + __builtin_popcount( state_a->native_at ^ state_b->native_at);
}

std::pair<std::vector<double>, double> Model::applyAction_(
    ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    size_t decision_point = 0;

    // Cast state and save old state
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);
    Gamestate old_state = *state;
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // Extract actions for each reach
    std::vector<bool> eradicate_action(num_reaches, false);
    std::vector<bool> restore_action(num_reaches, false);

    if(COMBINATORIAL_ACTION_SPACE) {
        for (int r = 0; r < num_reaches; ++r) {
            if ((action >> r) % 2)
                eradicate_action[r] = true;
            if ((action >> (num_reaches + r)) % 2)
                restore_action[r] = true;
        }
    }else {
        if(action < num_reaches)
            eradicate_action[action] = true;
        else if(action != 2*num_reaches) {
            restore_action.at(action - num_reaches) = true;
        }
    }

    // Track rewards, sample prob and sample num
    double reward = 0.0;
    double sample_prob = 1.0;

    //Get reward
    for (int r = 0; r < num_reaches; ++r){
        reward -= COST_PER_INVADED_REACH * (state->num_tamarisk_reach[r] > 0? 1:0);
        reward -= RESTORATION_COST * restore_action[r];
        reward -= ERADICATION_COST * eradicate_action[r];
        for(int s = 0; s < slots_at_reach[r]; s++){
            reward -= COST_PER_TREE * getPlant(state->tamarisk_at,r,s,state->max_slots_per_reach);
            reward -= COST_PER_EMPTY_SLOT * (!getPlant(state->tamarisk_at,r,s,state->max_slots_per_reach) && !getPlant(state->native_at,r,s,state->max_slots_per_reach));
            reward -= RESTORATION_COST_FOR_EMPTY_SLOT * (!getPlant(state->tamarisk_at,r,s,state->max_slots_per_reach) && !getPlant(state->native_at,r,s,state->max_slots_per_reach) && restore_action[r]);
        }
    }
    // Update Gamestae
    for (int r = 0; r < num_reaches; ++r)
    {
        int tamarisk_count = 0;
        for(int s = 0; s < slots_at_reach[r]; s++)
        {
            bool set_tamarisk;
            //Update tamarisk
            if(getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach)){
                bool b = decision_outcomes == nullptr ||COMPETITION_WIN_RATE_TAMARISK == 1 || COMPETITION_WIN_RATE_TAMARISK == 0? dist(rng) < COMPETITION_WIN_RATE_TAMARISK : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
                if(b){
                    sample_prob *= COMPETITION_WIN_RATE_TAMARISK;
                }else
                    sample_prob *= 1-COMPETITION_WIN_RATE_TAMARISK;
                set_tamarisk = b;
            }else if(!getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && eradicate_action[r]){
                set_tamarisk = false;
            }else if(getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && eradicate_action[r]){
                bool b = decision_outcomes == nullptr? dist(rng) >= ERADICATION_RATE : getDecisionPoint(decision_point,0,1,decision_outcomes) == 1;
                if(b){
                    sample_prob *= 1-ERADICATION_RATE;
                }else
                    sample_prob *= ERADICATION_RATE;
                set_tamarisk = b;
            } else if(getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach)){
                bool b = decision_outcomes == nullptr || DEATH_RATE_TAMARISK == 0 || DEATH_RATE_TAMARISK == 1? dist(rng) >= DEATH_RATE_TAMARISK : getDecisionPoint(decision_point,0,1,decision_outcomes) == 1;
                if(b){
                    sample_prob *= 1-DEATH_RATE_TAMARISK;
                }else
                    sample_prob *= DEATH_RATE_TAMARISK;
                set_tamarisk = b;
            } else if(!getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) and !getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach))
            {
                double p_not_spread_same_reach = std::pow(1-DOWNSTREAM_SPREAD_RATE,old_state.num_tamarisk_reach[r]);
                double p_not_spread_upstream = r > 0? std::pow(1-UPSTREAM_SPREAD_RATE,old_state.num_tamarisk_reach[r-1]) : 1;
                double p_not_spread_downstream = r < num_reaches-1? std::pow(1-DOWNSTREAM_SPREAD_RATE,old_state.num_tamarisk_reach[r+1]) : 1;
                double p = EXOGENOUS_PROD_RATE_TAMARISK + (1 - EXOGENOUS_PROD_RATE_TAMARISK) * (1 -
                        p_not_spread_downstream * p_not_spread_same_reach * p_not_spread_upstream);

                bool b = decision_outcomes == nullptr || p == 0 || p == 1 ? dist(rng) < p : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
                if(b){
                    sample_prob *= p;
                }else
                    sample_prob *= 1-p;
                set_tamarisk = b;
            }else
            {
                set_tamarisk = getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach);
            }
            if(set_tamarisk)
                tamarisk_count++;
            if(set_tamarisk)
                placePlant(state->tamarisk_at,r,s,state->max_slots_per_reach);
            else
                removePlant(state->tamarisk_at,r,s,state->max_slots_per_reach);
            

            //Update native
            bool set_native;
            if(getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach))
            {
                bool b = decision_outcomes == nullptr || COMPETITION_WIN_RATE_NATIVE == 0 || COMPETITION_WIN_RATE_NATIVE == 1? dist(rng) < COMPETITION_WIN_RATE_NATIVE : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
                if(b){
                    sample_prob *= COMPETITION_WIN_RATE_NATIVE;
                }else
                    sample_prob *= 1-COMPETITION_WIN_RATE_NATIVE;
                set_native = b;
            }else if(!getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach) && restore_action[r])
            {
                set_native = true;
            }
            else if(!getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && !getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach) && restore_action[r])
            {
                bool b = decision_outcomes == nullptr || (RESTORATION_RATE == 0) || RESTORATION_RATE == 1? dist(rng) < RESTORATION_RATE : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
                if(b){
                    sample_prob *= RESTORATION_RATE;
                }else
                    sample_prob *= 1-RESTORATION_RATE;
                set_native = b;
            } else if(getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach))
            {
                bool b = decision_outcomes == nullptr || (DEATH_RATE_NATIVE == 0 || DEATH_RATE_NATIVE == 1)? dist(rng) >= DEATH_RATE_NATIVE : getDecisionPoint(decision_point,0,1,decision_outcomes) == 1;
                if(b){
                    sample_prob *= 1-DEATH_RATE_NATIVE;
                }else
                    sample_prob *= DEATH_RATE_NATIVE;
                set_native = b;
            } else if(!getPlant(old_state.tamarisk_at,r,s,old_state.max_slots_per_reach) && !getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach))
            {
                bool b = decision_outcomes == nullptr || (EXOGENOUS_PROD_RATE_NATIVE == 0 || EXOGENOUS_PROD_RATE_NATIVE == 1)? dist(rng) < EXOGENOUS_PROD_RATE_NATIVE : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
                if(b){
                    sample_prob *= EXOGENOUS_PROD_RATE_NATIVE;
                }else
                    sample_prob *= 1-EXOGENOUS_PROD_RATE_NATIVE;
                set_native = b;
            }else
            {
                set_native = getPlant(old_state.native_at,r,s,old_state.max_slots_per_reach);
            }
            if(set_native)
                placePlant(state->native_at,r,s,state->max_slots_per_reach);
            else
                removePlant(state->native_at,r,s,state->max_slots_per_reach);
        }
        state->num_tamarisk_reach[r] = tamarisk_count;

    }

    return {{reward}, sample_prob};
}