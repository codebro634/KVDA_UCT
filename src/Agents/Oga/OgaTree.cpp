#include <random>
#include <ranges>

#include "../../../include/Agents/Oga/OgaGroundNodes.h"
#include "../../../include/Agents/Oga/OgaTree.h"
#include "../../../include/Agents/Oga/OgaUtils.h"
#include "../../../include/Agents/Oga/OgaAbstractNodes.h"
#include "../../../include/Utils/Distributions.h"

#include <cassert>
#include <fstream>
#include <set>
#include <algorithm>

#include "../../../include/Agents/Oga/OgaAgent.h"
#include "../../../include/Utils/Argparse.h"

using namespace OGA;


OgaTree::OgaTree(
    ABS::Gamestate* root_state,
    ABS::Model* model,
    const OgaBehaviorFlags& behavior_flags,
    std::mt19937& rng,
    OgaSearchStats& search_stats
) : behavior_flags(behavior_flags), model(model)
{
    auto [root, found] = findOrCreateState(root_state, 0, rng, search_stats
    );
    assert(!found);
    this->root = root;
}

OgaTree::~OgaTree(){

    for (const auto* state_node : d_states) {
        delete state_node->getLastFilteredNextDistr();
        delete state_node->getLastNextDistr();
        delete state_node;
    }
    for (const auto* q_state_node : q_states)
        delete q_state_node;

    for (const auto& next_distribution_map : next_distribution_map){
        for (const auto* next_distribution : next_distribution_map | std::views::values)
            delete next_distribution;
    }

    for (const auto& abstract_state_node_map : abstract_state_node_map){
        for (const auto* next_abstract_q_states : abstract_state_node_map | std::views::keys)
            delete next_abstract_q_states;
    }

    for (const auto &val: abstract_state_nodes | std::views::values) {
        for (const auto* abstract_node : val){
            assert (abstract_node->getCount() > 0);
            delete abstract_node;
        }
    }

    for (auto corpse : abstract_q_corpses)
        delete corpse;

    for (auto corpse : abstract_state_corpses)
        delete corpse;

    for (const auto &val: abstract_q_state_nodes | std::views::values){
        for (const auto* abstract_node : val){
            assert (abstract_node->getCount() > 0);
            delete abstract_node;
        }
    }
}

double OgaTree::getCompressionRate() const{

    int total_abs_nodes = 0;
    for (const auto& val: abstract_state_nodes | std::views::values)
        total_abs_nodes += val.size();
    double state_compression = static_cast<double>(d_states.size()) / static_cast<double>(total_abs_nodes);

    int total_abs_q_nodes = 0;
    for (const auto& val: abstract_q_state_nodes | std::views::values)
        total_abs_q_nodes += val.size();
    double q_state_compression = static_cast<double>(q_states.size()) / static_cast<double>(total_abs_q_nodes);

    return std::max(state_compression, q_state_compression);
}

OgaStateNode* OgaTree::getRoot() const
{
    return root;
}

std::pair<OgaStateNode*, bool> OgaTree::findOrCreateState(ABS::Gamestate* state, const unsigned depth, std::mt19937& rng, OgaSearchStats& search_stats
)
{
    auto* state_node = new OgaStateNode(model->copyState(state), depth, search_stats
        );

    if (const auto it = d_states.find(state_node); it != d_states.end()){
        // State already exists: Delete the new state node and return the existing one
        delete state_node;
        return {*it, true};
    }

    // Initialize state abstraction
    OgaAbstractStateNode* abstract_state_node;
    if (state->terminal && behavior_flags.group_terminal_states){
        // Use one terminal abstract state node per depth
        // Resize terminal abstract state nodes if necessary and create terminal abstract state node
        if (terminal_abstract_state_nodes.size() <= depth)
            terminal_abstract_state_nodes.resize(depth + 1);
        if (terminal_abstract_state_nodes[depth] == nullptr){
            terminal_abstract_state_nodes[depth] = new OgaAbstractStateNode(depth, search_stats);
            abstract_state_nodes[static_cast<int>(depth)].insert(terminal_abstract_state_nodes[depth]);
        }

        abstract_state_node = terminal_abstract_state_nodes[depth];
    }
    else if (behavior_flags.group_partially_expanded_states)
    {
        if (unexplored_abstract_state_nodes.size() <= depth)
            unexplored_abstract_state_nodes.resize(depth + 1);
        if (unexplored_abstract_state_nodes[depth] == nullptr){
            unexplored_abstract_state_nodes[depth] = new OgaAbstractStateNode(depth, search_stats);
            abstract_state_nodes[static_cast<int>(depth)].insert(unexplored_abstract_state_nodes[depth]);
        }

        abstract_state_node = unexplored_abstract_state_nodes[depth];
    }
    else{
        abstract_state_node = new OgaAbstractStateNode(depth, search_stats);
        abstract_state_nodes[static_cast<int>(depth)].insert(abstract_state_node);
    }

    // Init node and insert into tree
    OgaAbstractStateNode::transfer(state_node, nullptr, abstract_state_node, abstract_state_nodes[static_cast<int>(depth)]);
    state_node->initUntriedActions(model, state->terminal ? std::vector<int>{} : model->getActions(state), rng);
    d_states.insert(state_node);

    return {state_node, false};
}

