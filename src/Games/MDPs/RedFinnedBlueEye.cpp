
#include "../../../include/Games/MDPs/RedFinnedBlueEye.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>
#include <chrono>
#include <queue>
#include <set>

using namespace RFBE;

Model::Model(const std::string& fileName, bool deterministic_spread) : deterministic_gambusia_spread(deterministic_spread){
    std::ifstream in(fileName);
    if(!in.is_open()){
        std::cerr << "Could not open file: " << fileName << std::endl;
        exit(1);
    }

    int num_springs;
    in >> num_springs;
    in >> action_points;
    in >> poison_success_prob >> tranloc_succ_prob >> manual_succ_prob;
    for(int i = 0; i < 7; i++) //water probs
        in >> water_level_probs[i];

    //sanity check
    double sum = 0;
    for(int i = 0; i < 7; i++)
        sum += water_level_probs[i];
    assert (std::fabs(sum - 1.0) < 1e-6);

    for(int i = 0; i < num_springs; i++){
        double mort_prob;
        in >> mort_prob;
        natural_mortality_prob.push_back(mort_prob);
    }
    int num_connections;
    in >> num_connections;
    connected_springs_by_water_level_spread_prob = std::vector<std::vector<std::vector<std::pair<int,double>>>>(num_springs,std::vector<std::vector<std::pair<int,double>>>(7));
    for(int i = 0; i < num_connections; i++){
        int from,to, water_lvl;
        double gamb_prob;
        in >> from >> to >> water_lvl >> gamb_prob;
        connected_springs_by_water_level_spread_prob[from][water_lvl].emplace_back(to,gamb_prob);
    }

    int num_inits;
    in >> num_inits;
    initial_spring_populations = std::vector<int>(num_springs,0);
    for(int i = 0; i < num_inits; i++){
        int spring, pop;
        in >> spring >> pop;
        initial_spring_populations[spring] = pop;
    }

    assert (num_springs <= 64 && action_points <= 4); //if violated, the 32-bit action encoding is no longer possible
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    auto* other_state = dynamic_cast<const Gamestate*>(&other);
    return spring_populations == other_state->spring_populations && transloc_cooldown == other_state->transloc_cooldown;
}

size_t Gamestate::hash() const
{
    int hash = 0;
    for(size_t i = 0; i < spring_populations.size(); i++)
        hash = (hash << 2) | spring_populations[i];
    hash = (hash << 3) | transloc_cooldown;
    return hash;
}

