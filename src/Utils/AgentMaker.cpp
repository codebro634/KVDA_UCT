#include "../../include/Utils/AgentMaker.h"
#include "../../include/Agents/Oga/OgaAgent.h"
#include "../../include/Games/TwoPlayerGames/Pusher.h"
#include "../../include/Agents/RandomAgent.h"

#include <map>
#include <set>
#include <sstream>


Agent* getDefaultAgent(bool strong){
    assert(!strong);
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
     else if (agent_type == "oga") {
        assert (agent_args.contains("iterations"));
        acceptable_args = {"iterations", "discount", "expfac", "K","group_terminal_states", "group_partially_expanded_states", "equiv_chance",
            "consider_missing_outcomes", "smart_reward_handling", "q_abs_alg", "track_statistics",
            "partial_expansion_group_threshold", "ignore_partially_expanded_states", "eps_a", "eps_t", "abs_alg", "alpha",
            "num_rollouts", "rollout_length"};

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
         std::string in_abs_policy = agent_args.find("in_abs_policy") == agent_args.end() ? "random" : agent_args["in_abs_policy"];
        std::string abs_alg = agent_args.find("abs_alg") == agent_args.end() ? "asap" : agent_args["abs_alg"];
        bool smart_reward_handling = agent_args.find("smart_reward_handling") == agent_args.end() ? false : std::stoi(agent_args["smart_reward_handling"]);


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