std::pair<OgaQStateNode*, bool> OgaTree::findOrCreateQState(ABS::Gamestate* state, const unsigned depth,const int action, std::mt19937& rng, OgaSearchStats& search_stats
)
{
    auto* q_state_node = new OgaQStateNode(model->copyState(state), depth, action, search_stats);

    if (const auto it = q_states.find(q_state_node); it != q_states.end()){
        // Q-state already exists: Delete the new q-state node and return the existing one
        delete q_state_node;
        return {*it, true};
    }

    // Initialize q-state abstraction
    auto* abstract_q_state_node = new OgaAbstractQStateNode(q_state_node->getDepth(), search_stats);
    OgaAbstractQStateNode::transfer(q_state_node, nullptr, abstract_q_state_node, abstract_q_state_nodes[static_cast<int>(depth)],behavior_flags);

    // Init q state node and insert into tree
    auto [parent, found] = findOrCreateState(state, depth, rng, search_stats
    );
    assert(found);
    parent->addChild(q_state_node);
    q_states.insert(q_state_node);

    return {q_state_node, false};
}

void OgaTree::insert(OgaStateNode* state_node){
    d_states.insert(state_node);
}

void OgaTree::insert(OgaQStateNode* state_node){
    q_states.insert(state_node);
}

void OgaTree::_stageForUpdate(OgaStateNode* state_node){
    auto depth = state_node->getDepth();
    if (to_update_states.size() <= depth){
        to_update_states.resize(depth + 1);
        abstract_state_node_map.resize(depth + 1);
    }
    to_update_states[depth].insert(state_node);
}
void OgaTree::_stageForUpdate(OgaQStateNode* q_state_node){
    auto depth = q_state_node->getDepth();
    if (to_update_q_states.size() <= depth){
        to_update_q_states.resize(depth + 1);
        next_distribution_map.resize(depth + 1);
    }
    to_update_q_states[depth].insert(q_state_node);
}

