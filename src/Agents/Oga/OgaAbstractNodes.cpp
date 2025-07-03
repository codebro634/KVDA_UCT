#include <random>
#include <ranges>

#include "../../../include/Agents/Oga/OgaGroundNodes.h"
#include "../../../include/Agents/Oga/OgaUtils.h"
#include "../../../include/Agents/Oga/OgaAbstractNodes.h"

#include <cassert>
#include <fstream>
#include <map>
#include <set>

using namespace OGA;

unsigned OgaAbstractNode::getId() const{
    return id;
}

unsigned OgaAbstractNode::getDepth() const{
    return depth;
}

void* OgaAbstractNode::popAndSetKey(void* key){
    void* temp = this->key;
    this->key = key;
    return temp;
}

unsigned OgaAbstractNode::getCount() const{
    return count;
}

void OgaAbstractNode::increaseCount(){
    count++;
}

void OgaAbstractNode::decreaseCount(){
    assert(count > 0);
    count--;
}

bool OgaAbstractNode::operator==(const OgaAbstractNode& other) const{
    return id == other.id && depth == other.depth;
}

size_t OgaAbstractNode::hash() const{
    return std::hash<unsigned>{}(id) ^ std::hash<unsigned>{}(depth);
}

// OgaAbstractStateNode
void OgaAbstractStateNode::transfer(OgaStateNode* state_node, OgaAbstractStateNode* from, OgaAbstractStateNode* to, AbsStateSet& abstract_state_nodes){
    if (from != to) {
        abstract_state_nodes.erase(to);
        to->add(state_node);
        abstract_state_nodes.insert(to);
        if (from != nullptr) {
            abstract_state_nodes.erase(from);
            from->remove(state_node);
            if (from->getCount() > 0)
                abstract_state_nodes.insert(from);
        }
        state_node->setAbstractNode(to);
    }
}

OgaStateNode* OgaAbstractStateNode::getRepresentant() const{
    return representant;
}

void OgaAbstractStateNode::setRepresentant(OgaStateNode* representant){
    this->representant = representant;
}

void OgaAbstractStateNode::add(OgaStateNode* state_node){
    ground_nodes.insert(state_node);
    if (getCount() == 0)
        setRepresentant(state_node);
    increaseCount();
}

void OgaAbstractStateNode::remove(OgaStateNode* state_node){
    assert (ground_nodes.contains(state_node));
    ground_nodes.erase(state_node);
    if (state_node == getRepresentant()) {
        OgaStateNode *representant = nullptr;
        unsigned min_id = std::numeric_limits<unsigned>::max();
        for (auto* other_node : ground_nodes) { //Min id for reproducibility
            if (other_node->getId() < min_id || representant == nullptr) {
                min_id = other_node->getId();
                representant = other_node;
            }
        }
        setRepresentant(representant);
    }
    decreaseCount();
}


// OgaAbstractQStateNode

void OgaAbstractQStateNode::setRepresentant(OgaQStateNode* representant)
{
    this->representant = representant;
}

OgaQStateNode* OgaAbstractQStateNode::getRepresentant() const
{
    return representant;
}

void OgaAbstractQStateNode::addExperience(double values){
    this->values += values;
    this->squared_values += values * values;
    visits++;
}

double OgaAbstractQStateNode::getValues() const{
    return values;
}

double OgaAbstractQStateNode::getSquaredValues() const{
    return squared_values;
}

double OgaAbstractQStateNode::getVisits() const{
    return visits;
}

void OgaAbstractQStateNode::transfer(OgaQStateNode* q_state_node, OgaAbstractQStateNode* from,OgaAbstractQStateNode* to, AbsQSet& abs_q_set,OgaBehaviorFlags& flags){

    if (from != to) {
        abs_q_set.erase(to);
        to->add(q_state_node, flags);
        abs_q_set.insert(to);
        if (from != nullptr) {
            abs_q_set.erase(from);
            from->remove(q_state_node, flags);
            if (from->getCount() > 0){
                abs_q_set.insert(from);
            }
        }
        q_state_node->setAbstractNode(to);
        if (flags.smart_reward_handling)
            q_state_node->setQstarDistToRepr(q_state_node->calcQstarDistToNode(to->getRepresentant()));
    }

}

void OgaAbstractQStateNode::add(OgaQStateNode* q_state_node,OgaBehaviorFlags& flags){
    assert (!ground_nodes.contains(q_state_node));
    ground_nodes.insert(q_state_node);
    if (getCount() == 0)
        setRepresentant(q_state_node);
    assert (getRepresentant() != nullptr);
    visits += q_state_node->getVisits();
    values += q_state_node->getValues() + (flags.smart_reward_handling? (q_state_node->calcQstarDistToNode(getRepresentant()) * q_state_node->getVisits()) : 0);
    squared_values += q_state_node->getSquaredValues();
    increaseCount();
}

void OgaAbstractQStateNode::remove(OgaQStateNode* q_state_node,OgaBehaviorFlags& flags){
    assert (ground_nodes.contains(q_state_node));

    if (q_state_node == getRepresentant()) {
        OgaQStateNode *representant = nullptr;
        unsigned min_id = std::numeric_limits<unsigned>::max();
        for (auto* other_node : ground_nodes) { //Min id for reproducibility
            if (q_state_node != other_node && (other_node->getId() < min_id || representant == nullptr)) {
                min_id = other_node->getId();
                representant = other_node;
            }
        }

        setRepresentant(representant);
        if (representant != nullptr && flags.smart_reward_handling){
            for (auto* q_state_node : ground_nodes)
                q_state_node->recalcQstarDistToRepr();
        }
    }

    visits -= q_state_node->getVisits();
    values -= q_state_node->getValues() + (flags.smart_reward_handling? (q_state_node->getQstarDistToRepr() * q_state_node->getVisits()) : 0);
    squared_values -= q_state_node->getSquaredValues();
    decreaseCount();
    ground_nodes.erase(q_state_node);

}

void OgaAbstractQStateNode::testValueConsistency() const{
    double vsum = 0;
    for (auto* q_state_node : ground_nodes) {
        vsum += q_state_node->getValues() + q_state_node->getQstarDistToRepr() * q_state_node->getVisits();
    }
    if (std::fabs(vsum - values) > 1e-5)
        throw std::runtime_error("Inconsistent values in abstract node: " + std::to_string(vsum) + " != " + std::to_string(values));
}