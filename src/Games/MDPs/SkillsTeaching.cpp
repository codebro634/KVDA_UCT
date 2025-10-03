#include "../../../include/Games/MDPs/SkillsTeaching.h"
#include <fstream>
#include <sstream>
#include <cmath>

using namespace ST;

std::vector<int> SkillsTeachingModel::obsShape() const {
    return {6*(int)prerequisites.size()};
}

void SkillsTeachingModel::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto state = dynamic_cast<SkillsTeachingState*>(uncasted_state);
    for(size_t i=0;i<state->proficiencyMed.size();i++){
        obs[i] = state->proficiencyMed[i];
        obs[i+state->proficiencyMed.size()] = state->proficiencyHigh[i];
        obs[i+2*state->proficiencyMed.size()] = state->answeredRight[i];
        obs[i+3*state->proficiencyMed.size()] = state->hintedRight[i];
        obs[i+4*state->proficiencyMed.size()] = state->hintDelayVar[i];
        obs[i+5*state->proficiencyMed.size()] = state->updateTurn[i];
    }
}

[[nodiscard]] std::vector<int> SkillsTeachingModel::actionShape() const {
    return {(int)(2*skillWeights.size()) + 1};
}

int SkillsTeachingModel::encodeAction(int* decoded_action) {
    return decoded_action[0] - 1;
}

bool SkillsTeachingState::operator==(const ABS::Gamestate& o) const {
    const auto* other = dynamic_cast<const SkillsTeachingState*>(&o);
    if(!other) return false;
    return proficiencyMed == other->proficiencyMed && proficiencyHigh == other->proficiencyHigh && answeredRight == other->answeredRight &&
        hintedRight == other->hintedRight && hintDelayVar == other->hintDelayVar && updateTurn == other->updateTurn;
}

size_t SkillsTeachingState::hash() const {
    int hash = 0;
    for(bool b : proficiencyMed) hash = (hash << 1) | b;
    for(bool b : proficiencyHigh) hash = (hash << 1) | b;

    int turn_hash = 0;
    for(bool b : updateTurn) turn_hash = turn_hash | b;
    hash = (hash << 1) | turn_hash;

    int action_hash = 0;
    for(size_t i=0;i<answeredRight.size();i++){
        int sum = 0;
        if(answeredRight[i]) sum ++;
        if(hintedRight[i]) sum ++;
        if(hintDelayVar[i]) sum ++;
        action_hash = (action_hash << 2) | sum;
    }

    return (hash << 8) | action_hash;
}

SkillsTeachingModel::SkillsTeachingModel(const std::string& filePath, bool idle_action, bool reduced_action_space){
    this->reduced_action_space = reduced_action_space;

    std::ifstream in(filePath);
    if(!in.is_open()){
        std::cerr << "Could not open file: " << filePath << std::endl;
        exit(1);
    }

    int nSkills, m;
    in >> nSkills >> m;
    prerequisites.resize(nSkills, std::vector<int>());
    for(int i=0;i<m;i++){
        int p,c;
        in >> p >> c;
        prerequisites[c].push_back(p);
    }
    skillWeights.resize(nSkills);
    highProb.resize(nSkills);
    prob_all_pre.resize(nSkills);
    prob_all_pre_med.resize(nSkills);
    prob_per_pre.resize(nSkills);
    prob_per_pre_med.resize(nSkills);
    lose_prob.resize(nSkills);
    for(int i=0;i<nSkills;i++){
        double x1,x2,x3,x4,x5,x6,x7;
        in >> x1 >> x2 >> x3 >> x4 >> x5 >> x6 >> x7 ;
        skillWeights[i] = x1;
        highProb[i] = x2;
        prob_all_pre[i] = x3;
        prob_all_pre_med[i] = x4;
        prob_per_pre[i] = x5;
        prob_per_pre_med[i] = x6;
        lose_prob[i] = x7;
    }

    for(int i=0;i<2*nSkills;i++){
        actions.push_back(i);
    }

    idling_allowed = idle_action;
    if(idle_action)
        actions.push_back(-1);

}