void OgaTree::updateQAbstractions(unsigned K, int depth, std::mt19937& rng, OgaSearchStats& search_stats) {

    //Sort by id for reproducibility (for eps > 0 case this might make a difference)
    std::vector<OgaQStateNode*> sorted_to_update_nodes = std::vector(to_update_q_states[depth].begin(), to_update_q_states[depth].end());
    std::sort(sorted_to_update_nodes.begin(), sorted_to_update_nodes.end(),
              [](const OgaQStateNode* lhs, const OgaQStateNode* rhs) {
                  return lhs->getId() < rhs->getId();  // Sort by ID
              });

    //Update the abstraction
    for (auto* q_state_node : sorted_to_update_nodes){

        double old_repr_dist = q_state_node->getQstarDistToRepr();

        // Calculate Successor distribution and update Certainty
        double threshold = 0;
        if (behavior_flags.alpha > 0) {
            for (const auto& probability_successor : *q_state_node->getChildren() | std::views::values) {
                auto& [probability, successor] = probability_successor;
                threshold = probability*behavior_flags.alpha > threshold ? probability*behavior_flags.alpha : threshold;
            }
        }

        auto* next_distribution = new NextDistribution(q_state_node->getRewards(q_state_node->getState()->turn), behavior_flags.consider_missing_outcomes, behavior_flags.smart_reward_handling);
        double psum =0;
        for (const auto& probability_successor : *q_state_node->getChildren() | std::views::values){
            auto& [probability, successor] = probability_successor;
            assert (successor->getAbstractNode()->getCount() > 0);
            if (probability >= threshold) {
                psum += probability;
                next_distribution->addProbability(successor->getAbstractNode(), probability);
            }
        }

        // Reset recency count
        q_state_node->resetRecencyCount();
        OgaAbstractQStateNode* old_abstract_q_state_node = q_state_node->getAbstractNode();
        assert (old_abstract_q_state_node != nullptr && old_abstract_q_state_node->getCount() > 0);
        assert (old_abstract_q_state_node->getRepresentant() != nullptr);
        OgaAbstractQStateNode* new_abstract_q_state_node = old_abstract_q_state_node;

        // Determine new abstract node
        assert (behavior_flags.q_abs_alg == "random" || behavior_flags.q_abs_alg == "eps");
        if (behavior_flags.q_abs_alg == "random"){
            if (!q_state_node->hasReceivedAbsUpdate() && q_state_node->getAbstractNode()->getCount() <= 1){
                if (std::bernoulli_distribution(behavior_flags.equiv_chance)(rng)){
                        auto it = abstract_q_state_nodes[depth].begin();
                        std::advance(it, std::uniform_int_distribution<size_t>(0, abstract_q_state_nodes[depth].size() - 1)(rng));
                        new_abstract_q_state_node = *it;
                }
            }
            delete next_distribution;
        }
        else {

            [[maybe_unused]] bool contains_rep = next_distribution_map[depth].contains(old_abstract_q_state_node->getRepresentant());
            assert (contains_rep || old_abstract_q_state_node->getRepresentant() == q_state_node); //if !contains_rep then fresh q-node
            if (old_abstract_q_state_node->getRepresentant() == q_state_node && contains_rep) { //Try merging with bigger abs nodes.

               //Find biggest abs node that current abs node can merge with
                OgaAbstractQStateNode* merge = nullptr;
                for (auto it = abstract_q_state_nodes[depth].rbegin() ; it != abstract_q_state_nodes[depth].rend(); ++it){ //sorted by abs size
                    auto abs_other_node = *it;
                    if (!next_distribution_map[depth].contains(abs_other_node->getRepresentant()))
                        continue;
                    assert (abs_other_node->getRepresentant() != nullptr && abs_other_node->getCount() > 0 && abs_other_node->getRepresentant()->getAbstractNode() == abs_other_node);
                    bool approx = next_distribution->approxEqual(next_distribution_map[depth].at(abs_other_node->getRepresentant()), behavior_flags.eps_a, behavior_flags.eps_t);
                    if (approx) {
                        merge = abs_other_node;
                        break;
                    }
                }

                if (merge != nullptr && merge != old_abstract_q_state_node) {
                    new_abstract_q_state_node = merge;
                }else if (merge == nullptr) { //Create new abstract node
                    if (old_abstract_q_state_node->getCount() > 1)
                        new_abstract_q_state_node = new OgaAbstractQStateNode(depth, search_stats);
                    else// Recycle old state
                        new_abstract_q_state_node = old_abstract_q_state_node;
                }

            }
            else if ( !contains_rep || !next_distribution->approxEqual(next_distribution_map[depth].at(old_abstract_q_state_node->getRepresentant()), behavior_flags.eps_a, behavior_flags.eps_t)) {

                //find new matching abstract node with minimal distance
                double min_dist = std::numeric_limits<double>::infinity();
                unsigned min_id = -1;
                OgaAbstractQStateNode* min_dist_node = nullptr;
                for (auto abs_other_node : abstract_q_state_nodes[depth]) {
                    if ((abs_other_node->getRepresentant() == q_state_node && !contains_rep) || !next_distribution_map[depth].contains(abs_other_node->getRepresentant()))
                        continue;
                    assert (abs_other_node->getRepresentant() != q_state_node && abs_other_node->getRepresentant() != nullptr && abs_other_node->getCount() > 0 && abs_other_node->getRepresentant()->getAbstractNode() == abs_other_node);
                    double dist = next_distribution->dist(next_distribution_map[depth].at(abs_other_node->getRepresentant()));
                    if ( dist < min_dist - 1e-6 || ( std::fabs(dist - min_dist) <= 1e-6 && abs_other_node->getId() < min_id)) {
                        min_dist = dist;
                        min_id = abs_other_node->getId();
                        min_dist_node = abs_other_node;
                    }
                }
                //No match found?
                bool transferrable = min_dist_node != nullptr && next_distribution->approxEqual(next_distribution_map[depth].at(min_dist_node->getRepresentant()), behavior_flags.eps_a, behavior_flags.eps_t);
                bool create_new_abs_node = min_dist != std::numeric_limits<double>::infinity() && !transferrable;

                // Create new abstract node
                if (create_new_abs_node) {
                    if (old_abstract_q_state_node->getCount() > 1)
                        new_abstract_q_state_node = new OgaAbstractQStateNode(depth, search_stats);
                    else // Recycle old state
                       new_abstract_q_state_node = old_abstract_q_state_node;
                } else if (transferrable)
                    new_abstract_q_state_node = min_dist_node;

            }else {
               assert (next_distribution_map.at(depth).contains(q_state_node));
            }

            auto old_distr = next_distribution_map.at(depth)[q_state_node];
            delete old_distr;
            next_distribution_map.at(depth)[q_state_node] = next_distribution;
        }

        q_state_node->setReceivedAbsUpdate(true);
        bool abstract_node_changed = (old_abstract_q_state_node != new_abstract_q_state_node) || (q_state_node->getVisits() == K);
        if (abstract_node_changed) {
            assert (abstract_q_state_nodes[depth].contains(old_abstract_q_state_node));
            assert (old_abstract_q_state_node->getCount() > 0);
            OgaAbstractQStateNode::transfer(q_state_node, old_abstract_q_state_node, new_abstract_q_state_node, abstract_q_state_nodes[depth], behavior_flags);
            if (old_abstract_q_state_node->getCount() == 0)
                abstract_q_corpses.insert(old_abstract_q_state_node);
        }

        if (!abstract_node_changed && behavior_flags.smart_reward_handling)
            q_state_node->recalcQstarDistToRepr();

        if (abstract_node_changed || (q_state_node->getQstarDistToRepr() != old_repr_dist))
            _stageForUpdate(q_state_node->getParent());

    }

}

