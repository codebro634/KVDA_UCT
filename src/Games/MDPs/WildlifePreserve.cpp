#include "../../../include/Games/MDPs/WildlifePreserve.h"
#include <fstream>
#include <sstream>
#include <cmath>

using namespace WLP;

std::vector<int> Model::obsShape() const {
    return {static_cast<int>(area_attack_penalty.size() * (poacher_max_memory + 1) + area_attack_penalty.size() + 1)};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    for(size_t i = 0; i < state->defenses.size(); i++){
        for(size_t j = 0; j < state->defenses[i].size(); j++)
            obs[i*state->defenses[i].size() + j] = state->defenses[i][j] ? 1 : 0;
        obs[state->defenses.size()*state->defenses[i].size() + i] = state->area_successful_attacks[i] ? 1 : 0;
    }
    obs[state->defenses.size()*state->defenses[0].size() + state->defenses.size()] = state->caught_poachers.size();
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    auto shape = std::vector<int>(rangers, 0);
    for(int i = 0; i < rangers; i++)
        shape[i] = static_cast<int>(area_attack_penalty.size());
    return shape;
}

int Model::encodeAction(int* decoded_action) {
    int action = 0;
    for(int i = 0; i < rangers; i++)
        action |= decoded_action[i] << 4*i;
    return action;
}

bool Gamestate::operator==(const ABS::Gamestate& o) const {
    const auto* other = dynamic_cast<const Gamestate*>(&o);
    return defenses == other->defenses && area_successful_attacks == other->area_successful_attacks && caught_poachers == other->caught_poachers;
}

size_t Gamestate::hash() const {
    int hash = 0;

    for(size_t i = 0; i < defenses.size(); i++)
        for(size_t j = 0; j < defenses[i].size(); j++)
            hash = (hash << 1) | defenses[i][j];

    for(size_t i = 0; i < area_successful_attacks.size(); i++)
        hash = (hash << 1) | area_successful_attacks[i];

    hash = (hash << 4) | caught_poachers.size();
    return hash;
}

Model::Model(const std::string& filePath){
    std::ifstream in(filePath);
    //test if file was opened correctly
    if(!in.is_open()){
        std::cerr << "Could not open file: " << filePath << std::endl;
        exit(1);
    }

    size_t num_areas;
    in >> num_areas >> rangers >> num_poachers >> poacher_max_memory;
    assert (num_areas <= 16 && rangers <= 8);

    size_t attack_weight_triplets, max_num;
    in >> attack_weight_triplets >> max_num;
    poacher_area_num_attack_weight = std::vector<std::vector<std::vector<double>>>(num_poachers, std::vector<std::vector<double>>(num_areas, std::vector<double>(max_num+1, 0)));
    for(size_t i = 0; i < attack_weight_triplets; i++){
        int poacher;
        int area;
        int num;
        double weight;
        in >> poacher >> area >> num >> weight;
        poacher_area_num_attack_weight[poacher][area][num] = weight;
    }

    area_defense_reward = std::vector<double>(num_areas, 10.0);
    area_attack_penalty = std::vector<double>(num_areas, -10.0);
    for(size_t i = 0; i < num_areas; i++)
        in >> area_attack_penalty[i] >> area_defense_reward[i];

    //create a an std::vector<std::vector<int>> where each vector is the sequence of areas the rangers defend
    std::vector<std::vector<int>> ranger_defense_sequences;
    std::vector<std::vector<int>> stack = {{}};
    while(!stack.empty()){
        auto current = stack.back();
        stack.pop_back();
        if(static_cast<int>(current.size()) == rangers){
            ranger_defense_sequences.push_back(current);
        } else {
            for(size_t i = 0; i < num_areas; i++){
                auto new_current = current;
                new_current.push_back(i);
                stack.push_back(new_current);
            }
        }
    }
    for(auto& sequence: ranger_defense_sequences){
        int action = 0;
        for(int i = 0; i < rangers; i++){
            action |= sequence[i] << 4*i;
        }
        actions.push_back(action);
    }
}