void SkillsTeachingModel::printState(ABS::Gamestate* uncasted_state){
    auto* s = dynamic_cast<SkillsTeachingState*>(uncasted_state);

    for(size_t i=0;i<prerequisites.size();i++){
        std::cout << "Skill " << i << " Prerequisites: ";
        for(int p : prerequisites[i]){
            std::cout << p << " ";
        }
        std::cout << std::endl;
    }

    for(size_t i=0;i<prerequisites.size();i++){
        std::cout << "Skill " << i << " Proficiency: ";
        if(s->proficiencyHigh[i]) std::cout << "High ";
        else if(s->proficiencyMed[i]) std::cout << "Med ";
        else std::cout << "Low ";
        std::cout << "Answered Right: " << s->answeredRight[i] << " Hinted Right: " << s->hintedRight[i] << " Hint Delay Var: " << s->hintDelayVar[i] << " Update Turn: " << s->updateTurn[i];
        std::cout << std::endl;
    }
}

ABS::Gamestate* SkillsTeachingModel::getInitialState(std::mt19937&){
    auto* s = new SkillsTeachingState();
    s->proficiencyMed.resize(prerequisites.size(),false);
    s->proficiencyHigh.resize(prerequisites.size(),false);
    s->answeredRight.resize(prerequisites.size(),false);
    s->hintedRight.resize(prerequisites.size(),false);
    s->hintDelayVar.resize(prerequisites.size(),false);
    s->updateTurn.resize(prerequisites.size(),false);
    return s;
}

int SkillsTeachingModel::getNumPlayers(){
    return 1;
}

ABS::Gamestate* SkillsTeachingModel::copyState(ABS::Gamestate* uncasted_state){
    auto* sOld = dynamic_cast<SkillsTeachingState*>(uncasted_state);
    auto* sNew = new SkillsTeachingState();
    *sNew = *sOld;
    return sNew;
}

std::vector<int> SkillsTeachingModel::getActions_(ABS::Gamestate* uncasted_state){
    if(reduced_action_space) {
        auto state = dynamic_cast<SkillsTeachingState*>(uncasted_state);
        bool student_turn = true;
        for(size_t i = 0; i < prerequisites.size(); i++) {
            if(state->updateTurn[i]) {
                student_turn = false;
                break;
            }
        }
        if(student_turn)
            return actions;
        else
            return {-1};
    }else
        return actions;
}

