#include "../../../include/Agents/Oga/OgaAgent.h"
#include "../../../include/Agents/Oga/OgaTree.h"
#include "../../../include/Agents/Oga/OgaUtils.h"
#include "../../../include/Agents/Oga/OgaAbstractNodes.h"
#include "../../../include/Agents/Oga/OgaGroundNodes.h"

#include "../../../include/Games/Wrapper/FiniteHorizon.h"

#include <cassert>
#include <cmath>
#include <chrono>
#include <utility>
#include <cstring>

#include "../../../include/Utils/Distributions.h"

using namespace OGA;

OgaAgent::~OgaAgent(){}

OgaAgent::OgaAgent(const OgaArgs& args) :
    exploration_parameter(args.exploration_parameter),
    discount(args.discount),
    num_rollouts(args.num_rollouts),
    rollout_length(args.rollout_length),
    recency_count_limit(args.recency_count_limit),
    budget(args.budget),
    args(args),
    track_statistics(args.track_statistics),
    Q_map(args.Q_map),
    distribution_agent(args.distribution_agent)
{
    assert (args.exploration_parameter >= 0);
    assert (!args.behavior_flags.smart_reward_handling || args.behavior_flags.eps_a == 0);
}

int OgaAgent::getAction(ABS::Model* model, ABS::Gamestate* state, std::mt19937& rng){
    return getAction(model, state, rng, nullptr);
}

int OgaAgent::getAction(ABS::Model* model, ABS::Gamestate* state, std::mt19937& rng, OgaTree** treePtr){

    assert (dynamic_cast<FINITEH::Model*>(model) != nullptr && dynamic_cast<FINITEH::Gamestate*>(state) != nullptr);
    assert (!args.behavior_flags.smart_reward_handling || model->getNumPlayers() == 1); //Multiplayer is not supported since there is no unique value difference i.e. multiple equilibria

    const auto start = std::chrono::high_resolution_clock::now();

    OgaSearchStats search_stats = {budget, 0, 0,0,0,0,0,0,0,0,0};
    const auto total_forward_calls_before = model->getForwardCalls();

    auto tree = new OgaTree{state, model, args.behavior_flags,rng, search_stats
    };

    bool done = false;
    while (!done){
        OgaStateNode* leaf = treePolicy(tree, model, search_stats, rng);
        const auto rewards = rollout(leaf, model, rng);
        backup(tree, leaf, rewards, search_stats);
        tree->performUpdateAbstractions(recency_count_limit,search_stats, rng);

        search_stats.completed_iterations++;
        search_stats.total_forward_calls = model->getForwardCalls() - total_forward_calls_before;

        if(budget.quantity == "iterations"){
            done = search_stats.completed_iterations >= budget.amount;
        } else if (budget.quantity == "forward_calls"){
            done = static_cast<int>(search_stats.total_forward_calls) >= budget.amount;
        } else if (budget.quantity == "milliseconds"){
            done = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count() >= budget.amount;
        }
    }

    const int best_action = selectAction(model, tree->getRoot(), true, search_stats, rng);

    //Abstraction dropping statistics
    if (track_statistics)
        tree->updateStatistics(layerwise_statistics, global_statistics, Q_map, rng);

    if (treePtr != nullptr)
        *treePtr = tree;
    else
        delete tree;

    return distribution_agent == nullptr? best_action : distribution_agent->getAction(model, state, rng);
}

