#include <vector>
#include <iostream>
#include <cmath>
#include <map>
#include <algorithm>
#include "../include/Agents/Agent.h"
#include "../include/Arena.h"
#include "../include/Utils/MemoryAnalysis.h"
#include <cassert>
#include <chrono>
#include <queue>
#include "../include/Utils/Distributions.h"
#include "../include/Games/Wrapper/FiniteHorizon.h"
#include <fstream>


static double DEFAULT_CONFIDENCE = 0.99;

void outputStats(OutputMode& output_mode,
                 std::vector<std::pair<double,double>>& results,
                 std::vector<double>& regrets,
                 std::vector<double>& highest_rewards,
                 std::vector<double>& lowest_rewards,
                 std::vector<std::pair<int,int>>& num_optimal_actions,
                 int games_played,
                 std::vector<int>& permutation,

                 std::vector<std::vector<int>>& played_actions,
                 std::vector<std::vector<double>>& individual_times,
                 std::vector<double>& reward_sum,
                 std::vector<unsigned>& num_actions,
                 std::vector<std::vector<double>>& regrets_last_game,
                 std::vector<std::pair<double,double>>& times)
{

    if (output_mode == VERBOSE) {
        std::cout << "------- Interim Results -------" << std::endl;
        for (size_t j = 0; j < results.size(); j++) {
            std::cout << "Player " << j << " total returns: " << results[j].first << " in " << games_played << " games." << std::endl;
            if(num_optimal_actions[j].second > 0) {
                std::cout << "Player " << j << " ratio of optimal play: " << num_optimal_actions[j].first << "/" << num_optimal_actions[j].second << std::endl;
                std::cout << "Player " << j << " average regret: " << regrets[j] / num_optimal_actions[j].second << std::endl;
            }
            //Confidence interval for rewards
            double conf_range = distr::confidence_interval(results[j].first, results[j].second, games_played, DEFAULT_CONFIDENCE, 2).second - results[j].first / games_played;
            std::cout << "Player " << j << " avg rewards: " << results[j].first/games_played << " +- " << conf_range <<  std::endl;

            //Last game reward
            std::cout << "Player " << j << " last game reward: " << reward_sum[j] << std::endl;

            //Best and worst reward
            std::cout << "Player " << j << " highest reward: " << highest_rewards[j] << std::endl;
            std::cout << "Player " << j << " lowest reward: " << lowest_rewards[j] << std::endl;

            //Confidence interval for times
            conf_range = distr::confidence_interval(times[j].first, times[j].second, num_actions[j], DEFAULT_CONFIDENCE, 2).second - times[j].first / num_actions[j];
            std::cout << "Player " << j << " avg time: " << times[j].first/num_actions[j] << " +- " << conf_range <<  std::endl;

            std::cout << "Player " << j << " played " << played_actions[j].size() << " actions: ";
            for (size_t action_idx = 0; action_idx < played_actions[j].size(); action_idx++)
                std::cout << played_actions[j][action_idx] << " ";
            std::cout << std::endl;

            MEMORY::PrintUsedMemory();

            std::cout << std::endl;
        }

    }

    if (output_mode == CSV || output_mode == CSV_OMIT_TIMES)
    {
        // Episode Nr.
        std::cout << games_played << ";";

        // Permutation
        for (int j : permutation)
        {
            std::cout << j << " ";
        }
        std::cout << ";";

        // Player info
        for (size_t j = 0; j < results.size(); j++)
        {
            std::cout << reward_sum[permutation[j]] << ";";

            for (double regret : regrets_last_game[permutation[j]])
            {
                std::cout << regret << " ";
            }
            std::cout << ";";

            for (const auto& played_action : played_actions[j])
            {
                std::cout << played_action << " ";
            }

            if (output_mode != CSV_OMIT_TIMES){
                std::cout << ";";

                double sum_times = 0;
                for (const auto& individual_time : individual_times[j])
                {
                    std::cout << individual_time << " ";
                    sum_times += individual_time;
                }
                std::cout << ";";

                std::cout << sum_times;
            }

            if (j != results.size() - 1)
            {
                std::cout << ";";
            }
        }

        std::cout << std::endl;
    }
}

