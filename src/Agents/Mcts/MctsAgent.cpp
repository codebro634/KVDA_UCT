#include "../../../include/Agents/Mcts/MctsAgent.h"

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <queue>
#include <fstream>
#include <ranges>

using namespace Mcts;

MctsAgent::MctsAgent(const MctsArgs& args):
    exploration_parameters(args.exploration_parameters),
    discount(args.discount),
    budget(args.budget),
    num_rollouts(args.num_rollouts),
    dag(args.dag),
    dynamic_exploration_factor(args.dynamic_exploration_factor),
    max_backup(args.max_backup),
    rollout_length(args.rollout_length),
    wirsa(args.wirsa),
    a(args.a),
    b(args.b),
    puct(args.puct),
    greedy_decision_policy(args.greedy_decision_policy)
    {}

MctsNode* MctsAgent::buildTree(ABS::Model* model, ABS::Gamestate* state, MctsSearchStats& search_stats, std::mt19937& rng, bool determinize_env, bool determ_var_reduction){

    const auto start = std::chrono::high_resolution_clock::now();

    auto init_state = model->copyState(state);
    auto* root = new MctsNode(model, init_state,0,  rng);
    std::map<int,gsToNodeMap<MctsNode*>> layerMap = {};
    if(dag)
        layerMap = {{0,{{init_state,root}}}};
    search_stats = {budget, 0, 0, 0, &layerMap,std::vector<double>(model->getNumPlayers(), 0), std::vector<double>(model->getNumPlayers(),0),0, determinize_env, determ_var_reduction, {}, determinize_env? std::uniform_int_distribution<int>(0, 10000000)(rng) : 0};
    const int total_forward_calls_before = model->getForwardCalls();

    while ( // Within budget
        (budget.quantity == "iterations" && search_stats.completed_iterations < budget.amount) ||
        (budget.quantity == "forward_calls" && search_stats.total_forward_calls < budget.amount) ||
        (budget.quantity == "milliseconds" && std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - start).count() < budget.amount)
    ){
        // MCTS with forward calls cond can end in infinite loop
        auto leaf_path = treePolicy(model, root, rng, search_stats);
        const auto rewards = rollout(model, std::get<0>(leaf_path.back()), rng);
        backup(rewards,leaf_path, search_stats);

        search_stats.completed_iterations++;
        search_stats.total_forward_calls = model->getForwardCalls() - total_forward_calls_before;
    }
    return root;
}

void MctsAgent::cleanupTree(MctsNode* root){
    //cleanup tree
    std::vector<MctsNode*> clear_stack = {root};
    std::set<MctsNode*> to_clear_nodes = {root};
    while(!clear_stack.empty()){
        auto next = clear_stack.back();
        clear_stack.pop_back();
        for (auto& q_state : std::views::values(*next->getChildren())){
            for (const auto& child_state_node : std::views::values(q_state)){
                if(!to_clear_nodes.contains(child_state_node)){
                    clear_stack.push_back(child_state_node);
                    to_clear_nodes.insert(child_state_node);
                }
            }
        }
    }

    delete root->getState();
    for(auto* node : to_clear_nodes){
        for (auto& q_state : std::views::values(*node->getChildren())){
            for (const auto& gamestate : std::views::keys(q_state))
               delete gamestate;
        }
        delete node;
    }
}


int MctsAgent::getAction(ABS::Model* model, ABS::Gamestate* state, std::mt19937& rng){
    MctsSearchStats search_stats;
    auto root = buildTree(model, state, search_stats, rng);
    const int best_action = greedy_decision_policy? selectAction(root, true, rng, search_stats) : sampleAction(root, rng);
    cleanupTree(root);
    return best_action;
}

std::vector<std::tuple<MctsNode*,int,std::vector<double>>> MctsAgent::treePolicy(ABS::Model* model, MctsNode* node, std::mt19937& rng, MctsSearchStats& search_stats){
    bool reached_leaf = false;
    std::vector<std::tuple<MctsNode*,int,std::vector<double>>>  state_action_reward_path;
    auto old_node = node;
    while (!node->isTerminal() && !reached_leaf){
        int chosen_action;
        std::vector<double> rewards;
        node = selectNode(model, node, reached_leaf, chosen_action, rewards, rng, search_stats);
        state_action_reward_path.emplace_back(old_node,chosen_action, rewards);
        old_node = node;
        if (node->getDepth() > search_stats.max_depth)
            search_stats.max_depth = node->getDepth();
    }
    state_action_reward_path.push_back({node,-1,{}});
    return state_action_reward_path;
}

