// ////////////////////////////////////////////////////////////////////
// Items of various weight and value are shown sequentially one at a
// time. The item can be accepted and placed into a specific knapsack
// of fixed capacity, or discarded. The goal is to maximize the value
// stored across all knapsacks.
//
// This is a c++ adaptation of
// 'https://github.com/pyrddlgym-project/rddlrepository/blob/99b41edaa1912d770e6e68ebf9e9dcbcabda9318/rddlrepository/archive/or/Knapsack/domain.rddl'
// originally by author(s):
// 		Mike Gimelfarb (mgimelfarb@yahoo.ca)
// ////////////////////////////////////////////////////////////////////

#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/Knapsack.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;

using namespace KNAPSACK;


//Helper func
int Model::random_item(std::mt19937& rng) const {
    std::discrete_distribution<std::size_t> distribution(item_probabilities.begin(), item_probabilities.end());
    return distribution(rng);
}

std::vector<int> Model::obsShape() const {
    return {2, nr_of_knapsacks};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    const Gamestate* casted_state = dynamic_cast<KNAPSACK::Gamestate*>(uncasted_state);
    assert (casted_state != nullptr);
    for (int i = 0; i < nr_of_knapsacks; i++) {
        obs[i] = casted_state->total_knapsack_values[i];
        obs[nr_of_knapsacks + i] = casted_state->total_knapsack_weights[i];
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {1 + nr_of_knapsacks};
}

[[nodiscard]] std::string Gamestate::toString() const {
    std::stringstream ss;
    ss << "((" << current_item_weight << ',' << current_item_value;
    for (size_t i = 0; i < total_knapsack_weights.size(); i++) {
        ss << ',' << total_knapsack_weights[i] << ',' << total_knapsack_values[i];
    }
    ss << ")," << ABS::Gamestate::toString() << ")";
    return ss.str();
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0]-1;
}



ABS::Gamestate* Model::deserialize(std::string &ostring) const {

    Gamestate* state = new KNAPSACK::Gamestate();
    vector<int> temp_nr_buffer;

    std::istringstream iss(ostring);
    char c1, c2, c3, c4, c5;

    iss >> c1 >> c2; assert(c1 == '(' && c2 == '(');

    int temp_total_size_element;
    do {
        iss >> temp_total_size_element;
        temp_nr_buffer.push_back(temp_total_size_element);
        iss >> c1;
    } while (c1 == ',');

    assert (c1 == ')');
    iss >> c1 >> c2 >> state->turn >> c3 >> state->terminal >> c4 >> c5;
    assert (c1 == ',' && c2 == '(' && c3 == ',' && c4 == ')' && c5 == ')');

    assert (temp_nr_buffer.size() >= 2);
    assert (temp_nr_buffer.size() % 2 == 0);

    state->current_item_weight = temp_nr_buffer[0];
    state->current_item_value = temp_nr_buffer[1];

    for (size_t i = 2; i < temp_nr_buffer.size();) {
        state->total_knapsack_weights.push_back(temp_nr_buffer[i++]);
        state->total_knapsack_values.push_back(temp_nr_buffer[i++]);
    }
    return state;
}


