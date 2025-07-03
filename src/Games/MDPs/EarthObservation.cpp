#include "../../../include/Games/MDPs/EarthObservation.h"
#include <fstream>
#include <sstream>
#include <cmath>

using namespace EO;

std::vector<int> Model::obsShape() const {
    return {static_cast<int>(init_targets.size() * 2)};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    for(size_t i = 0; i < init_targets.size(); i++) {
        obs[i] = static_cast<int>(dynamic_cast<Gamestate*>(uncasted_state)->visibility[i]);
        obs[i + init_targets.size()] = static_cast<int>(dynamic_cast<Gamestate*>(uncasted_state)->is_target[i]);
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {4};
}

int Model::encodeAction(int* decoded_action) {
   return decoded_action[0];
}

bool Gamestate::operator==(const ABS::Gamestate& o) const {
    const auto* other = dynamic_cast<const Gamestate*>(&o);
    return focal_point == other->focal_point && visibility == other->visibility && is_target == other->is_target;
}

size_t Gamestate::hash() const {
    size_t hash = 0;
    for(size_t i = 0; i < visibility.size(); i++)
        hash = (hash << 3) | (visibility[i] << 1) | is_target[i];

    hash = (hash << 3) | focal_point;
   return hash;
}

Model::Model(const std::string& filePath){

    std::ifstream in(filePath);
    if(!in.is_open()){
        std::cerr << "Could not open file: " << filePath << std::endl;
        exit(1);
    }

    int num_patches;
    in >> num_patches;

    in >> init_focal_point;
    //std::cout << "Focal point: " << init_focal_point << std::endl;
    in >> failure_probs[0] >> failure_probs[1] >> failure_probs[2];
    //std::cout << "Failure probs: " << failure_probs[0] << " " << failure_probs[1] << " " << failure_probs[2] << std::endl;
    in >> vis_change_probs[0][0] >> vis_change_probs[0][1] >> vis_change_probs[0][2];
    //std::cout << "Visibility change probs: " << vis_change_probs[0][0] << " " << vis_change_probs[0][1] << " " << vis_change_probs[0][2] << std::endl;
    in >> vis_change_probs[1][0] >> vis_change_probs[1][1] >> vis_change_probs[1][2];
    //std::cout << vis_change_probs[1][0] << " " << vis_change_probs[1][1] << " " << vis_change_probs[1][2] << std::endl;
    in >> vis_change_probs[2][0] >> vis_change_probs[2][1] >> vis_change_probs[2][2];
    //std::cout << vis_change_probs[2][0] << " " << vis_change_probs[2][1] << " " << vis_change_probs[2][2] << std::endl;

    for(int i = 0; i < num_patches; i++){
        int vis;
        in >> vis;
        init_visibility.push_back(vis);
    }


    //print vis
    // for(int i : init_visibility)
    //     std::cout << i << " ";
    // std::cout << std::endl;

    int num_connections;
    in >> num_connections;

   connection_per_direction.resize(num_patches, std::vector<int>(3,-1)); //-1 means there is no connection
    for(int i=0;i<num_connections;i++){
        int from,to,dir;
        in >> from >> dir >> to;
        connection_per_direction[from][dir] = to;
    }

    int num_init_targets;
    in >> num_init_targets;
    init_targets.resize(num_patches, false);
    for(int i = 0; i < num_init_targets; i++){
        int target;
        in >> target;
        init_targets[target] = true;
    }

}

void Model::printState(ABS::Gamestate* uncasted_state){
    auto* s = dynamic_cast<Gamestate*>(uncasted_state);

    //print connections
    std::cout << "Focal point: " << s->focal_point << std::endl;
    for(size_t p = 0; p < init_targets.size(); p++)
        std::cout << p << "-> " << connection_per_direction[p][0] << " " << connection_per_direction[p][1] << " " << connection_per_direction[p][2] << std::endl;

    std::cout << "Visibility: ";
    for(int i : s->visibility)
        std::cout << i << " ";
    std::cout << std::endl;
    std::cout << "Targets: ";
    for(size_t i = 0; i < s->is_target.size(); i++) {
        if(s->is_target[i])
        std::cout << i << " ";
    }
    std::cout << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937&){
    auto* s = new Gamestate();
    s->focal_point = init_focal_point;
    s->visibility = init_visibility;
    s->is_target = init_targets;
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
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto actions = std::vector<int>();
    for(int i = 0; i < 3; i++){
        if(connection_per_direction[state->focal_point][i] != -1)
            actions.push_back(i);
    }
    if(connection_per_direction[state->focal_point][2] != -1)
        actions.push_back(3);

    return actions; //0 = slew north-east, 1 = slew south-east, 2 = slew east, 3 = slew east + take iamge
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){

    size_t decision_point = 0;
    auto newState = dynamic_cast<Gamestate*>(uncasted_state);
    auto oldState = *newState;

    bool take_image = action == 3;
    if(take_image)
        action--;

    newState->focal_point = connection_per_direction[oldState.focal_point][action];
    if (newState->focal_point == -1) {
        std::cerr << "Invalid action: " << action << std::endl;
        exit(1);
    }

    double outcomeProb = 1;

    int num_targets = 0;
    for(size_t p = 0; p < oldState.visibility.size(); p++){
        if (decision_outcomes == nullptr)
            newState->visibility[p] = std::discrete_distribution<int>(std::begin(vis_change_probs[oldState.visibility[p]]),std::end(vis_change_probs[oldState.visibility[p]]))(rng);
        else {
            std::vector<int> non_zero;
            for(int i = 0; i < VIS_LEVELS; i++) {
                if(vis_change_probs[oldState.visibility[p]][i] > 0)
                    non_zero.push_back(i);
            }
            int decision = getDecisionPoint(decision_point, 0, non_zero.size()-1, decision_outcomes);
            newState->visibility[p] = non_zero[decision];
        }
        outcomeProb *= vis_change_probs[oldState.visibility[p]][newState->visibility[p]];
        if(oldState.is_target[p] && take_image && oldState.focal_point == static_cast<int>(p)){
            bool failed_image = (decision_outcomes == nullptr && std::bernoulli_distribution(failure_probs[oldState.visibility[p]])(rng)) ||
                                (decision_outcomes != nullptr && (failure_probs[oldState.visibility[p]] == 1 || (failure_probs[oldState.visibility[p]] > 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))));
            outcomeProb *= failed_image ? failure_probs[oldState.visibility[p]] : 1 - failure_probs[oldState.visibility[p]];
            newState->is_target[p] = failed_image;
        }
        num_targets += oldState.is_target[p];
    }

    double reward = -num_targets - take_image - (action==2? 0:1);
    std::vector<double> rew(1, reward);
    return std::make_pair(rew, outcomeProb);
}

