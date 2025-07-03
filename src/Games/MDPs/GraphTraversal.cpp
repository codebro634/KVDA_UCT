#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/GraphTraversal.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;

using namespace GTR;


std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

Model::Model(const std::string& fileName)
{

    std::ifstream file(fileName); // Open the file
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << fileName << std::endl;
        return;
    }

    std::string line;


    //get num of vertices
    std::getline(file, line);
    int vertex_count = std::stoi(line);
    std::getline(file, line); //skip empty line

    //Parse transition graph
    for(int i = 0; i < vertex_count; i++) {
        std::getline(file, line);
        std::vector<std::vector<std::pair<int,double>>> connections;

        //check if line is empty and fill remaining vertices with empty connections
        if(line.empty() || line == "\r") {
            transition_graph.emplace_back();
            continue;
        }

        //split line by " "
        auto split = splitString(line, ' ');
        for (const auto& s : split) {
            if('[' != s[0])
                connections.push_back({{std::stoi(s),1}}); //deterministic transition
            else { //probabilistic transition
                auto connection = std::vector<std::pair<int,double>>();
                const auto conn_str = s.substr(1, s.size() - 2);
                auto conns = splitString(conn_str, ',');
                double psum = 0;
                for (const auto& c : conns) {
                    auto target_and_prob = splitString(c, ':');
                    connection.emplace_back(std::stoi(target_and_prob[0]), std::stod(target_and_prob[1]));
                    psum += std::stod(target_and_prob[1]);
                }
                assert( std::fabs(psum - 1) < 0.0001);
                connections.push_back(connection);
            }
        }

        transition_graph.push_back(connections); // Add parsed data to prereqs

    }

    //skip empty line
    std::getline(file, line);

    //Parse reward graph
    for(int i = 0; i < vertex_count; i++) {
        std::getline(file, line);
        if(line.empty() || line == "\r") {
            transition_rewards.emplace_back();
            continue;
        }

        std::istringstream iss(line); // Create a string stream for each line
        std::vector<double> rewards;
        double reward;

        // Parse floats from the line
        while (iss >> reward) {
            rewards.push_back(reward);
        }
        transition_rewards.push_back(rewards);
    }

    //skip empty line
    std::getline(file, line);

    std::getline(file, line);
    std::istringstream iss(line); // Create a string stream for each line
    int term;
    while (iss >> term)
        terms.insert(term);

    file.close(); // Close the file
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    return vertex_at == dynamic_cast<const Gamestate&>(other).vertex_at && terminal == dynamic_cast<const Gamestate&>(other).terminal;
}

size_t Gamestate::hash() const
{
    return vertex_at;
}

void Model::printState(ABS::Gamestate* state) {
    auto* AAState = dynamic_cast<GTR::Gamestate*>(state);
    if (!AAState) return;

    //print vertex at
    std::cout << "Vertex at: " << AAState->vertex_at << std::endl;

    //print probabilistic transition graph
    for (size_t i = 0; i < transition_graph[AAState->vertex_at].size(); ++i) {
        std::cout << "Action " << i << ": ";
        for (const auto& [target, prob] : transition_graph[AAState->vertex_at][i]) {
            std::cout << target << " with prob " << prob << ", ";
        }
        std::cout << std::endl;
    }
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new GTR::Gamestate();
    state->vertex_at = 0;
    return state;
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    auto* state = dynamic_cast<GTR::Gamestate*>(uncasted_state);
    if (!state) return {};

    std::vector<int> actions = {};
    for (size_t i = 0; i < transition_graph[state->vertex_at].size(); ++i) {
        actions.push_back(i);
    }

    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<GTR::Gamestate*>(uncasted_state);
    size_t decision_point = 0;

    //transition reward
    double reward = transition_rewards[state->vertex_at][action];

    //sample transition
    auto transitions = transition_graph[state->vertex_at][action];
    auto probs = std::vector<double>();
    for (const auto& [target, prob] : transitions) {
        probs.push_back(prob);
    }
    std::discrete_distribution<int> candidate_dist(probs.begin(), probs.end());
    int idx;
    if (decision_outcomes == nullptr)
        idx = candidate_dist(rng);
    else {
        std::vector<int> non_zero_probs;
        for (size_t i = 0; i < probs.size(); ++i) {
            if (probs[i] > 0)
                non_zero_probs.push_back(i);
        }
        int decision = getDecisionPoint(decision_point, 0, non_zero_probs.size()-1, decision_outcomes);
        idx = non_zero_probs[decision];
    }
    state->vertex_at = transitions[idx].first;

    //check terminality
    if (terms.contains(state->vertex_at)) {
       state->terminal = true;
    }

    return {{(double)reward}, probs[idx]};
}