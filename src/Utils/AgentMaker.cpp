#include "../../include/Utils/AgentMaker.h"
#include "../../include/Agents/Oga/OgaAgent.h"
#include "../../include/Agents/Mcts/MctsAgent.h"
#include "../../include/Games/TwoPlayerGames/Pusher.h"
#include "../../include/Agents/RandomAgent.h"

#include <map>
#include <set>
#include <sstream>


Agent* getDefaultAgent(bool strong){
    if (strong)
        return new Mcts::MctsAgent({{500, "iterations"}, {4}, 1.0, 1, -1, true, true});
    else
       return new RandomAgent();
}

std::string extraArgs(std::map<std::string, std::string>& given_args, const std::set<std::string>& acceptable_args){
    for (auto& [key, val] : given_args) {
        if(!acceptable_args.contains(key))
            return key;
    }
    return "";
}

Agent* getAgent(const std::string& agent_type, const std::vector<std::string>& a_args)
{
    //Parse named args
    std::map<std::string, std::string> agent_args;
    for(auto &arg   : a_args) {
        //split at '='
        auto pos = arg.find('=');
        if (pos == std::string::npos) {
            std::cout << "Invalid agent argument: " << arg << ". It must be of the form arg_name=arg_val" << std::endl;
            return nullptr;
        }
        agent_args[arg.substr(0, pos)] = arg.substr(pos + 1);
    }
    std::set<std::string> acceptable_args;

    Agent* agent;
    if (agent_type == "random") {
        acceptable_args = {};
        agent =  new RandomAgent();
    }
    else if(agent_type == "mcts")
    {
        assert (agent_args.contains("iterations"));
        if(agent_args.contains("wirsa"))
            assert (agent_args.contains("a") && agent_args.contains("b"));
        acceptable_args = {"iterations", "rollout_length", "discount", "num_rollouts", "dag", "dynamic_exp_factor", "expfacs", "wirsa", "a", "b", "puct", "max_backup"};

        int iterations = std::stoi(agent_args["iterations"]);
        int rollout_length = agent_args.find("rollout_length") == agent_args.end() ? -1 : std::stoi(agent_args["rollout_length"]);
        double discount = agent_args.find("discount") == agent_args.end() ? 1.0 : std::stod(agent_args["discount"]);
        int num_rollouts = agent_args.find("num_rollouts") == agent_args.end() ? 1: std::stoi(agent_args["num_rollouts"]);
        bool dag = agent_args.find("dag") == agent_args.end() ? false : std::stoi(agent_args["dag"]);
        bool dynamic_exp_factor = agent_args.find("dynamic_exp_factor") == agent_args.end() ? false : std::stoi(agent_args["dynamic_exp_factor"]);
        bool wirsa = agent_args.find("wirsa") == agent_args.end() ? false : std::stoi(agent_args["wirsa"]);
        double a = agent_args.find("a") == agent_args.end() ? 0.0 : std::stod(agent_args["a"]);
        double b = agent_args.find("b") == agent_args.end() ? 0.0 : std::stod(agent_args["b"]);
        std::string exp_facs = agent_args.find("expfacs") == agent_args.end() ? "1" : agent_args["expfacs"];
        std::vector<double> expfac;
        std::stringstream ss(exp_facs);
        double i;
        while (ss >> i){
            expfac.push_back(i);
            if (ss.peek() == ';')
                ss.ignore();
        }
        bool max_backup = agent_args.find("max_backup") == agent_args.end() ? false : std::stoi(agent_args["max_backup"]);
        bool puct = agent_args.find("puct") == agent_args.end() ? false : std::stoi(agent_args["puct"]);

        auto args = Mcts::MctsArgs{.budget = {iterations, "iterations"}, .exploration_parameters = expfac, .discount = discount,
            .num_rollouts = num_rollouts,
            .rollout_length = rollout_length,
            .dag=dag,
            .dynamic_exploration_factor=dynamic_exp_factor,
            .max_backup = max_backup,
            .wirsa = wirsa,
            .a=a,.b=b, .puct = puct};
        agent =  new Mcts::MctsAgent(args);
    }
     else if (agent_type == "oga") {
        assert (agent_args.contains("iterations"));
        acceptable_args = {"iterations", "discount", "expfac", "K","group_terminal_states", "group_partially_expanded_states", "equiv_chance",
            "consider_missing_outcomes", "policy_nn_path", "value_nn_path", "smart_reward_handling", "q_abs_alg", "track_statistics",
            "partial_expansion_group_threshold", "ignore_partially_expanded_states", "eps_a", "eps_t", "abs_alg", "filter_bonus", "confidence", "in_abs_policy", "alpha",
            "drop_check_point", "drop_threshold", "drop_confidence", "drop_at_visits", "num_rollouts", "rollout_length", "state_abs_min_visits", "state_abs_top_n_matches"};

        int iterations = std::stoi(agent_args["iterations"]);
        double discount = agent_args.find("discount") == agent_args.end() ? 1.0 : std::stod(agent_args["discount"]);
        double expfac = agent_args.find("expfac") == agent_args.end() ? 2.0 : std::stod(agent_args["expfac"]);
        unsigned K = agent_args.find("K") == agent_args.end() ? 3 : std::stoi(agent_args["K"]);
        bool group_terminal_states = agent_args.find("group_terminal_states") == agent_args.end() ? true : std::stoi(agent_args["group_terminal_states"]);
        bool group_partially_expanded_states = agent_args.find("group_partially_expanded_states") == agent_args.end() ? false : std::stoi(agent_args["group_partially_expanded_states"]);
        unsigned partial_expansion_group_threshold = agent_args.find("partial_expansion_group_threshold") == agent_args.end() ? std::numeric_limits<int>::max() : std::stoi(agent_args["partial_expansion_group_threshold"]);
        double eps_a = agent_args.find("eps_a") == agent_args.end() ? 0.0 : std::stod(agent_args["eps_a"]);
        double eps_t = agent_args.find("eps_t") == agent_args.end() ? 0.0 : std::stod(agent_args["eps_t"]);
        int num_rollouts = agent_args.find("num_rollouts") == agent_args.end() ? 1 : std::stoi(agent_args["num_rollouts"]);
        int rollout_length = agent_args.find("rollout_length") == agent_args.end() ? -1 : std::stoi(agent_args["rollout_length"]);
        double equiv_chance = agent_args.find("equiv_chance") == agent_args.end() ? 0.1 : std::stod(agent_args["equiv_chance"]);
        double alpha = agent_args.find("alpha") == agent_args.end() ? 0.0 : std::stod(agent_args["alpha"]);
        bool consider_missing_outcomes = agent_args.find("consider_missing_outcomes") == agent_args.end() ? false : std::stoi(agent_args["consider_missing_outcomes"]);
        bool track_statistics = agent_args.find("track_statistics") == agent_args.end() ? false : std::stoi(agent_args["track_statistics"]);

         std::string q_abs_alg = agent_args.find("q_abs_alg") == agent_args.end() ? "eps" : agent_args["q_abs_alg"];
         std::string abs_alg = agent_args.find("abs_alg") == agent_args.end() ? "asap" : agent_args["abs_alg"];
        bool smart_reward_handling = agent_args.find("smart_reward_handling") == agent_args.end() ? false : std::stoi(agent_args["smart_reward_handling"]);
         auto policy_nn_path = agent_args.find("policy_nn_path") == agent_args.end() ? "" : agent_args["policy_nn_path"];
         auto value_nn_path = agent_args.find("value_nn_path") == agent_args.end() ? "" : agent_args["value_nn_path"];

        auto args = OGA::OgaArgs{
            .budget = {iterations, "iterations"},
            .recency_count_limit = K,
            .exploration_parameter = expfac,
            .discount = discount,
            .num_rollouts = num_rollouts,
            .rollout_length = rollout_length,
            .behavior_flags = {
                .group_terminal_states=group_terminal_states,
                .group_partially_expanded_states=group_partially_expanded_states,
                .partial_expansion_group_threshold=partial_expansion_group_threshold,
                .q_abs_alg = q_abs_alg,
                .eps_a = eps_a,
                .eps_t = eps_t,
                .consider_missing_outcomes = consider_missing_outcomes,
                .alpha = alpha,
                .equiv_chance = equiv_chance,
                .state_abs_alg = abs_alg,
                .smart_reward_handling = smart_reward_handling,
            },
            .track_statistics = track_statistics,
        };
        agent =  new OGA::OgaAgent(args);
    }else{
        throw std::runtime_error("Invalid agent");
    }

    if (agent != nullptr) {
        if (!extraArgs(agent_args, acceptable_args).empty()) {
            std::string err_string = "Invalid agent argument: " + extraArgs(agent_args, acceptable_args);
            std::cout << err_string << std::endl;
            throw std::runtime_error(err_string);
        }
        return agent;
    }else {
        std::cout << "Invalid agent" << std::endl;
        throw std::runtime_error("Invalid agent");
    }
}