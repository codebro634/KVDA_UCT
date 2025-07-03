#include <random>
#include <ranges>

#include "../../../include/Agents/Oga/OgaGroundNodes.h"
#include "../../../include/Agents/Oga/OgaUtils.h"
#include "../../../include/Agents/Oga/OgaAbstractNodes.h"

#include <cassert>
#include <fstream>
#include <algorithm>
#include <cstring>

#include "../../../include/Utils/Distributions.h"

using namespace OGA;

OgaStateNode::OgaStateNode(ABS::Gamestate* state, const unsigned depth, OgaSearchStats& search_stats

)
: id(search_stats.max_state_id++), state(state), depth(depth)
{}

OgaStateNode::~OgaStateNode(){
    delete state;
}

ABS::Gamestate* OgaStateNode::getState() const{
    return state;
}

ABS::Gamestate* OgaStateNode::getStateCopy(ABS::Model* model) const{
    return model->copyState(state);
}

unsigned OgaStateNode::getDepth() const{
    return depth;
}

bool OgaStateNode::isTerminal() const{
    return state->terminal;
}

void OgaStateNode::addVisit(){
    visits++;
}

unsigned OgaStateNode::getVisits() const{
    return visits;
}

void OgaStateNode::initUntriedActions(ABS::Model* model, const std::vector<int>& actions, std::mt19937& rng){
    untried_actions = actions;

    if (!policyProbsAvailable(model))
        std::ranges::shuffle(untried_actions.begin(), untried_actions.end(), rng);
    else{
        //sort untried actions by policy scores
        std::sort(untried_actions.begin(), untried_actions.end(), [this](int a, int b){
            return policy_probs[a] < policy_probs[b];
        });
    }
}

int OgaStateNode::popUntriedAction(){
    assert(!untried_actions.empty());
    int a = untried_actions.back();
    untried_actions.pop_back();
    tried_actions.push_back(a);
    return a;
}

bool OgaStateNode::isFullyExpanded() const{
    return untried_actions.empty();
}

bool OgaStateNode::isPartiallyExpanded() const{
    return !tried_actions.empty();
}

unsigned OgaStateNode::numTriedActions() const{
    return tried_actions.size();
}

unsigned OgaStateNode::numUntriedActions() const{
    return untried_actions.size();
}

void OgaStateNode::addChild(OgaQStateNode* child){
    children.push_back(child);
}

const std::vector<OgaQStateNode*>& OgaStateNode::getChildren() const{
    return children;
}

void OgaStateNode::setAbstractNode(OgaAbstractStateNode* abstract_node){
    this->abstract_node = abstract_node;
}

OgaAbstractStateNode* OgaStateNode::getAbstractNode() const{
    return abstract_node;
}

const Set<OgaQStateNode>& OgaStateNode::getParents() const{
    return parents;
}

void OgaStateNode::setTrajectoryParent(OgaQStateNode* parent){
    assert(trajectory_parent == nullptr);
    trajectory_parent = parent;
    parents.insert(parent);
}

OgaQStateNode* OgaStateNode::popTrajectoryParent(){
    auto* parent = trajectory_parent;
    trajectory_parent = nullptr;
    return parent;
}


bool OgaStateNode::operator==(const OgaStateNode& other) const{
    return *state == *other.state && depth == other.depth;
}

size_t OgaStateNode::hash() const{
    return state->hash() ^ std::hash<unsigned>{}(depth);
}

// OgaQStateNode

OgaQStateNode::OgaQStateNode(ABS::Gamestate* state, const unsigned depth, const int action, OgaSearchStats& search_stats) : id(search_stats.max_q_id++), state(state),
    depth(depth), action(action)
{}

OgaQStateNode::~OgaQStateNode(){
    delete state;
    for (const auto& state : children | std::views::keys)
        delete state;
}

ABS::Gamestate *OgaQStateNode::getState() const{
    return state;
}

unsigned OgaQStateNode::getDepth() const{
    return depth;
}

int OgaQStateNode::getAction() const{
    return action;
}

unsigned OgaQStateNode::getVisits() const{
    return visits;
}

double OgaQStateNode::getValues() const{
    return values;
}

double OgaQStateNode::getSquaredValues() const{
    return squared_values;
}

double OgaQStateNode::getAbsVisits() const{
    return abstract_node->getVisits();
}

double OgaQStateNode::getAbsValues() const{
    return abstract_node->getValues() - qstart_dist_to_repr * abstract_node->getVisits();
}

void OgaQStateNode::addExperience(double values){
    getAbstractNode()->addExperience(values + qstart_dist_to_repr);
    this->values += values;
    this->squared_values += values * values;
    visits++;
}

