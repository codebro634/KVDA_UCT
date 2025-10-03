// ////////////////////////////////////////////////////////////////////
// A set of bins must be filled with items while ensuring the total
// weight of each bin is not greater than some limit. Items of random
// weight are presented sequentially one at a time, and the
// goal is to add it to a designated bin.
//
// This is a c++ adaptation of
// 'https://github.com/pyrddlgym-project/rddlrepository/blob/99b41edaa1912d770e6e68ebf9e9dcbcabda9318/rddlrepository/archive/or/BinPacking/domain.rddl'
// originally by author(s):
// 		Mike Gimelfarb (mgimelfarb@yahoo.ca)
// ////////////////////////////////////////////////////////////////////

#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/BinPacking.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;

using namespace BIN_PACKING;


//Helper func
size_t Model::random_item(std::mt19937& rng) const {
    std::discrete_distribution<std::size_t> distribution(item_probabilities.begin(), item_probabilities.end());
    return distribution(rng);
}

std::vector<int> Model::obsShape() const {
    return {nr_of_bins};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    const Gamestate* casted_state = dynamic_cast<BIN_PACKING::Gamestate*>(uncasted_state);
    assert (casted_state != nullptr);
    for (int i = 0; i < nr_of_bins; i++) {
        obs[i] = casted_state->total_size_in_bin[i];
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {nr_of_bins + 1};
}

[[nodiscard]] std::string Gamestate::toString() const {
    std::stringstream ss;
    ss << "((" << current_item_size;
    for (const int i : total_size_in_bin) {
        ss << ',' << i;
    }
    ss << ")," << ABS::Gamestate::toString() << ")";
    return ss.str();
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0] - 1;
}



ABS::Gamestate* Model::deserialize(std::string &ostring) const {
    Gamestate* state = new BIN_PACKING::Gamestate();
    vector<int> temp_nr_list;

    std::istringstream iss(ostring);
    char c1, c2, c3, c4, c5;

    iss >> c1 >> c2; assert(c1 == '(' && c2 == '(');

    int temp_nr;
    do {
        iss >> temp_nr;
        temp_nr_list.push_back(temp_nr);
        iss >> c1;
    } while (c1 == ',');

    assert (c1 == ')');
    iss >> c1 >> c2 >> state->turn >> c3 >> state->terminal >> c4 >> c5;
    assert (c1 == ',' && c2 == '(' && c3 == ',' && c4 == ')' && c5 == ')');

    assert (temp_nr_list.size() == static_cast<size_t>(nr_of_bins + 1));

    state->current_item_size = temp_nr_list[0];

    state->total_size_in_bin = std::vector(temp_nr_list.begin() + 1, temp_nr_list.end());
    return state;
}


Model::Model(const std::string& fileName){
    std::ifstream file(fileName);

    if (!file.is_open()){
        std::cerr << "Could not open file " << fileName << std::endl;
        exit(1);
    }

    std::string line;
    std::vector<int> temp_bin_sizes;
    std::vector<int> temp_item_sizes;
    std::vector<double> temp_item_probabilities;


    // Read bin_sizes as decimals on single lines
    while (std::getline(file, line) && !(line.empty() || line == "\r")) {
        temp_bin_sizes.push_back(std::stoi(line));
    }

    // Read lines consisting of a decimal for item-size and a double for item-probability
    bool initial_item_read = false;
    do {
        if (line.empty() || line == "\r" || line == "\n") continue;  // Skip any additional blank lines
        int temp_item_size;
        double temp_item_probability;
        int nr_of_read_values = sscanf(line.c_str(), "%d %lf", &temp_item_size, &temp_item_probability);
        assert (nr_of_read_values == 2 or (nr_of_read_values == 1 && not initial_item_read));
        if (nr_of_read_values == 1) {
            initial_item_size = temp_item_size;
            initial_item_read = true;
            continue;
        }
        temp_item_sizes.push_back(temp_item_size);
        temp_item_probabilities.push_back(temp_item_probability);
    } while (std::getline(file, line));
    if (not initial_item_read) {
        initial_item_size = -1;
    }


    // Assign temp vectors to the member variables
    bin_capacity = temp_bin_sizes;
    nr_of_bins = temp_bin_sizes.size();
    item_sizes = temp_item_sizes;
    item_probabilities = temp_item_probabilities;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    const auto* other_bin_packing = dynamic_cast<const BIN_PACKING::Gamestate*>(&other);
    return other_bin_packing != nullptr &&
        total_size_in_bin == other_bin_packing->total_size_in_bin &&
            current_item_size == other_bin_packing->current_item_size;
}