OgaStateNode* OgaAgent::treePolicy(OgaTree* tree,  ABS::Model* model, OgaSearchStats& search_stats,std::mt19937& rng){

    auto* curr_node = tree->getRoot();

    while (!curr_node->isTerminal())
    {
        if (!curr_node->isFullyExpanded())
        {
            auto* state = curr_node->getStateCopy(model);

            const int action = curr_node->popUntriedAction();
            auto [q_node, found_q] = tree->findOrCreateQState(state, curr_node->getDepth(), action, rng, search_stats
            );
            assert(!found_q);

            auto [rewards, prob] = model->applyAction(state, action, rng, nullptr);
            auto [successor, found] = tree->findOrCreateState(state, curr_node->getDepth() + 1, rng, search_stats
            );

            q_node->addChild(model->copyState(state), prob, successor);
            q_node->setRewards(rewards);
            q_node->setParent(curr_node);

            delete state;

            // Trajectory bookkeeping
            successor->setTrajectoryParent(q_node);

            if (!found){
                if (successor->getDepth() > search_stats.max_depth)
                    search_stats.max_depth = successor->getDepth();

                return successor;
            }
            curr_node = successor;
            continue;
        }

        bool new_state;
        curr_node = selectSuccessorState(tree, curr_node, model, search_stats, rng, &new_state);
        if (new_state)
            return curr_node;
    }

    return curr_node;
}

OgaStateNode* OgaAgent::selectSuccessorState(OgaTree* tree, OgaStateNode* node, ABS::Model* model, OgaSearchStats& search_stats, std::mt19937& rng,
                                             bool* new_state)
{
    const int best_action = selectAction(model, node, false, search_stats, rng);

    const auto sample_state = node->getStateCopy(model);
    auto [q_node, found_q] = tree->findOrCreateQState(sample_state, node->getDepth(), best_action, rng, search_stats
    );
    assert(found_q);

    // Sample successor of state-action-pair
    auto [rewards, prob] = model->applyAction(sample_state, best_action, rng, nullptr);
    auto [successor, found] = tree->findOrCreateState(sample_state, node->getDepth() + 1, rng, search_stats
    );
    *new_state = !found;

    auto state_cpy = model->copyState(sample_state);
    bool added_copy = q_node->addChild(state_cpy, prob, successor);
    if (!added_copy)
        delete state_cpy;

    // Trajectory bookkeeping
    successor->setTrajectoryParent(q_node);

    delete sample_state;
    return successor;
}

int OgaAgent::selectAction(ABS::Model* model, OgaStateNode* node, const bool greedy, OgaSearchStats& search_stats,std::mt19937& rng)
{
    // UCT Formula: w/n + c * sqrt(ln(N)/n)
    assert(node->isPartiallyExpanded());

    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    //Determine N from the UCT formula
    double parent_node_visits = 0;
    for (const auto child_q_node : node->getChildren())
        parent_node_visits += child_q_node->getAbsVisits();


    //Determine c from the UCT formula
    const double var = std::max(0.0,search_stats.total_squared_v / search_stats.global_num_vs - (search_stats.total_v / search_stats.global_num_vs) *  (search_stats.total_v / search_stats.global_num_vs));
    double dynamic_exp_factor = sqrt(var);
    double exploration_factor = greedy? 0 : args.exploration_parameter * dynamic_exp_factor;

    double best_value = -std::numeric_limits<double>::infinity();
    OgaQStateNode* best_qnode = nullptr;
    for (const auto child_q_node : node->getChildren()){

        //Get visits and Q used in uct formula
        double action_visits, Q_value;
        action_visits = child_q_node->getAbsVisits();
        Q_value = child_q_node->getAbsValues() / action_visits;


        //Exploration term in uct formula
        double exploration_term = exploration_factor;
        if (node->policyProbsAvailable(model))
            exploration_term *= node->getPolicyProb(child_q_node->getAction()) * sqrt(parent_node_visits) / (1.0 + action_visits);
        else
            exploration_term *= sqrt(log(parent_node_visits) / action_visits);

        //Get final uct score
        double score = Q_value + exploration_term;
        score += TIEBREAKER_NOISE * dist(rng); //trick to efficiently break ties

        if (score > best_value){
            best_value = score;
            best_qnode = child_q_node;
        }
    }

    if (track_statistics) {
        global_statistics["ucb_calls"]++;
        int n = 0;
        for (const auto child_q_node : node->getChildren()) {
            if (best_qnode->getAbstractNode()->getGroundNodes().contains(child_q_node))
                n++;
            if (n >= 2)
                break;
        }
        global_statistics["intra_abs_required"]+= (n >= 2)? 1 : 0;
    }

    assert (best_qnode != nullptr);
    return best_qnode->getAction();
}