Model::Model(const std::string& fileName){
    std::ifstream file(fileName);

    if (!file.is_open()){
        std::cerr << "Could not open file " << fileName << std::endl;
        exit(1);
    }

    std::string line;
    std::vector<int> temp_knapsack_capacities;
    std::vector<int> temp_item_weights;
    std::vector<int> temp_item_values;
    std::vector<double> temp_item_probabilities;


    // Read knapsack capacities as decimals on single lines
    while (std::getline(file, line) && !(line.empty() || line == "\r")) {
        temp_knapsack_capacities.push_back(std::stoi(line));
    }

    // Read lines consisting of a decimal for item-weights, item-values and a double for item-probability
    bool default_item_initialized = false;
    do {
        if (line.empty() || line == "\r" || line == "\n") continue;  // Skip any additional blank lines
        int temp_item_weight, temp_item_value;
        double temp_item_probability;
        int read_nrs = sscanf(line.c_str(), "%d %d %lf", &temp_item_weight, &temp_item_value, &temp_item_probability);
        assert (read_nrs == 3 or (read_nrs == 2 and not default_item_initialized));
        if (read_nrs == 2) {
            initial_item_weight = temp_item_weight;
            initial_item_value = temp_item_value;
            default_item_initialized = true;
            continue;
        }
        temp_item_weights.push_back(temp_item_weight);
        temp_item_values.push_back(temp_item_value);
        temp_item_probabilities.push_back(temp_item_probability);
    } while (std::getline(file, line));
    if (not default_item_initialized) {
        initial_item_weight = -1;
        initial_item_value = -1;
    }


    // Assign temp vectors to the member variables
    nr_of_knapsacks = temp_knapsack_capacities.size();
    knapsack_capacity = temp_knapsack_capacities;
    item_weights = temp_item_weights;
    item_values = temp_item_values;
    item_probabilities = temp_item_probabilities;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    const auto* other_knapsack = dynamic_cast<const KNAPSACK::Gamestate*>(&other);
    return (
        other_knapsack != nullptr &&
        total_knapsack_weights == other_knapsack->total_knapsack_weights &&
        total_knapsack_values == other_knapsack->total_knapsack_values &&
        current_item_weight == other_knapsack->current_item_weight &&
        current_item_value == other_knapsack->current_item_value
    );
}

size_t Gamestate::hash() const {
    constexpr std::hash<int> hasher;
    size_t res = hasher(current_item_weight) + 0x9e3779b9;
    for (const int& size : total_knapsack_weights)
        res ^= hasher(size) + 0x9e3779b9 + (res << 6) + (res >> 2);
    res ^= hasher(current_item_value) + 0x9e3779b9 + (res << 6) + (res >> 2);
    for (const int& size : total_knapsack_values)
        res ^= hasher(size) + 0x9e3779b9 + (res << 6) + (res >> 2);
    return res;
}

void Model::printState(ABS::Gamestate* state) {
    const Gamestate* knapsack_state = dynamic_cast<KNAPSACK::Gamestate*>(state);
    assert (!!knapsack_state);

    for (int i = 0; i < nr_of_knapsacks; i++) {
        std::cout << "knapsack nr. " << i << ':' << std::endl;
        int value_sum = knapsack_state->total_knapsack_values[i];
        int weight_sum = knapsack_state->total_knapsack_weights[i];
        do {
            int layer = value_sum;
            if (weight_sum != 0)
                layer = ((value_sum - 1) % weight_sum) + 1;
            value_sum -= layer;
            std::cout << '|';
            for (int j = 0; j < weight_sum - layer; j++) std::cout << ' ';
            for (int j = weight_sum - layer; j < weight_sum; j++) std::cout << '#';
            for (int j = weight_sum; j < knapsack_capacity[i]; j++) std::cout << ' ';
            std::cout << '|' << std::endl;
        } while (value_sum > 0);
    }
    std::cout << "value/weight/capacity:";
    for (int i = 0; i < nr_of_knapsacks; i++) {
        std::cout << "\t" << knapsack_state->total_knapsack_values[i];
        std::cout << '/' << knapsack_state->total_knapsack_weights[i];
        std::cout << '/' << knapsack_capacity[i];
    }
    std::cout << std::endl << "current item:" << std::endl;
    int remaining_value = knapsack_state->current_item_value;
    while (remaining_value > 0) {
        int layer = ((remaining_value - 1) % knapsack_state->current_item_weight) + 1;
        remaining_value -= layer;
        std::cout << '|';
        for (int j = 0; j < knapsack_state->current_item_weight - layer; j++) std::cout << ' ';
        for (int j = knapsack_state->current_item_weight - layer; j < knapsack_state->current_item_weight; j++) std::cout << '#';
        std::cout << '|' << std::endl;
    }
}