void OgaTree::update_next_distrs(OgaStateNode* node, OgaSearchStats& search_stats) {

    auto next_distr = new NextAbstractQStates();
    auto next_filtered_distr = new NextAbstractQStates();

    if (behavior_flags.state_abs_alg == "asap") {
        for (auto* q_state : node->getChildren()){
            assert (q_state->getAbstractNode()->getCount() > 0);
            next_distr->addAbstractQState(q_state->getAbstractNode());
            next_filtered_distr->addAbstractQState(q_state->getAbstractNode());
        }
    }
    else {
        throw std::runtime_error("Unknown abstraction algorithm: " + behavior_flags.state_abs_alg);
    }

    node->setLastNextDistr(next_distr,next_filtered_distr);
}

bool OgaTree::distrSimilarity(NextAbstractQStates* nd1, NextAbstractQStates* nf1, NextAbstractQStates* nd2, NextAbstractQStates* nf2, OgaStateNode* s1, OgaStateNode* s2 ) {
    for (auto* state : nf1->getStates()){
        if (!nd2->getStates().contains(state))
            return false;
    }
    for (auto* state : nf2->getStates()){
        if (!nd1->getStates().contains(state))
            return false;
    }

    if (behavior_flags.smart_reward_handling){
        bool consistent;
        OgaStateNode::consistentVstarDist(s1,s2,&consistent);
        if (!consistent)
            return false;
    }

    return true;
}