std::vector<double> OgaAgent::rollout(const OgaStateNode* leaf, ABS::Model* model, std::mt19937& rng) const
{
    auto reward_sum = std::vector<double>(model->getNumPlayers(), 0.0);
    if (leaf->isTerminal())
        return reward_sum; // No rewards to collect

    for (int i = 0; i < num_rollouts; i++){
        double total_discount = 1;
        auto* rollout_state = leaf->getStateCopy(model);
        int episode_steps = 0;
        while (!rollout_state->terminal && (rollout_length == -1 || episode_steps < rollout_length)){
            // Sample action
            auto available_actions = model->getActions(rollout_state);
            std::uniform_int_distribution<int> dist(0, static_cast<int>(available_actions.size()) - 1);
            const int action = available_actions[dist(rng)];

            // Apply action and get rewards
            auto [rewards, outcome_and_probability] = model->applyAction(rollout_state, action, rng, nullptr);
            for (int j = 0; j < model->getNumPlayers(); j++)
                reward_sum[j] += rewards[j] * total_discount;
            total_discount *= discount;

            episode_steps++;
        }
        delete rollout_state;
    }

    for (int j = 0; j < model->getNumPlayers(); j++)
        reward_sum[j] /= num_rollouts;

    return reward_sum;
}

void OgaAgent::backup(OgaTree* tree, OgaStateNode* leaf, std::vector<double> values, OgaSearchStats& search_stats) const
{
    auto* child_node = leaf;
    auto* parent_q_node = child_node->popTrajectoryParent();
    while (parent_q_node != nullptr)
    {
        for (size_t i = 0; i < values.size(); i++) {
            auto rewards = parent_q_node->getRewards(i);
            values[i] = values[i] * discount + rewards;
        }

        parent_q_node->addExperience(values[parent_q_node->getState()->turn]);
        parent_q_node->addRecencyCount();
        if (parent_q_node->getRecencyCount() >= recency_count_limit)
            tree->addUpdateQStateNodeAbstraction(parent_q_node);

        child_node = parent_q_node->getParent();
        child_node->addVisit();

        if (args.behavior_flags.state_abs_alg != "asap") {
            child_node->addRecencyCount();
            if (child_node->getRecencyCount() >= recency_count_limit)
                tree->addUpdateStateNodeAbstraction(child_node);
        }

        //Dynamic exploration factor bookkeeping
        if (parent_q_node->getVisits() == 1)
            search_stats.global_num_vs++;
        if(parent_q_node->getVisits() > 1) { //only remove value if it was present before
            int player = parent_q_node->getState()->turn;
            double old_q = (parent_q_node->getValues() - values[player]) / ((double) parent_q_node->getVisits()-1);
            search_stats.total_v -= old_q;
            search_stats.total_squared_v -= old_q * old_q;
        }
        double q = parent_q_node->getValues() / (double) parent_q_node->getVisits();
        search_stats.total_v += q;
        search_stats.total_squared_v+= q*q;

        parent_q_node = child_node->popTrajectoryParent();
    }
}

double OgaAgent::getStatistics(std::string name, int layer) {
    if (layerwise_statistics.contains(name)) {
        if (layer == -1) {
            double stat_sum = 0;
            for (const auto& [depth, value] : layerwise_statistics.at(name))
                stat_sum += value;
            return stat_sum;
        }else
            return layerwise_statistics.at(name).at(layer);
    }else if (global_statistics.contains(name)) {
        assert (layer == -1);
        return global_statistics.at(name);
    }else
        throw std::runtime_error("Unknown or not contained statistics name: " + name);
}