double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const auto* knapsackA = dynamic_cast<const KNAPSACK::Gamestate*>(a);
    assert (!!knapsackA);
    const auto* knapsackB = dynamic_cast<const KNAPSACK::Gamestate*>(b);
    assert (!!knapsackB);
    int res = abs(knapsackA->current_item_value - knapsackB->current_item_value);
    res += abs(knapsackA->current_item_weight - knapsackB->current_item_weight);
    for (int i = 0; i < nr_of_knapsacks; i++) {
        res += abs(knapsackA->total_knapsack_weights[i] - knapsackB->total_knapsack_weights[i]);
        res += abs(knapsackA->total_knapsack_values[i] - knapsackB->total_knapsack_values[i]);
    }
    return res;
}

ABS::Gamestate* Model::getInitialState(int num) {
    auto* res = new KNAPSACK::Gamestate();
    switch (num) {
        default:
            assert (false);
    }
    return res;
}


ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new KNAPSACK::Gamestate();
    state->total_knapsack_weights = std::vector<int>(nr_of_knapsacks, 0);
    state->total_knapsack_values = std::vector<int>(nr_of_knapsacks, 0);

    if (initial_item_value == -1 and initial_item_weight == -1) {
        size_t current_item = random_item(rng);
        state->current_item_weight = item_weights[current_item];
        state->current_item_value = item_values[current_item];
    } else {
        assert (initial_item_value >= 0 and initial_item_weight >= 0);
        state->current_item_value = initial_item_value;
        state->current_item_weight = initial_item_weight;
    }

    return state;
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    const auto state = dynamic_cast<Gamestate*>(uncasted_state);
    const auto new_state = new Gamestate();
    *new_state = *state;
    return new_state;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    const Gamestate* state = dynamic_cast<Gamestate*>(uncasted_state);
    std::vector<int> actions = {-1};
    for (int i = 0; i < nr_of_knapsacks; i++) {
        if (state->total_knapsack_weights[i] + state->current_item_weight <= knapsack_capacity[i]) {
            actions.push_back(i);
        }
    }
    return actions;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<KNAPSACK::Gamestate*>(uncasted_state);

    if (action != -1) {
        // put current item into the knapsack chosen by action:
        assert(action >= 0 && action < nr_of_knapsacks);
        assert (state->total_knapsack_weights[action] + state->current_item_weight <= knapsack_capacity[action]);
        state->total_knapsack_weights[action] += state->current_item_weight;
        state->total_knapsack_values[action] += state->current_item_value;
    }

    // generate next item:
    int next_item;
    if (decision_outcomes == nullptr) {
        next_item = random_item(rng);
    } else {
        size_t decision_point = 0;
        next_item = getDecisionPoint(decision_point, 0, item_weights.size(), decision_outcomes);
    }
    state->current_item_weight = item_weights[next_item];
    state->current_item_value = item_values[next_item];

    double prob = item_probabilities[next_item];

    int total_value_sum = 0; // aka reward
    for (int i = 0; i < nr_of_knapsacks; i++)
        total_value_sum += state->total_knapsack_values[i];

    return {{static_cast<double>(total_value_sum)}, prob};
}

[[nodiscard]] double Model::getMaxV(int steps) const {
    double max_value_per_weight = 0.0;
    for (size_t i = 0; i < item_values.size(); i++) {
        double value_per_weight = static_cast<double>(item_values[i]) / static_cast<double>(item_weights[i]);
        max_value_per_weight = std::max(max_value_per_weight, value_per_weight);
    }
    const int total_capacity = std::reduce(knapsack_capacity.begin(), knapsack_capacity.end(), 0);
    return max_value_per_weight * static_cast<double>(total_capacity * steps);
}