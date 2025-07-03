
#include "../../include/Utils/ValueIteration.h"

#include <chrono>
#include <unordered_map>
#include <functional>
#include <limits>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>


bool VALUE_IT::runValueIteration(FINITEH::Model* model, int num_init_samples, double discount,std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, std::mt19937& rng, bool verbose, int time_limit) {
    auto start = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < num_init_samples; i++) {
        if (verbose)
            std::cout << "Sample:" << i << std::endl;
        auto *init_state = dynamic_cast<FINITEH::Gamestate *>(model->getInitialState(rng));
        for(int action : model->getActions(init_state)) {
            if (verbose)
                std::cout << "Action:" << action << std::endl;
            auto remaining_time = time_limit - std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count();
            if (runValueIteration(model,init_state,action,discount,Q_map,rng, verbose, start, remaining_time))
                return true;
        }
    }
    return false;
}

bool VALUE_IT::outOfTime(std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, std::chrono::time_point<std::chrono::system_clock> start, int time_limit) {
    auto current_time = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(current_time - start).count() >= time_limit) {
        //free memory
        for(auto& [key, value] : Q_map)
            delete key.first;
        Q_map.clear();
        return true;
    }
    return false;
}

bool VALUE_IT::runValueIteration(FINITEH::Model* model, FINITEH::Gamestate* branch_state, int branch_action, double discount,
    std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, std::mt19937& rng, bool verbose, std::chrono::time_point<std::chrono::system_clock> start, int time_limit) {

    assert (model->getNumPlayers() == 1 && !branch_state->terminal  && model->hasTransitionProbs());

    if(outOfTime(Q_map,start,time_limit))
        return true;

    if(Q_map.contains({branch_state,branch_action}))
        return false;

    if(verbose && Q_map.size() % 1000 == 0)
        std::cout << "Pairs determined: " << Q_map.size() << std::endl;

    //gather all possible outcomes for this action
    auto [outcomes,psum] = model->getOutcomes(branch_state,branch_action,1000); //in anycase we should be able to iterate all outcomes within a second
    if (std::fabs(1-psum) > 1e-6) {
        model->printState(branch_state);
        std::cerr << "Value Iteration: getOutcomes timed out. The environment probably has too many state-action pair successors. " << psum << " " << outcomes.size() << std::endl;
        exit(1);
    }

    //calculate the expected value of this action
    double value = 0;
    for(auto& [outcome, data] : outcomes) {
        auto [prob, reward] = data;
        auto succ = dynamic_cast<FINITEH::Gamestate *>(outcome);
        double value_succ = std::numeric_limits<double>::lowest();
        if(succ->terminal)
            value_succ = 0;
        else {
            for(int succ_action : model->getActions(succ)) {
                if(!Q_map.contains({succ,succ_action})){
                    if (runValueIteration(model,succ,succ_action, discount,Q_map,rng, verbose, start, time_limit))
                        return true;
                }
                value_succ = std::max(value_succ,Q_map.at({succ,succ_action}));
            }
        }
        value += prob * ( reward[0] + discount * value_succ);
        delete succ;
    }

    Q_map.insert({{dynamic_cast<FINITEH::Gamestate *>(model->copyState(branch_state)),branch_action},value});

    return false;
}


void VALUE_IT::saveQTable(const std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>& Q_map, const std::string& filename, bool free_memory) {
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Could not open the file for writing!" << std::endl;
        return;
    }

    for (const auto& [key,value] : Q_map) {
        std::string str = key.first->ground_state->toString();
        str += std::string(" ") + std::to_string(key.first->remaining_steps) + std::string(" ") + std::to_string(key.second) + std::string(" ") + std::to_string(value);
        outFile << str << std::endl; // Write the string followed by a newline
        if(free_memory)
            delete key.first;
    }

    outFile.close();
}

void VALUE_IT::loadQTable(ABS::Model* model, std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, QMapHash, QMapCompare>* Q_map, const std::string& filename) {
    if (dynamic_cast<FINITEH::Model*>(model) != nullptr) {
        std::cerr << "Error: Model must not be FINITEH Model. Unwrap it first." << std::endl;
        return;
    }

    std::ifstream inFile(filename);
    if (!inFile) {
        std::cerr << "Error: Could not open the file for reading! Qtable is therefore not loaded." << std::endl;
        return;
    }
    //std::cout << "Loading Q-table from " << filename << std::endl;
    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string state_str, remaining_steps, action, value;
        iss >> state_str >> remaining_steps >> action >> value;
        auto ostate = FINITEH::Model::wrapState(model->deserialize(state_str),std::stoi(remaining_steps));
        Q_map->insert({{dynamic_cast<FINITEH::Gamestate *>(ostate),std::stoi(action)},std::stof(value)});
    }

    if (inFile.bad()) {
        std::cerr << "Error: An error occurred while reading the file!" << std::endl;
        exit(1);
    }

    inFile.close();
}