MctsNode* MctsAgent::selectNode(ABS::Model* model, MctsNode* node, bool& reached_leaf, int& chosen_action, std::vector<double>& rewards,  std::mt19937& rng, MctsSearchStats& search_stats)
{
    reached_leaf = false;
    chosen_action = node->isFullyExpanded()? selectAction(node, false, rng, search_stats) : node->popUntriedAction(max_backup? -std::numeric_limits<double>::infinity() : 0.0);
    auto sample_state = node->getStateCopy();

    int determ_seed=0;
    if(search_stats.deterministic_env)
    {
        if(search_stats.determ_var_reduction){
            if( static_cast<int>(search_stats.layer_seeds.size()) <= node->getDepth()){
                std::uniform_int_distribution<int> uni_dist(0, 10000000);
                search_stats.layer_seeds.push_back(uni_dist(rng));
            }
            determ_seed = search_stats.layer_seeds[node->getDepth()];
        }else
            determ_seed = static_cast<int>(search_stats.determinization_salt + node->id + 37*std::hash<int>()(chosen_action));
    }
    std::mt19937 determ_action_rng(determ_seed);

    // Sample successor of state-action-pair
    auto [rewards_tmp, probability] = model->applyAction(sample_state, chosen_action, search_stats.deterministic_env? determ_action_rng : rng, nullptr);
    rewards = rewards_tmp;

    if(wirsa && node->getActionVisits(chosen_action) > 0) {
        //get nearest neighbor of outcome state
        MctsNode* nearest_neighbor = nullptr;
        double min_distance = std::numeric_limits<double>::infinity();
        for(auto& [outcome, child] : (*node->getChildren())[chosen_action]){
            double distance = model->getDistance(sample_state,child->getState());
            if(distance < min_distance){
                min_distance = distance;
                nearest_neighbor = child;
            }
        }
        assert (nearest_neighbor != nullptr);
        double eps = a * pow(nearest_neighbor->getVisits(),b);
        if(min_distance < eps) {
            delete sample_state;
            return nearest_neighbor;
        }
    }

    const auto* successors = &node->getChildren()->at(chosen_action);
    if (!node->getChildren()->at(chosen_action).contains(sample_state)){

        assert (!search_stats.deterministic_env || node->getActionVisits(chosen_action) == 0);
        // New successor sampled
        if(dag && (*search_stats.layerMap)[node->getDepth()+1].contains(sample_state))
        {
            auto* new_leaf = (*search_stats.layerMap)[node->getDepth()+1][sample_state];
            (*node->getChildren())[chosen_action][sample_state] = new_leaf;
            return new_leaf;
        }else{
            auto* new_leaf = new MctsNode(model, sample_state, node->getDepth() + 1, rng);
            reached_leaf = true;
            if(dag)
                (*search_stats.layerMap)[node->getDepth()+1][sample_state] = new_leaf;
            (*node->getChildren())[chosen_action][sample_state] = new_leaf;
            return new_leaf; //we dont delete sample state here because it has to be saved in the new node
        }
    }

    // Already sampled successor
    auto successor = successors->at(sample_state);
    delete sample_state;
    return successor;
}

int MctsAgent::sampleAction(MctsNode* node,std::mt19937& rng)
{
    //sample proportional to visit count
    double total_visits = 0.0;
    for (const int action : *node->getTriedActions())
        total_visits += node->getActionVisits(action);

    std::vector<double> probs = std::vector<double>(node->getTriedActions()->size(), 0.0);
    int idx = 0;
    for (const int action : *node->getTriedActions())
        probs[idx++] = node->getActionVisits(action) / (double)total_visits;

    std::discrete_distribution<> dist(probs.begin(), probs.end());
    int sampled_action = (*node->getTriedActions())[dist(rng)];
    return sampled_action;
}