void OgaTree::updateStateAbstractions(unsigned K, int depth, OgaSearchStats& search_stats, std::mt19937& rng) {

    //Sort by id for reproducibility (for eps > 0 case this might make a difference)
    std::vector<OgaStateNode*> sorted_to_update_nodes = std::vector(to_update_states[depth].begin(), to_update_states[depth].end());
    std::sort(sorted_to_update_nodes.begin(), sorted_to_update_nodes.end(),
              [](const OgaStateNode* lhs, const OgaStateNode* rhs) {
                  return lhs->getId() < rhs->getId();  // Sort by ID
              });

    for (auto* state_node : sorted_to_update_nodes){

        state_node->resetRecencyCount();

        if (!state_node->isFullyExpanded() && state_node->numTriedActions() <= behavior_flags.partial_expansion_group_threshold)
            continue;


        OgaAbstractStateNode* old_abstract_state_node = state_node->getAbstractNode();
        assert (old_abstract_state_node == nullptr || state_node->getAbstractNode()->getCount() > 0);
        assert (old_abstract_state_node == nullptr || abstract_state_nodes[depth].contains(old_abstract_state_node));

        OgaAbstractStateNode* new_abstract_state_node = old_abstract_state_node;

        if (behavior_flags.state_abs_alg == "random"){
             if (!state_node->hasReceivedAbsUpdate() && state_node->getAbstractNode()->getCount() <= 1){
                if (std::bernoulli_distribution(behavior_flags.equiv_chance)(rng)){
                    auto term_abs_node = static_cast<int>(terminal_abstract_state_nodes.size()) > depth ? terminal_abstract_state_nodes[depth] : nullptr;
                    new_abstract_state_node = term_abs_node;
                    assert (abstract_state_nodes[depth].size() > 1 || term_abs_node == nullptr);
                    while (new_abstract_state_node == term_abs_node) {
                        auto it = abstract_state_nodes[depth].begin();
                        std::advance(it, std::uniform_int_distribution<size_t>(0, abstract_state_nodes[depth].size() - 1)(rng));
                        new_abstract_state_node = *it;
                    }
                }
            }
        } else if (behavior_flags.state_abs_alg == "asap"){

            bool is_repr = state_node->getAbstractNode()->getRepresentant() == state_node;
            bool is_partial = (behavior_flags.group_partially_expanded_states && state_node->getAbstractNode() == unexplored_abstract_state_nodes[depth]);

            assert (!(state_node->getLastNextDistr() == nullptr && !is_repr && !is_partial));

            auto representant_distr = state_node->getAbstractNode()->getRepresentant()->getLastNextDistr();
            auto representant_filtered_distr = state_node->getAbstractNode()->getRepresentant()->getLastFilteredNextDistr();
            if (state_node != state_node->getAbstractNode()->getRepresentant()) {
                delete state_node->getLastFilteredNextDistr();
                delete state_node->getLastNextDistr();
            }
            update_next_distrs(state_node,search_stats);

            bool no_match = representant_distr != nullptr && !distrSimilarity(state_node->getLastNextDistr(),state_node->getLastFilteredNextDistr(),representant_distr,representant_filtered_distr, state_node, state_node->getAbstractNode()->getRepresentant());
            if (state_node == state_node->getAbstractNode()->getRepresentant()) {
                delete representant_distr;
                delete representant_filtered_distr;
            }

            if (is_repr || no_match || is_partial){
                //find biggest matching abs node
                OgaAbstractStateNode* merge = nullptr;
                for (auto it = abstract_state_nodes[depth].rbegin() ; it != abstract_state_nodes[depth].rend(); ++it){ //sorted by abs size and id
                    auto other = *it;
                    auto repr = other->getRepresentant();
                    assert (repr != nullptr && other->getCount() > 0);

                    if ( (behavior_flags.group_partially_expanded_states && other == unexplored_abstract_state_nodes[depth]) || repr == state_node || repr->getLastNextDistr() == nullptr)
                        continue;
                    if ( (merge != nullptr && merge->getCount() > other->getCount()) || (merge != nullptr && merge->getCount() == other->getCount() && merge->getId() < other->getId()))
                        break;

                    if (distrSimilarity(state_node->getLastNextDistr(),state_node->getLastFilteredNextDistr(),repr->getLastNextDistr(),repr->getLastFilteredNextDistr(),state_node,repr))
                        merge = other;
                }

                if (merge != nullptr)
                    new_abstract_state_node = merge;
                else if (is_partial || (no_match && old_abstract_state_node->getCount() > 1))  //Create new abstract node
                    new_abstract_state_node = new OgaAbstractStateNode(depth, search_stats);
            }
        }

        // Transfer abstract nodes
        bool abstract_node_changed = old_abstract_state_node != new_abstract_state_node;
        if (abstract_node_changed) {
            OgaAbstractStateNode::transfer(state_node, old_abstract_state_node, new_abstract_state_node, abstract_state_nodes[depth]);
            if (old_abstract_state_node->getCount() == 0) {
                abstract_state_nodes[depth].erase(old_abstract_state_node);
                abstract_state_corpses.insert(old_abstract_state_node);
                if (old_abstract_state_node->getKey() != nullptr) { //nullptr happens when a freshly inited node gets directly moved to a bigger abs node
                    auto *old_key = static_cast<NextAbstractQStates*>(old_abstract_state_node->popAndSetKey(nullptr));
                    assert(old_key != nullptr && abstract_state_node_map[depth].contains(old_key) && abstract_state_node_map[depth][old_key] == old_abstract_state_node);
                    abstract_state_node_map[depth].erase(old_key);
                    delete old_key;
                }
                if (behavior_flags.group_partially_expanded_states && unexplored_abstract_state_nodes[depth] == old_abstract_state_node){
                    unexplored_abstract_state_nodes[depth] = nullptr;
                }
            }
        }

        double old_vstar_dist = state_node->getVstarDistToRepr();
        if (behavior_flags.smart_reward_handling){
            [[maybe_unused]] bool consistent;
            double new_dist = OgaStateNode::consistentVstarDist(state_node,state_node->getAbstractNode()->getRepresentant(), &consistent);
            assert (consistent);
            state_node->setVstarDistToRepr(new_dist);
        }

        state_node->setReceivedAbsUpdate(true);
        if (abstract_node_changed || (state_node->getVstarDistToRepr() != old_vstar_dist)) {
            for (auto* q_state_node_parent : state_node->getParents())
                _stageForUpdate(q_state_node_parent);
        }

    }

}