size_t Gamestate::hash() const {
    constexpr std::hash<int> hasher;
    size_t res = hasher(current_item_size) + 0x9e3779b9;
    for (const int& size : total_size_in_bin)
        res ^= hasher(size) + 0x9e3779b9 + (res << 6) + (res >> 2);
    return res;
}

void Model::printState(ABS::Gamestate* state) {
    const Gamestate* binPackingState = dynamic_cast<BIN_PACKING::Gamestate*>(state);
    assert (!!binPackingState);

    for (int i = 0; i < nr_of_bins; i++) {
        std::cout << "bin" << i << ":\t|";
        for (int j = 0; j < binPackingState->total_size_in_bin[i]; j++) {
            std::cout << "#";
        }
        for (int j = binPackingState->total_size_in_bin[i]; j < bin_capacity[i]; j++) {
            std::cout << " ";
        }
        std::cout << "|";
        if (binPackingState->total_size_in_bin[i] == 0)
            std::cout << " *";
        std::cout << std::endl;
    }
    std::cout << "Fill heights:";
    for (int i = 0; i < nr_of_bins; i++) {
        std::cout << "\t" << binPackingState->total_size_in_bin[i] << "/" << bin_capacity[i];
    }
    std::cout << std::endl << "Size of current item: " << binPackingState->current_item_size << std::endl;
}


double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const auto* binPackingA = dynamic_cast<const BIN_PACKING::Gamestate*>(a);
    assert (!!binPackingA);
    const auto* binPackingB = dynamic_cast<const BIN_PACKING::Gamestate*>(b);
    assert (!!binPackingB);
    int res = abs(binPackingA->current_item_size - binPackingB->current_item_size);
    for (int i = 0; i < nr_of_bins; i++) {
        res += abs(binPackingA->total_size_in_bin[i] - binPackingB->total_size_in_bin[i]);
    }
    return res;
}

ABS::Gamestate* Model::getInitialState(int num) {
    auto* res = new BIN_PACKING::Gamestate();
    switch (num) {
        default:
            assert (false);
    }
    return res;
}


ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new BIN_PACKING::Gamestate();

    state->total_size_in_bin = std::vector<int>(nr_of_bins, 0);
    if (initial_item_size == -1) {
        state->current_item_size = item_sizes[random_item(rng)];
    } else {
        state->current_item_size = initial_item_size;
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
    std::vector<int> actions;
    for (int i = 0; i < nr_of_bins; i++) {
        if (state->total_size_in_bin[i] + state->current_item_size <= bin_capacity[i]) {
            actions.push_back(i);
        }
    }
    if(actions.empty())
        actions.push_back(-1);
    return actions;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<BIN_PACKING::Gamestate*>(uncasted_state);
    size_t decision_point = 0;
    double prob;

    if (action == -1) {
        // check, if ignoring is acceptable
        for (int i = 0; i < nr_of_bins; i++)
            assert (state->total_size_in_bin[i] + state->current_item_size > bin_capacity[i]);
        prob = 1.0;
    } else {
        // put current item into the bin chosen by action:
        assert (action >= 0 && action < nr_of_bins);
        assert (state->total_size_in_bin[action] + state->current_item_size <= bin_capacity[action]);
        state->total_size_in_bin[action] += state->current_item_size;

        // generate next item:
        size_t next_item;
        if (decision_outcomes == nullptr) {
            next_item = random_item(rng);
        } else {
            next_item = getDecisionPoint(decision_point, 0, item_sizes.size(), decision_outcomes);
        }
        state->current_item_size = item_sizes[next_item];

        prob = item_probabilities[next_item];
    }
    int unused_bins = 0;
    for (int i = 0; i < nr_of_bins; i++)
        if (state->total_size_in_bin[i] == 0)
            unused_bins++;

    return {{static_cast<double>(unused_bins)}, prob};
}