void Model::printState(ABS::Gamestate* uncasted_state){
    auto* s = dynamic_cast<Gamestate*>(uncasted_state);
    std::cout << "Last defended: ";
    for(size_t i = 0; i < static_cast<unsigned int>(poacher_max_memory); i++) {
        std::cout << i << ": ";
        for(size_t j = 0; j < s->defenses.size(); j++)
            std::cout << s->defenses[j][i] << " ";
        std::cout << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Area successful attacks: ";
    for(size_t i = 0; i < s->area_successful_attacks.size(); i++)
        std::cout << s->area_successful_attacks[i] << " ";
    std::cout << std::endl;
    std::cout << "Caught poachers: ";
    for(int i: s->caught_poachers)
        std::cout << i << " ";
    std::cout << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937&){
    auto* s = new Gamestate();
    s->defenses = std::vector<std::vector<bool>>(area_attack_penalty.size(), std::vector<bool>(poacher_max_memory+1, false));
    s->area_successful_attacks = std::vector<bool>(poacher_area_num_attack_weight[0].size(), false);
    s->caught_poachers = {};
    return s;
}

int Model::getNumPlayers(){
    return 1;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state){
    auto* sOld = dynamic_cast<Gamestate*>(uncasted_state);
    auto* sNew = new Gamestate();
    *sNew = *sOld;
    return sNew;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state){
   return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){
    size_t decision_point = 0;
    auto newState = dynamic_cast<Gamestate*>(uncasted_state);
    auto oldState = *newState;

    std::vector<bool> area_defended = std::vector<bool>(poacher_area_num_attack_weight[0].size(), false);
    for(int i = 0; i < rangers; i++){
       int area = (action >> 4*i) & 15;
       area_defended[area] = true;
       newState->defenses[area][0] = true;
    }

    std::vector<int> defenses_in_poacher_memory = std::vector<int>(area_attack_penalty.size(), 0);
    for(size_t area = 0; area < poacher_area_num_attack_weight[0].size(); area++){
        int defs = 0;
        for(int i = 0; i < poacher_max_memory+1; i++) {
            defs += oldState.defenses[area][i]? 1:0;
            if(i < poacher_max_memory)
                newState->defenses[area][i+1] = oldState.defenses[area][i];
        }
        defenses_in_poacher_memory[area] = defs;
    }

    std::vector<int> poacher_attacks = std::vector<int>(num_poachers, 0);
    for(int p = 0; p < num_poachers; p++){
        std::vector<double> area_attack_probs = std::vector<double>(poacher_area_num_attack_weight[0].size(), 0);
        double sum = 0;
        for(size_t area = 0; area < poacher_area_num_attack_weight[0].size(); area++) {
            int num_defs = defenses_in_poacher_memory[area];
            area_attack_probs[area] = poacher_area_num_attack_weight[p][area][num_defs];
            sum += area_attack_probs[area];
        }
        for(size_t area = 0; area < poacher_area_num_attack_weight[0].size(); area++) {
            if (sum == 0)
                area_attack_probs[area] = 1.0 / poacher_area_num_attack_weight[0].size();
            else
                area_attack_probs[area] /= sum;
        }
        std::discrete_distribution<int> dist(area_attack_probs.begin(), area_attack_probs.end());
        int area_atk;
        if (decision_outcomes == nullptr)
            area_atk = dist(rng);
        else{
            std::vector<int> non_zero;
            for(int i = 0; i < static_cast<int>(area_attack_probs.size()); i++) {
                if(area_attack_probs[i] > 0)
                    non_zero.push_back(i);
            }
            area_atk = non_zero[getDecisionPoint(decision_point,0,non_zero.size()-1,decision_outcomes)];
        }
        poacher_attacks[p] = area_atk;
    }

    newState->caught_poachers.clear();
    std::set<int> successful_area_attacks = {};
    for(int p = 0; p < num_poachers; p++){
        if(area_defended[poacher_attacks[p]])
            newState->caught_poachers.insert(p);
        else if(!oldState.caught_poachers.contains(p))
            successful_area_attacks.insert(poacher_attacks[p]);
    }
    for(size_t area = 0; area < poacher_area_num_attack_weight[0].size(); area++){
        if(successful_area_attacks.contains(area))
            newState->area_successful_attacks[area] = true;
        else
            newState->area_successful_attacks[area] = false;
    }

    double reward = 0;
    for(size_t area = 0; area < poacher_area_num_attack_weight[0].size(); area++){
        if(oldState.area_successful_attacks[area])
            reward += area_attack_penalty[area];
        else
            reward += area_defense_reward[area];
    }
    std::vector<double> rewards = { reward };

    return std::make_pair(rewards, std::nan(""));
}