int MctsAgent::selectAction(MctsNode* node, bool greedy, std::mt19937& rng, MctsSearchStats& search_stats)
{
    // UCT Formula: w/n + c * sqrt(ln(N)/n)
    assert(!node->getTriedActions()->empty());

    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    // Determine action
    const double node_visits = node->getVisits();

    double best_value = -std::numeric_limits<double>::infinity();
    int best_action = -42;

    //For local std calculation
    double dynamic_exp_factor = 1;
    if(dynamic_exploration_factor){
        const double var = std::max(0.0,search_stats.total_squared_v[node->getPlayer()] / search_stats.global_num_vs - (search_stats.total_v[node->getPlayer()] / search_stats.global_num_vs) *  (search_stats.total_v[node->getPlayer()] / search_stats.global_num_vs));
        dynamic_exp_factor = sqrt(var);
    }

    for (const int action : *node->getTriedActions()){
        const auto action_values = node->getActionValues(action);
        const double action_visits = node->getActionVisits(action);
        const double exploration_term = puct? (sqrt(node_visits) / (1 + action_visits)) : (sqrt(log(node_visits) / action_visits));
        const double q_value = action_values->at(node->getPlayer()) / (max_backup? 1.0 : action_visits);

        double exploration_param = node->getDepth() >= static_cast<int>(exploration_parameters.size()) ? exploration_parameters.back() : exploration_parameters[node->getDepth()];
        double score;
        if(exploration_param == -1 && !greedy) //greedy overwrites uniform
            score = exploration_term;
        else
            score = q_value + (greedy? 0 : exploration_param * exploration_term * dynamic_exp_factor);
        score += TIEBREAKER_NOISE * dist(rng); //trick to efficiently break ties
        if (score > best_value){
            best_value = score;
            best_action = action;
        }
    }

    return best_action;
}

std::vector<double> MctsAgent::rollout(ABS::Model* model, MctsNode* node, std::mt19937& rng) const
{
    auto reward_sum = std::vector<double>(model->getNumPlayers(), 0);
    if (node->isTerminal())
    {
        return reward_sum;
    }

    for(int i = 0; i < num_rollouts; i++) {
        double total_discount = 1;
        auto* rollout_state = node->getStateCopy();
        int episode_steps = 0;
        while (!rollout_state->terminal && (rollout_length == -1 || episode_steps < rollout_length))
        {
            // Sample action
            auto available_actions = model->getActions(rollout_state);
            std::uniform_int_distribution<int> dist(0, static_cast<int>(available_actions.size()) - 1);
            const int action = available_actions[dist(rng)];

            // Apply action and get rewards
            auto [rewards, probability] = model->applyAction(rollout_state, action,rng, nullptr);
            for (size_t player = 0; player < rewards.size(); player++)
            {
                reward_sum[player] += rewards[player] * total_discount * (1 / (double) num_rollouts);
            }
            total_discount *= discount;
            episode_steps++;
        }
        delete rollout_state;
    }

    return reward_sum;
}

void MctsAgent::backup(std::vector<double> values, std::vector<std::tuple<MctsNode*,int,std::vector<double>>>& path, MctsSearchStats& search_stats) const
{
    for (size_t i = path.size()-1; i >= 1; i--)
    {
        auto parent = std::get<0>(path[i-1]);
        auto parent_action = std::get<1>(path[i-1]);
        auto rewards = std::get<2>(path[i-1]);

        for (size_t player = 0; player < values.size(); player++)
            values[player] = values[player] * discount + rewards[player];

        for (size_t player = 0; player < values.size(); player++) {
            if(parent->getActionVisits(parent_action) >= 1) { //only remove value if it was present before
                double old_q = parent->getActionValues(parent_action)->at(player) / (max_backup? 1.0 : ((double) parent->getActionVisits(parent_action)));
                search_stats.total_v[player] -= old_q;
                search_stats.total_squared_v[player] -= old_q * old_q;
            }
        }

        parent->addVisit();
        parent->addActionVisit(parent_action);
        parent->addActionValues(parent_action, values, max_backup);

        if(parent->getActionVisits(parent_action) == 1)
            search_stats.global_num_vs++;

        for (size_t player = 0; player < values.size(); player++){
            double q = parent->getActionValues(parent_action)->at(player) / (max_backup? 1.0 : (double) parent->getActionVisits(parent_action));
            search_stats.total_v[player] += q;
            search_stats.total_squared_v[player] += q*q;
        }

    }
}