void Model::printState(ABS::Gamestate* state) {
    auto rfbe_state = dynamic_cast<Gamestate*>(state);
    std::cout << "Spring populations: ";
    for(size_t i = 0; i < rfbe_state->spring_populations.size(); i++){
        std::cout << rfbe_state->spring_populations[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Translocation cooldown: " << rfbe_state->transloc_cooldown << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)  {
    auto state = new Gamestate();
    state->transloc_cooldown =0;
    state->spring_populations = initial_spring_populations;
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


void decodeSequence(int action, std::vector<std::pair<int,int>>& seq){
    for(int i = 0; i < 4; i++){
        int type = action & ((1 << 2)-1);
        if(type == 3)
            return;
        action = action >> 2;
        int spring = action & ((1 << 6)-1);
        action = action >> 6;
        seq.emplace_back(type,spring);
    }
    assert (false);
}

inline bool addable(std::vector<std::pair<int,int>>& seq, int spring){
    for(auto& [a,s] : seq){
        if(s == spring)
            return false;
    }
    return true;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);

    bool extinct = true;
    for(size_t i = 0; i < state->spring_populations.size(); i++){
        if(state->spring_populations[i] == 1){
            extinct = false;
            break;
        }
    }

    std::vector<int> poisons = {};
    std::vector<int> relocates = {};
    std::vector<int> removes = {};
    for(size_t i = 0; i < state->spring_populations.size(); i++){
        if(state->spring_populations[i] == 2){
            poisons.push_back(i);
            removes.push_back(i);
        }
        if(state->spring_populations[i] == 0 && state->transloc_cooldown ==0 && !extinct)
            relocates.push_back(i);
    }

    std::vector<std::vector<std::pair<int,int>>> asequences = {};
    std::queue<std::pair<std::vector<std::pair<int,int>>,int>> q;
    q.push( {{},action_points});
    while(!q.empty()){
        auto [seq,points] = q.front();
        q.pop();
        auto finalized_seq = seq;
        finalized_seq.emplace_back(3,0);
        asequences.push_back(finalized_seq);
        if(points > 0){
            //add poison
            for(int & poison : poisons){
                if(!addable(seq,poison))
                    continue;
                auto new_seq = seq;
                new_seq.emplace_back(0,poison);
                q.emplace(new_seq,points-1);
            }
        }
        if(points > 1){
            //add remove
            for(int & remove : removes){
                if(!addable(seq,remove))
                    continue;
                auto new_seq = seq;
                new_seq.emplace_back(1,remove);
                q.emplace(new_seq,points-2);
            }
        }
        if(points > 2){
            //add relocate
            for(int & relocate : relocates){
                if(!addable(seq,relocate))
                    continue;
                auto new_seq = seq;
                new_seq.emplace_back(2,relocate);
                q.emplace(new_seq,points-3);
            }
        }
    }

    std::vector<int> actions;
    for(auto& seq : asequences){
        int action = 0;
        for(size_t i = 0; i < seq.size(); i++){
            auto& [type,spring]  = seq[i];
            action = (action) | (type << (8* i));
            action = (action)| (spring << (2 + 8*i));
        }
        actions.push_back(action);
    }

    assert (!actions.empty());
    return actions;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
        auto* newState = dynamic_cast<RFBE::Gamestate*>(uncasted_state);
        auto oldState = *newState;
        size_t decision_point = 0;

        std::vector<bool> poisoned_springs = std::vector<bool>(oldState.spring_populations.size(), false);
        std::vector<bool> remove_manually = std::vector<bool>(oldState.spring_populations.size(), false);
        std::vector<bool> translocate_to = std::vector<bool>(oldState.spring_populations.size(), false);
        std::vector<std::pair<int,int>> action_seq;
        decodeSequence(action,action_seq);
        for(auto& [type,spring] : action_seq){
            if(type == 0)
                poisoned_springs[spring] = true;
            if(type == 1)
                remove_manually[spring] = true;
            if(type == 2)
                translocate_to[spring] = true;
        }

        std::discrete_distribution<int> water_level_dist(std::begin(water_level_probs), std::end(water_level_probs));
        int water_level;
        if (decision_outcomes == nullptr)
            water_level = water_level_dist(rng);
        else {
            std::vector<int> non_zero;
            for(int i = 0; i < WATER_LEVELS; i++) {
                if(water_level_probs[i] > 0)
                    non_zero.push_back(i);
            }
            water_level = non_zero[getDecisionPoint(decision_point,0,non_zero.size()-1,decision_outcomes)];
        }

        double reward = 0;
        bool attempted_translocation = false;
        bool extinct_red_finned_blue_eye = true;

        for(size_t s = 0; s < oldState.spring_populations.size(); s++){
            attempted_translocation = attempted_translocation || translocate_to[s];

            if(oldState.spring_populations[s] == 1)
                extinct_red_finned_blue_eye = false;

            if(poisoned_springs[s])
                reward -= POISON_PENALTY;

            if(oldState.spring_populations[s] == 1)
                reward += RED_FINNED_BLUE_EYE_REWARD;

            if(poisoned_springs[s]){
                std::bernoulli_distribution dist(poison_success_prob);
                if( (decision_outcomes == nullptr && dist(rng)) || (decision_outcomes != nullptr && (poison_success_prob == 1 || (poison_success_prob != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 1)))){
                    newState->spring_populations[s] = 0;
                    continue;
                }
            }
            if(remove_manually[s]){
                std::bernoulli_distribution dist(manual_succ_prob);
                if( (decision_outcomes == nullptr && dist(rng)) || (decision_outcomes != nullptr && (manual_succ_prob == 1 || (manual_succ_prob != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 1)))){
                    newState->spring_populations[s] = 0;
                    continue;
                }
            }
            if(translocate_to[s]){
                std::bernoulli_distribution dist(tranloc_succ_prob);
                if( (decision_outcomes == nullptr && dist(rng)) || (decision_outcomes != nullptr && (tranloc_succ_prob == 1 ||  (tranloc_succ_prob != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 1)))){
                    newState->spring_populations[s] = 1; //1 = red finned blue eye
                    continue;
                }
            }

            if(oldState.spring_populations[s] == 1){ // 1= red finned blue eye.
                std::bernoulli_distribution dist(natural_mortality_prob[s]);
                if( (decision_outcomes == nullptr && dist(rng)) || (decision_outcomes != nullptr && (natural_mortality_prob[s] == 1 || (natural_mortality_prob[s] != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 1)))){
                    newState->spring_populations[s] = 0;
                    continue;
                }
            }

            if (oldState.spring_populations[s] != 2) {
                double not_spread_prob = 1.0;
                for(auto& [neighbor,spread_prob] : connected_springs_by_water_level_spread_prob[s][water_level]){
                    if(oldState.spring_populations[neighbor] == 2) {
                        if(deterministic_gambusia_spread){
                            not_spread_prob = 0.0;
                            break;
                        }
                        else
                            not_spread_prob *= 1.0 - spread_prob;
                    }
                }
                std::bernoulli_distribution dist(1.0 - not_spread_prob);
                if( (decision_outcomes == nullptr && dist(rng)) || (decision_outcomes != nullptr && (not_spread_prob == 0 || (not_spread_prob != 1 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 1)))){
                    newState->spring_populations[s] = 2;
                }
            }

        }

        //State outcome probability calculation. Trick: The springs are conditionally independent given the water level
        double outcomeProb = 0.0;
        for(int cf_water_level = 0; cf_water_level < WATER_LEVELS; cf_water_level ++) {
            double wl_prob = 1;
            for (int s = 0; s < static_cast<int>(oldState.spring_populations.size()); s++) {
                double p_poison = poisoned_springs[s] ? poison_success_prob : 0;
                double p_manual = remove_manually[s] ? manual_succ_prob : 0;
                double p_transloc = translocate_to[s] ? tranloc_succ_prob : 0;
                double p_die = oldState.spring_populations[s] == 1 ? natural_mortality_prob[s] : 0;
                double not_spread_prob = 1.0;
                if (oldState.spring_populations[s] != 2 && ((newState->spring_populations[s] == 2 && oldState.spring_populations[s] != 2) || (newState->spring_populations[s] == 1 && oldState.spring_populations[s] == 1) || (newState->spring_populations[s] == 0 && oldState.spring_populations[s] == 0))) { //condition only for computational speedup
                    for (auto & [neighbor, spread_prob] : connected_springs_by_water_level_spread_prob[s][cf_water_level]) {
                        if (oldState.spring_populations[neighbor] == 2){
                            if(deterministic_gambusia_spread){
                                not_spread_prob = 0.0;
                                break;
                            }
                            else
                                not_spread_prob *= 1.0 - spread_prob;
                        }
                    }
                }
                double p_spread = oldState.spring_populations[s] != 2? 1.0 - not_spread_prob : 0;
                if (newState->spring_populations[s] == 0) {
                    double p = p_poison + (1 - p_poison) * p_manual + (1 - p_poison) * (1 - p_manual) * (1-p_transloc) * p_die;
                    if (oldState.spring_populations[s] == 0)
                        p += (1 - p_poison) * (1 - p_manual) * (1-p_transloc) * (1 - p_die) * (1 - p_spread);
                    wl_prob *= p;
                }else if (newState->spring_populations[s] == 1) {
                    double p = p_transloc;
                    if (oldState.spring_populations[s] == 1)
                        p += (1 - p_transloc) * (1 - p_die) * (1 - p_spread);
                    wl_prob *= p;
                }else if (newState->spring_populations[s] == 2) {
                    double p = 0;
                    if (oldState.spring_populations[s] == 2)
                        p += (1 - p_poison) * (1 - p_manual) * (1 - p_transloc);
                    else
                        p += (1 - p_poison) * (1 - p_manual) * (1 - p_die) * (1 - p_transloc) * p_spread;
                    wl_prob *= p;
                }
            }
            outcomeProb += wl_prob* water_level_probs[cf_water_level];
        }

        newState->transloc_cooldown = attempted_translocation? TRANSLOC_CD : std::max(0, oldState.transloc_cooldown - 1);

        if(extinct_red_finned_blue_eye)
            reward -= EXTINCT_PENALTY;

        std::vector<double> rew(1, reward);
        return std::make_pair(rew, outcomeProb);
}