std::pair<std::vector<double>,double> SkillsTeachingModel::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){

    auto newState = dynamic_cast<SkillsTeachingState*>(uncasted_state);
    auto oldState = *newState;
    size_t decision_point = 0;

    double reward=0.0;
    for(size_t i=0;i<prerequisites.size();i++){
        if(oldState.proficiencyHigh[i]) reward += skillWeights[i];
        if(!oldState.proficiencyMed[i]) reward -= skillWeights[i];
    }

    int targeted_skill;
    bool askProb = false;
    if(action == -1)
        targeted_skill = -1;
    else if(action < static_cast<int>(prerequisites.size())) {
        targeted_skill = action;
        askProb = true;
    }
    else
        targeted_skill = action - prerequisites.size();

    double outcomeProb=1.0;

    bool student_turn = true;
    for(size_t i = 0; i < prerequisites.size(); i++) {
        if(oldState.updateTurn[i]) {
            student_turn = false;
            break;
        }
    }

    for(size_t i = 0; i < prerequisites.size(); i++) {

        newState->updateTurn[i] = student_turn  && targeted_skill == static_cast<int>(i);
        bool all_pre_high = true;
        for(int s : prerequisites[i]) {
            if(!oldState.proficiencyHigh[s]) {
                all_pre_high = false;
                break;
            }
        }

        bool answered_right;
        if(student_turn && (askProb && targeted_skill == static_cast<int>(i)) && oldState.proficiencyHigh[i]) {
            std::bernoulli_distribution bernoulli(highProb[i]);
            answered_right = decision_outcomes == nullptr || highProb[i] == 0 || highProb[i] == 1? bernoulli(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
            outcomeProb *= answered_right ? highProb[i] : 1.0 - highProb[i];
        } else if(student_turn && (askProb && targeted_skill == static_cast<int>(i)) && oldState.proficiencyMed[i] && all_pre_high) {
            std::bernoulli_distribution bernoulli(prob_all_pre_med[i]);
            answered_right = decision_outcomes == nullptr || prob_all_pre_med[i] == 0 || prob_all_pre_med[i] == 1? bernoulli(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
            outcomeProb *= answered_right ? prob_all_pre_med[i] : 1.0 - prob_all_pre_med[i];
        } else if(student_turn && (askProb && targeted_skill == static_cast<int>(i)) && oldState.proficiencyMed[i]) {
            double p = prob_per_pre_med[i] * prerequisites[i].size();
            std::bernoulli_distribution bernoulli(p);
            answered_right = decision_outcomes == nullptr || p==1 || p==0? bernoulli(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
            outcomeProb *= answered_right ? p : (1.0 - p);
        } else if(student_turn && (askProb && targeted_skill == static_cast<int>(i)) && all_pre_high) {
            std::bernoulli_distribution bernoulli(prob_all_pre[i]);
            answered_right = decision_outcomes == nullptr || prob_all_pre[i] == 0 || prob_all_pre[i] == 1? bernoulli(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
            outcomeProb *= answered_right ? prob_all_pre[i] : 1.0 - prob_all_pre[i];
        } else if(student_turn && (askProb && targeted_skill == static_cast<int>(i))) {
            double p = prob_per_pre[i] * prerequisites[i].size();
            std::bernoulli_distribution bernoulli(p);
            answered_right =decision_outcomes == nullptr || p == 0 || p == 1? bernoulli(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
            outcomeProb *= answered_right ? p : (1.0 - p);
        }else {
            answered_right = false;
        }
        newState->answeredRight[i] = answered_right;

        newState->hintedRight[i] = student_turn && (!askProb && targeted_skill == static_cast<int>(i)) && all_pre_high;
        newState->hintDelayVar[i] = student_turn && (!askProb && targeted_skill == static_cast<int>(i));

        bool prof_med;
        if(student_turn)
            prof_med = oldState.proficiencyMed[i];
        else if( (oldState.updateTurn[i] && oldState.hintedRight[i]) || (oldState.updateTurn[i] && oldState.answeredRight[i])
            || (oldState.proficiencyHigh[i]) || (oldState.proficiencyMed[i] && oldState.updateTurn[i] && oldState.hintDelayVar[i]))
            prof_med = true;
        else
            prof_med = false;
        newState->proficiencyMed[i] = prof_med;

        bool prof_high;
        if(student_turn)
            prof_high = oldState.proficiencyHigh[i];
        else if(!oldState.updateTurn[i] && oldState.proficiencyHigh[i]) {
            std::bernoulli_distribution bernoulli(1.0 - lose_prob[i]);
            prof_high = decision_outcomes == nullptr || lose_prob[i] == 0 || lose_prob[i] == 1? bernoulli(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
            outcomeProb *= prof_high ? (1.0 - lose_prob[i]) : lose_prob[i];
        }else if( (oldState.proficiencyMed[i] && oldState.updateTurn[i] && oldState.answeredRight[i]) ||
            (oldState.proficiencyHigh[i] && oldState.updateTurn[i] && (oldState.hintDelayVar[i] || oldState.answeredRight[i])))
            prof_high = true;
        else
            prof_high = false;
        newState->proficiencyHigh[i] = prof_high;

    }



    std::vector<double> rew(1, reward);
    return std::make_pair(rew, outcomeProb);
}

