// ////////////////////////////////////////////////////////////////////
// The travelling salesman problem involves finding a least cost route
// that visits each city exactly once. In this slightly relaxed version
// the reward includes a fuel cost of traversing from one city to another
// and a bonus if all the cities have previously been visited (including
// if they were visited more than once). Due to the structure of the
// reward, an optimal policy should visit each city exactly once.
//
// This is a c++ adaptation of
// 'https://github.com/pyrddlgym-project/rddlrepository/blob/0943222bdba46a8679c60c2407361aca9de010ca/rddlrepository/archive/or/TSP/domain.rddl'
// originally by author(s):
// 		Mike Gimelfarb (mgimelfarb@yahoo.ca)
// ////////////////////////////////////////////////////////////////////


#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/TravellingSalesPerson.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;

using namespace TRAVELLING_SALES_PERSON;



std::vector<int> Model::obsShape() const {
    return {2, nr_of_nodes};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    const Gamestate* casted_state = dynamic_cast<TRAVELLING_SALES_PERSON::Gamestate*>(uncasted_state);
    assert (casted_state != nullptr);
    for (int i = 0; i < nr_of_nodes; i++) {
        obs[i] = casted_state->current_node == i ? 1 : 0;
        obs[nr_of_nodes + i] = casted_state->has_been_visited(i) ? 1 : 0;
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    return {nr_of_nodes + 1};
}

[[nodiscard]] std::string Gamestate::toString() const {
    std::stringstream ss;
    ss << "((" << current_node << ',' << visited_nodes << ")," << ABS::Gamestate::toString() << ")";
    return ss.str();
}

int Model::encodeAction(int* decoded_action) {
    return decoded_action[0] - 1;
}



ABS::Gamestate* Model::deserialize(std::string &ostring) const {
    auto* state = new TRAVELLING_SALES_PERSON::Gamestate();

    int terminal;
    if (sscanf(
        ostring.c_str(), "((%d,%lld),(%d,%d))",
        &state->current_node, &state->visited_nodes,
        &state->turn, &terminal
    ) != 4)
        throw std::runtime_error("Failed to deserialize Gamestate: " + ostring);
    state->terminal = terminal;

    return state;
}


Model::Model(const std::string& fileName){
    std::ifstream file(fileName);

    if (!file.is_open()){
        std::cerr << "Could not open file " << fileName << std::endl;
        exit(1);
    }

    std::vector<double> distance_buffer;
    int temp_nr_of_nodes, temp_origin;

    file >> temp_nr_of_nodes >> temp_origin;

    double temp_number;
    while (file >> temp_number)
        distance_buffer.push_back(temp_number);

    assert (temp_nr_of_nodes > 0);
    assert (temp_origin >= 0 && temp_origin < temp_nr_of_nodes);
    assert (distance_buffer.size() == temp_nr_of_nodes * temp_nr_of_nodes);

    nr_of_nodes = temp_nr_of_nodes;
    origin = temp_origin;
    costs = distance_buffer;
}

bool Gamestate::operator==(const ABS::Gamestate& other) const{
    const auto* other_bin_packing = dynamic_cast<const TRAVELLING_SALES_PERSON::Gamestate*>(&other);
    return (
        other_bin_packing != nullptr &&
        current_node == other_bin_packing->current_node &&
        visited_nodes == other_bin_packing->visited_nodes
    );
}

size_t Gamestate::hash() const {
    constexpr std::hash<long long> hasher;
    size_t res = hasher(current_node) + 0x9e3779b9;
    res ^= hasher(visited_nodes) + 0x9e3779b9 + (res << 6) + (res >> 2);
    return res;
}

void Model::printState(ABS::Gamestate* state) {
    const Gamestate* tspState = dynamic_cast<TRAVELLING_SALES_PERSON::Gamestate*>(state);
    assert (!!tspState);

    std::cout << "Positions:";
    for (int i = 0; i < nr_of_nodes; i++) {
        std::cout << '\t';
        if (tspState->has_been_visited(i))
            std::cout << (tspState->current_node == i ? 'X' : '_');
        else
            std::cout << i;
    }
    std::cout << std::endl;

    std::cout << "Costs    :";
    for (int i = 0; i < nr_of_nodes; i++) {
        std::cout << '\t';
        if (tspState->has_been_visited(i))
            std::cout << ' ';
        else
            std::cout << get_cost(tspState->current_node, i);
    }
    std::cout << std::endl;
}


double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const auto* tspA = dynamic_cast<const TRAVELLING_SALES_PERSON::Gamestate*>(a);
    assert (!!tspA);
    const auto* tspB = dynamic_cast<const TRAVELLING_SALES_PERSON::Gamestate*>(b);
    assert (!!tspB);

    return __builtin_popcount(tspA->visited_nodes ^ tspB->visited_nodes) +
        (tspA->current_node == tspB->current_node ? 0 : 1);
}

ABS::Gamestate* Model::getInitialState(int num) {
    auto* res = new TRAVELLING_SALES_PERSON::Gamestate();
    switch (num) {
        default:
            assert (false);
    }
    return res;
}


ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    auto* state = new TRAVELLING_SALES_PERSON::Gamestate();
    state->current_node = origin;
    state->visited_nodes = 1 << origin;
    return state;
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    const auto state = dynamic_cast<Gamestate*>(uncasted_state);
    assert (!!state);
    const auto new_state = new Gamestate();
    *new_state = *state;
    return new_state;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    const Gamestate* state = dynamic_cast<Gamestate*>(uncasted_state);
    assert (!!state);
    if (state->visited_nodes == (1 << nr_of_nodes) - 1)
        return {-1};
    std::vector<int> actions;
    for (int i = 0; i < nr_of_nodes; i++)
        if (!state->has_been_visited(i))
            actions.push_back(i);
    return actions;
}


std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<TRAVELLING_SALES_PERSON::Gamestate*>(uncasted_state);
    assert (!!state);

    if (action == -1) {
        assert (state->visited_nodes == (1 << nr_of_nodes) - 1);
        return {{0.0}, 1.0};
    }

    assert (action >= 0 && action < nr_of_nodes);
    assert (!state->has_been_visited(action));
    double cost = get_cost(state->current_node, action);
    state->current_node = action;
    state->set_visited_node(action);

    return {{-cost}, 1.0};
}