void OgaTree::performUpdateAbstractions(unsigned K, OgaSearchStats& search_stats, std::mt19937& rng){

    assert (behavior_flags.state_abs_alg != "asap" || to_update_q_states.size() >= to_update_states.size());
    for (int depth = static_cast<int>(std::max(to_update_q_states.size(),to_update_states.size())) - 1; depth >= 0; depth--){
        if (depth < static_cast<int>(to_update_q_states.size()))
            updateQAbstractions(K, depth, rng, search_stats);
        if (depth < static_cast<int>(to_update_states.size()))
            updateStateAbstractions(K, depth,search_stats, rng);
    }

    // Clear all update sets
    for (auto& update_set : to_update_q_states)
        update_set.clear();
    for (auto& update_set : to_update_states)
        update_set.clear();
}

void OgaTree::addUpdateQStateNodeAbstraction(OgaQStateNode* q_state_node){
    _stageForUpdate(q_state_node);
}

void OgaTree::addUpdateStateNodeAbstraction(OgaStateNode* state_node) {
    _stageForUpdate(state_node);
}


void OgaTree::estimateValueEquivalentAbsStateRatio(std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map, int sample_pairs, std::mt19937& rng, std::map<std::string,double>& global_statistics, std::map<std::string,std::map<int,double>>& layerwise_statistics) {
    int correct_pairs = 0;
    double regret = 0;

    //build list of all state nodes that have a non-trivial abstraction
    std::vector<OgaStateNode*> state_nodes;
    for (const auto& state_node : d_states){
        if (state_node->getAbstractNode()->getCount() > 1 && !state_node->getState()->terminal)
            state_nodes.push_back(state_node);
    }

    if (state_nodes.size() >= 2) {
        int samples = 0;
        while (samples < sample_pairs){

            //sample a random state node
            int sample_idx1 = std::uniform_int_distribution<int>(0, state_nodes.size() - 1)(rng);
            auto state_node1 = state_nodes[sample_idx1];

            //sample a second different state from the same abstract node
            auto ground_nodes = state_node1->getAbstractNode()->getGroundNodes();
            auto it = ground_nodes.begin();
            std::advance(it, std::uniform_int_distribution<int>(0, ground_nodes.size() - 1)(rng));
            auto state_node2 = *it;
            if (state_node2->getState()->terminal)
                throw std::runtime_error("Terminal state in abstract node");

            if (state_node1 == state_node2)
                continue;
            else
                samples++;

            //get the Q-value of the best action in the abstract state
            double max_Q1 = -std::numeric_limits<double>::infinity();
            double max_Q2 = -std::numeric_limits<double>::infinity();
            for (int action : model->getActions(state_node1->getState())){
                auto state_cpy = dynamic_cast<FINITEH::Gamestate*>(state_node1->getStateCopy(model));
                max_Q1 = std::max(max_Q1, Q_map->at({state_cpy, action}));
                delete state_cpy;
            }
            for (int action : model->getActions(state_node2->getState())){
                auto state_cpy = dynamic_cast<FINITEH::Gamestate*>(state_node2->getStateCopy(model));
                max_Q2 = std::max(max_Q2, Q_map->at({state_cpy, action}));
                delete state_cpy;
            }

            if (std::fabs(max_Q1 - max_Q2) < 1e-6)
                correct_pairs++;
            regret += std::fabs(max_Q1 - max_Q2);
        }
    }

    global_statistics["abs_val_equiv_sample_pairs"] += static_cast<double>(correct_pairs);
    global_statistics["abs_val_equiv_sample_pairs_regret"] += regret;
    global_statistics["abs_val_equiv_sample_pairs_total"] += state_nodes.size() >= 2? static_cast<double>(sample_pairs) : 0;
}

