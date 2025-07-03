#include <random>
#include <ranges>

#include "../../../include/Agents/Oga/OgaUtils.h"
#include "../../../include/Agents/Oga/OgaAbstractNodes.h"

#include <cassert>
#include <fstream>

using namespace OGA;

NextDistribution::NextDistribution(double rewards, bool consider_missing_outcomes, bool ignore_reward_diffs) :
    rewards(rewards),
    consider_missing_outcomes(consider_missing_outcomes),
    ignore_reward_diffs(ignore_reward_diffs)
{}

void NextDistribution::addProbability(OgaAbstractStateNode* next_abstract_state_node, const double probability){
    if (!distribution.contains(next_abstract_state_node))
        distribution[next_abstract_state_node] = probability;
    else
        distribution[next_abstract_state_node] += probability;
}

double NextDistribution::getRewards() const{
    return rewards;
}

const Map<OgaAbstractStateNode, double>& NextDistribution::getDistribution() const{
    return distribution;
}

inline double NextDistribution::rewardDist(const NextDistribution* other) const{
    return ignore_reward_diffs? 0 : std::fabs(rewards - other->getRewards());
}

inline double NextDistribution::transDist(const NextDistribution* other) const {

    Map<OgaAbstractStateNode,std::pair<bool,bool>> union_map;
    for(const auto &abs_node: other->getDistribution() | std::views::keys)
        union_map.insert({abs_node,{false,true}});
    for(const auto &abs_node: distribution | std::views::keys) {
        if(union_map.contains(abs_node))
            union_map.at(abs_node).first = true;
        else
            union_map.insert({abs_node,{true,false}});
    }

    // double trans_dist = 0;
    // double missing_p1 = 1.0;
    // double missing_p2 = 1.0;
    // for(auto [abs_node,occurences] : union_map) {
    //     double p1 = occurences.first ? distribution.at(abs_node) : 0;
    //     double p2 = occurences.second ? other->getDistribution().at(abs_node) : 0;
    //     trans_dist += std::fabs(p1 - p2);
    //     missing_p1 -= p1;
    //     missing_p2 -= p2;
    // }

    double trans_dist = 0;
    double missing_p1 = 1.0;
    double missing_p2 = 1.0;
    std::set<std::tuple<double,double,int>> probs = {}; //Doing it this way it a bit slower, but we want reproducibility and iterating over union_map is undefined order
    int ctr = 0;
    for(auto [abs_node,occurences] : union_map) {
        double p1 = occurences.first ? distribution.at(abs_node) : 0;
        double p2 = occurences.second ? other->getDistribution().at(abs_node) : 0;
        probs.insert({p1,p2, ctr++});
    }
    for (auto [p1,p2,ctr] : probs){
        trans_dist += std::fabs(p1 - p2);
        missing_p1 -= p1;
        missing_p2 -= p2;
    }

    if (consider_missing_outcomes)
        trans_dist += missing_p1 + missing_p2;

    return trans_dist;
}

double NextDistribution::dist(const NextDistribution* other) const {
    return std::max(rewardDist(other), transDist(other));
}

bool NextDistribution::approxEqual(const NextDistribution* other, double eps_a, double eps_t) const {
    return std::max(rewardDist(other) -  eps_a, 0.0) < 1e-6 && std::max(transDist(other) - eps_t, 0.0) < 1e-6;
}

// NextAbstractQStates
void NextAbstractQStates::addAbstractQState(OgaAbstractQStateNode* next_abstract_q_state_node){
    states.insert(next_abstract_q_state_node);
}

const Set<OgaAbstractQStateNode>& NextAbstractQStates::getStates() const{
    return states;
}

bool NextAbstractQStates::operator==(const NextAbstractQStates& other) const{
    return states == other.getStates();
}

size_t NextAbstractQStates::hash() const{
    size_t hash = 0;
    for (const auto* next_abstract_q_state_node : states)
        hash ^= next_abstract_q_state_node->hash();
    return hash;
}