void outputCsvHeader(int players, bool omit_times)
{
    // Header for Episode Number
    std::cout << "Episode Nr;";

    // Header for Permutation
    std::cout << "Permutation;";

    // Headers for Player Info
    for (int j = 0; j < players; j++) {
        // Reward Sum for each player
        std::cout << "Rewards Player " << j << ";";

        // Cumulative regret for each player
        std::cout << "Regrets Player " << j << ";";

        // Played Actions for each player
        std::cout << "Actions Player " << j;

        if (!omit_times){
            // Individual Times for each player
            std::cout << ";Times Player " << j << ";";

            // Sum of Times for each player
            std::cout << "Total Time Player " << j;
        }

        // Separate players by semicolons
        if (j != players-1) {
            std::cout << ";";
        }
    }

    // End the header row
    std::cout << std::endl;
}

std::vector<std::vector<double>> playGames(
    ABS::Model* unwrapped_model,
    int num_maps,
    std::vector<Agent*> agents,
    std::mt19937& rng,
    OutputMode output_mode,
    std::pair<int,int> horizons,
    bool planning_beyond_execution_horizon,
    bool random_init_state,
    double required_conf_range,
    std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map,
    const std::string& rng_save_path,
    int episode_num_offset)
{

    auto model = FINITEH::Model(unwrapped_model, horizons.first, false);

    std::vector<std::pair<double,double>> results = std::vector<std::pair<double,double>>(model.getNumPlayers(), {0,0}); //cumulative reward, and cumulative squared reward
    std::vector<double> regrets = std::vector<double>(model.getNumPlayers(), 0);
    std::vector<std::pair<double,double>> times = std::vector<std::pair<double,double>>(model.getNumPlayers(), {0,0}); //cumulative times and squared cumul times
    std::vector<unsigned> num_actions = std::vector<unsigned>(model.getNumPlayers(), 0);
    std::vector<std::pair<int,int>> num_optimal_action_chosen = std::vector<std::pair<int,int>>(model.getNumPlayers(), {0,0});
    std::vector<double> highest_rewards = std::vector<double>(model.getNumPlayers(), std::numeric_limits<double>::lowest());
    std::vector<double> lowest_rewards = std::vector<double>(model.getNumPlayers(), std::numeric_limits<double>::max());

    assert (static_cast<int>(agents.size()) == model.getNumPlayers() && horizons.first >= horizons.second);

    if ((output_mode == CSV || output_mode == CSV_OMIT_TIMES) && episode_num_offset == 0)
        outputCsvHeader(model.getNumPlayers(),output_mode == CSV_OMIT_TIMES);

    int games_played = episode_num_offset;
    int num_perms = 1;
    for (size_t i = 2; i <= agents.size(); i++)
        num_perms *= i;

    double performance_confidence_interval_length = 0;
    for (int i = episode_num_offset; i < num_maps || performance_confidence_interval_length > required_conf_range; i++) {

        //if specified, save the rng state before each game
        if (!rng_save_path.empty() ){
            std::ofstream out(rng_save_path);
            if (out) {
                out << rng;
            } else {
                std::cerr << "Failed to open file for saving the RNG state.\n";
                exit(42);
            }
            out.close();
        }

        //iterate through all possible agent assignments
        std::vector<int> permutation(agents.size());
        for(size_t j = 0; j < agents.size(); j++)
            permutation[j] = j;

        do {
            auto played_actions = std::vector<std::vector<int>>(agents.size());
            auto individual_times = std::vector<std::vector<double>>(agents.size());

            auto* gamestate = dynamic_cast<FINITEH::Gamestate *>(random_init_state ? model.getInitialState(rng) : model.getInitialState(i));
            std::vector<double> reward_sum = std::vector<double>(model.getNumPlayers(), 0);
            std::vector<std::pair<int,int>> num_optimal_action_chosen_last_game = std::vector<std::pair<int,int>>(model.getNumPlayers(), {0,0});
            std::vector<std::vector<double>> regrets_last_game = std::vector<std::vector<double>>(model.getNumPlayers(), std::vector<double>());

            while (!gamestate->terminal) {
                //choose action
                int planning_horizon = planning_beyond_execution_horizon? horizons.second : std::min(static_cast<int>(gamestate->remaining_steps),horizons.second);
                size_t prev_remaining_steps = gamestate->remaining_steps;
                gamestate->remaining_steps = planning_horizon;
                const auto start = std::chrono::high_resolution_clock::now();
                int action = agents[permutation[gamestate->turn]]->getAction(&model, gamestate, rng);
                gamestate->remaining_steps = prev_remaining_steps;

                //update statistics
                const auto time_elapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start).count();
                times[permutation[gamestate->turn]].first += time_elapsed;
                times[permutation[gamestate->turn]].second += time_elapsed * time_elapsed;
                individual_times[permutation[gamestate->turn]].push_back(time_elapsed);
                num_actions[permutation[gamestate->turn]]++;
                auto key = std::make_pair(gamestate, action);
                bool actions_in_table =  Q_map != nullptr && Q_map->contains(key);
                if(actions_in_table) {
                    for(int comp_action : model.getActions(gamestate)) {
                        if(comp_action == action)
                            continue;
                        auto comp_key = std::make_pair(gamestate,comp_action);
                        if(Q_map->at(comp_key) > Q_map->at(key) + 1e-4) {
                            num_optimal_action_chosen[permutation[gamestate->turn]].first--;
                            num_optimal_action_chosen_last_game[permutation[gamestate->turn]].first--;
                            break;
                        }
                    }
                    num_optimal_action_chosen[permutation[gamestate->turn]].second++;
                    num_optimal_action_chosen[permutation[gamestate->turn]].first++;
                    num_optimal_action_chosen_last_game[permutation[gamestate->turn]].second++;
                    num_optimal_action_chosen_last_game[permutation[gamestate->turn]].first++;
                }

                //print current Q values
                // if (output_mode == VERBOSE && actions_in_table) {
                //     execution_model.printState(gamestate);
                //     std::cout << "Q values for player " << permutation[gamestate->turn] << " at state " << gamestate->num_moves << " and planning horizon " << planning_horizon << std::endl;
                //     for(int comp_action : execution_model.getActions(gamestate)) {
                //         auto comp_key = std::make_pair(std::make_pair(gamestate,planning_horizon),comp_action);
                //         std::cout << "Action " << comp_action << " Q value: " << Q_map->at(comp_key) << std::endl;
                //     }
                // }

                //regret
                if (actions_in_table) {
                    double regret = 0;
                    for(int comp_action : model.getActions(gamestate)) {
                        auto comp_key = std::make_pair(gamestate,comp_action);
                        regret = std::max(regret,Q_map->at(comp_key) - Q_map->at(key));
                    }
                    regrets_last_game[permutation[gamestate->turn]].push_back(regret);
                }

                //apply action
                auto rewards = model.applyAction(gamestate, action, rng, nullptr).first;
                played_actions[permutation[gamestate->turn]].push_back(action);
                for (size_t j = 0; j < rewards.size(); j++)
                    reward_sum[j] += rewards[j];
            }

            games_played++;
            for(size_t k = 0; k < reward_sum.size(); k++) {
                results[k].first += reward_sum[permutation[k]];
                results[k].second += reward_sum[permutation[k]] * reward_sum[permutation[k]];
                highest_rewards[k] = std::max(highest_rewards[k], reward_sum[permutation[k]]);
                lowest_rewards[k] = std::min(lowest_rewards[k], reward_sum[permutation[k]]);
                for(double r : regrets_last_game[permutation[k]])
                    regrets[k] += r;
            }

            outputStats(output_mode, results, regrets, highest_rewards, lowest_rewards, num_optimal_action_chosen, games_played, permutation, played_actions, individual_times, reward_sum, num_actions, regrets_last_game, times);

            delete gamestate;
        } while(std::next_permutation(permutation.begin(), permutation.end()));

        auto [lower,upper] = distr::confidence_interval(results[0].first, results[0].second, games_played, DEFAULT_CONFIDENCE, 0);
        performance_confidence_interval_length = (upper - lower) / 2.0;
     }

    auto cumulative_rewards = std::vector<double>(results.size());
    for (size_t i = 0; i < results.size(); i++)
        cumulative_rewards[i] = results[i].first;

    auto average_decision_times = std::vector<double>(times.size());
    for (size_t i = 0; i < times.size(); i++)
        average_decision_times[i] = times[i].first / num_actions[i];

    return {cumulative_rewards,average_decision_times};
}