void OgaTree::updateStatistics(std::map<std::string,std::map<int,double>>& layerwise_statistics, std::map<std::string,double>& global_statistics, std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map, std::mt19937& rng) {

    //State node statistics
    for (auto [depth,abs_layer] : abstract_state_nodes){

        int non_trivial_sizes_sum = 0;
        int non_trivial_num = 0;
        int trivial_num = 0;
        int total = 0;

        for (auto abs_node : abs_layer) {
            bool unexplored_abs = (int)unexplored_abstract_state_nodes.size() > depth && abs_node == unexplored_abstract_state_nodes[abs_node->getDepth()];
            bool term_abs = (int)terminal_abstract_state_nodes.size() > depth && abs_node == terminal_abstract_state_nodes[abs_node->getDepth()];
            bool received_update = false;
            for (const auto* state_node : abs_node->getGroundNodes()){
                if (state_node->hasReceivedAbsUpdate()){
                    received_update = true;
                    break;
                }
            }
            if (unexplored_abs || term_abs || !received_update)
                continue;

            total++;
            if (abs_node->getCount() > 1) {
                non_trivial_sizes_sum += abs_node->getCount();
                non_trivial_num++;
            }else if (abs_node->getCount() == 1)
                trivial_num++;
            else
                throw std::runtime_error("Invalid count");

        }

        layerwise_statistics["non_trivial_state_abs_count_sum"][depth] += non_trivial_sizes_sum;
        layerwise_statistics["non_trivial_state_abs_num"][depth] += non_trivial_num;
        layerwise_statistics["trivial_state_abs_num"][depth] += trivial_num;
        layerwise_statistics["total_state_abs_num"][depth] += total;
    }

    //Qnode statistics
    for (auto [depth,abs_layer] : abstract_q_state_nodes){

        int non_trivial_sizes_sum = 0;
        int non_trivial_num = 0;
        int trivial_num = 0;
        int total = 0;

        for (auto abs_node : abs_layer) {

            bool received_update = false;
            for (const auto* q_state_node : abs_node->getGroundNodes()){
                if (q_state_node->hasReceivedAbsUpdate()){
                    received_update = true;
                    break;
                }
            }
            if (!received_update)
                continue;

            total++;
            if (abs_node->getCount() > 1) {
                non_trivial_sizes_sum += abs_node->getCount();
                non_trivial_num++;
            }else if (abs_node->getCount() == 1) {
                trivial_num++;
            }
        }

        layerwise_statistics["non_trivial_q_abs_count_sum"][depth] += non_trivial_sizes_sum;
        layerwise_statistics["non_trivial_q_abs_num"][depth] += non_trivial_num;
        layerwise_statistics["trivial_q_abs_num"][depth] += trivial_num;
        layerwise_statistics["total_q_abs_num"][depth] += total;
    }

    global_statistics["statistics_updates"]++;
    if (Q_map != nullptr && Q_map->size() > 0) {
        const static int abs_equiv_sample_pairs = 1000;
        estimateValueEquivalentAbsStateRatio(Q_map, abs_equiv_sample_pairs, rng, global_statistics, layerwise_statistics);
    }

}