bool OgaQStateNode::addChild(ABS::Gamestate* state, const double probability, OgaStateNode* child){
    if (!children.contains(state)) {
        children[state] = {probability, child};
        prob_sum += probability;
        return true;
    }else
        return false;
}

const Map<ABS::Gamestate, std::pair<double, OgaStateNode*>>* OgaQStateNode::getChildren() const{
    return &children;
}

void OgaQStateNode::setAbstractNode(OgaAbstractQStateNode* abstract_node){
    this->abstract_node = abstract_node;
}

OgaAbstractQStateNode* OgaQStateNode::getAbstractNode() const{
    return abstract_node;
}

void OgaQStateNode::setRewards(std::vector<double>& rewards){
    this->rewards = rewards;
}

double OgaQStateNode::getRewards(int player) const{
    return rewards[player];
}

void OgaQStateNode::addRecencyCount(){
    recency_count++;
}

void OgaQStateNode::resetRecencyCount(){
    recency_count = 0;
}

unsigned OgaQStateNode::getRecencyCount() const{
    return recency_count;
}

void OgaQStateNode::setParent(OgaStateNode* parent){
    assert(this->parent == nullptr);
    this->parent = parent;
}

OgaStateNode* OgaQStateNode::getParent() const{
    return parent;
}


bool OgaQStateNode::operator==(const OgaQStateNode& other) const{
    return *state == *other.state && depth == other.depth && action == other.action;
}

size_t OgaQStateNode::hash() const{
    return state->hash() ^ std::hash<unsigned>{}(depth) ^ std::hash<int>{}(action);
}

bool OgaStateNode::policyProbsAvailable(ABS::Model* model){
    return false;
}



/**
 * For smart reward handling
 */

void OgaQStateNode::recalcQstarDistToRepr() {
    double old_dist = qstart_dist_to_repr;
    qstart_dist_to_repr = calcQstarDistToNode(getAbstractNode()->getRepresentant());
    if (std::fabs (old_dist - qstart_dist_to_repr) > 1e-5)
        getAbstractNode()->addValues( (qstart_dist_to_repr-old_dist) * getVisits());
}

double OgaQStateNode::calcQstarDistToNode(const OgaQStateNode* other) const{
    if (other == this)
        return 0;

    double d = other->getRewards(state->turn) - getRewards(state->turn);
    for (auto child : other->children)
        d -= child.second.first * child.second.second->getVstarDistToRepr();
    for (const auto& child : children)
        d += child.second.first * child.second.second->getVstarDistToRepr();

    return d;
}

double OgaStateNode::consistentVstarDist(OgaStateNode* s1, OgaStateNode* s2, bool *consistent){


    /**
     * TEMP
     **/
    // if (s1 == s2) {
    //     *consistent = true;
    //     return 0;
    // }
    // for (auto child : s1->children) {
    //     if (std::fabs(child->getQstarDistToRepr()) > 1e-5) {
    //         *consistent = false;
    //         return 0;
    //     }
    // }
    // for (auto child : s2->children) {
    //     if (std::fabs(child->getQstarDistToRepr()) > 1e-5) {
    //         *consistent = false;
    //         return 0;
    //     }
    // }
    // *consistent = true;
    // return 0;
    /**
     *  END TEMP
     **/


    if (s1 == s2){
        *consistent = true;
        return 0;
    }

    std::map<OgaAbstractQStateNode*,double> self_dist_map = {};
    for (auto child : s1->children){
        auto* abs = child->getAbstractNode();
        if (self_dist_map.contains(abs) && std::fabs(self_dist_map.at(abs) - child->getQstarDistToRepr()) > 1e-5){
            *consistent = false;
            return 0;
        }
        self_dist_map[abs] = child->getQstarDistToRepr();
    }

    std::map<OgaAbstractQStateNode*,double> other_dist_map = {};
    OgaAbstractQStateNode* last_abs;
    for (auto child : s2->children){
        last_abs = child->getAbstractNode();
        if (other_dist_map.contains(last_abs) && std::fabs(other_dist_map.at(last_abs) - child->getQstarDistToRepr()) > 1e-5){
            *consistent = false;
            return 0;
        }
        other_dist_map[last_abs] = child->getQstarDistToRepr();
    }

    double offset = self_dist_map[last_abs] - other_dist_map[last_abs];
    for (const auto& [abs_node, dist] : other_dist_map){
        double difference = self_dist_map[abs_node] - dist;
        if (std::fabs(difference - offset) > 1e-5){
            *consistent = false;
            return 0;
        }
    }
    *consistent = true;

    return offset;
}

void OgaStateNode::setVstarDistToRepr(double dv) {
    vstar_dist_to_repr = dv;
}