void OgaTree::printAbsTree(ABS::Model* model, size_t layers) const {
    // std::cout << "-------- OGA abs tree ------------ " << std::endl;
    // for (size_t depth = 0; depth < abstract_state_nodes.size(); depth++) {
    //     auto astates = abstract_state_nodes.at(depth);
    //     std::cout << "Depth " << depth << " states:" << astates.size() << std::endl;
    //     if (abstract_q_state_nodes.contains(depth)) {
    //         auto qstates = abstract_q_state_nodes.at(depth);
    //         std::cout << "Depth " << depth << " Q-states:" << qstates.size() << std::endl;
    //     }
    // }

    std::cout << "-------- OGA abs tree ------------ " << std::endl;

    for (size_t depth = 0; depth < std::min(layers,abstract_state_nodes.size()); depth++) {
        std::cout << "Depth " << depth << " states:" << std::endl;
        auto astates = abstract_state_nodes.at(depth);
        for (auto* state_node : astates) {
            std::cout << " [ Repr:" << state_node->getRepresentant()->getId() << ", ";

            std::vector<OgaStateNode*> sorted_nodes = std::vector(state_node->getGroundNodes().begin(), state_node->getGroundNodes().end());
            std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                      [](const OgaStateNode* lhs, const OgaStateNode* rhs) {
                          return lhs->getId() < rhs->getId();  // Sort by ID
                      });
            for (const auto state_node : sorted_nodes) {
                //model->printState(state_node->getState());
                std::cout << state_node->getId() <<  ", ";
            }
            std::cout << " ]    ";
        }
        std::cout << std::endl;

        std::cout << "Depth " << depth << " Q-states:" << std::endl;
        if (abstract_q_state_nodes.find(depth) == abstract_q_state_nodes.end())
            continue;
        auto aqstates = abstract_q_state_nodes.at(depth);
        for (auto* q_state_node : aqstates) {
            std::cout << " [ Repr: " << q_state_node->getRepresentant()->getId() << ", ";

            std::vector<OgaQStateNode*> sorted_nodes = std::vector(q_state_node->getGroundNodes().begin(), q_state_node->getGroundNodes().end());
            std::sort(sorted_nodes.begin(), sorted_nodes.end(),
                      [](const OgaQStateNode* lhs, const OgaQStateNode* rhs) {
                          return lhs->getId() < rhs->getId();  // Sort by ID
            });

            for (const auto ground_q_state_node : sorted_nodes) {
                std::cout << ground_q_state_node->getId() << ": (" << ground_q_state_node->getAction() << ", " << ground_q_state_node->getAbsValues() / (double) ground_q_state_node->getAbsVisits() << ", " << ground_q_state_node->getVisits() << "), ";
            }
            std::cout << " ]    ";
        }
        std::cout << std::endl;

    }

    std::cout << "ABS Q Node set:" << std::endl;
    for (const auto& [d,nodes] : abstract_q_state_nodes) {
        std::cout << "Depth " << d << " states:" << std::endl;
        for (const auto* state_node : nodes)
            std::cout << state_node->getId() << " | Repr:" << state_node->getRepresentant()->getId() << std::endl;
    }

    std::cout << "ABS State Node set:" << std::endl;
    for (const auto& [d,nodes] : abstract_state_nodes) {
        std::cout << "Depth " << d << " states:" << std::endl;
        for (const auto* state_node : nodes)
            std::cout << state_node->getId() << " | Repr:" << state_node->getRepresentant()->getId() << std::endl;
    }

}
