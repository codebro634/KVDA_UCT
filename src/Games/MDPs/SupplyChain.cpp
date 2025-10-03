// //////////////////////////////////////////////////////////////////
// A supply chain control problem in which the goal is to manage
// the inventory levels at multiple warehouses. Items can be produced
// at a factory location and then shipped to a network of warehouses
// to meet demand. Demand is a nonstationary function that depends
// on time and contains stochastic shocks, i.e. for warehouse i at time t:
//
// 		demand(i,t) = floor[D/2 * sin[2 pi (t + 2i) / 12] + D/2 + e(i,t)]
//
// where D is an upper level of demand, and e(i, t) is a Bernoulli(0.5)
// random variable.
//
// References:
//
//		Kemmer, L., von Kleist, H., de Rochebouët, D., Tziortziotis,
//		N., & Read, J. (2018, October). Reinforcement learning for supply
//		chain optimization. In European Workshop on Reinforcement Learning
//		(Vol. 14, No. 10).
//
// This is a c++ adaptation of
// 'https://github.com/pyrddlgym-project/rddlrepository/blob/2d3fd59cd4127e04327c22b174c6e3f0d0eb23cf/rddlrepository/archive/or/SupplyChain/domain.rddl'
// originally by author(s):
// 		Mike Gimelfarb (mgimelfarb@yahoo.ca)
// //////////////////////////////////////////////////////////////////

#include <bits/atomic_base.h>
#include <numbers>
#include "../../../include/Games/MDPs/SupplyChain.h"
#include <iostream>
#include <cassert>
#include <complex>
#include <fstream>
#include <functional>
using namespace std;


namespace SUPPLY_CHAIN {
    void print_symbol(const char symbol, const int times) {
        for (int i = 0; i < times; i++)
            std::cout << symbol;
    }

    void print_bar(const int stock, const int capacity) {
        const int out_bar_minuses = -min(capacity + stock, 0);
        const int in_bar_pluses = min(max(stock, 0), capacity);
        const int in_bar_minuses = min(max(-stock, 0), capacity);
        const int in_bar_space = capacity - in_bar_pluses - in_bar_minuses;
        const int out_bar_pluses = max(stock - capacity, 0);
        std::cout << '|';
        print_symbol('-', in_bar_minuses);
        print_symbol('+', in_bar_pluses);
        print_symbol(' ', in_bar_space);
        std::cout << '|';
        print_symbol('-', out_bar_minuses);
        print_symbol('+', out_bar_pluses);
    }
}


using namespace SUPPLY_CHAIN;


std::vector<int> Model::obsShape() const {
    return {2, static_cast<int>(warehouses.size())};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    const Gamestate* casted_state = dynamic_cast<SUPPLY_CHAIN::Gamestate*>(uncasted_state);
    assert (casted_state != nullptr);
    for (unsigned int i = 0; i < warehouses.size(); i++) {
        obs[i] = casted_state->warehouses[i].demand;
        obs[warehouses.size() + i] = casted_state->warehouses[i].stock;
    }
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    // an action consists of:
    //  - a number to produce: 0 - produce_max
    std::vector<int> res = {produce_max};
    //  - a number to ship to each warehouse: 0 - capacity_factory;
    for (size_t i = 0; i < warehouses.size(); i++)
        res.push_back(capacity_factory);
    return res;
}

[[nodiscard]] std::string Gamestate::toString() const {
    std::stringstream ss;
    ss << "((" << epoch << ',' << stock_factory;
    for (auto warehouse : warehouses) {
        ss << ',' << warehouse.stock;
        ss << ',' << warehouse.demand;
    }
    ss << ")," << ABS::Gamestate::toString() << ")";
    return ss.str();
}



ABS::Gamestate* Model::deserialize(std::string &ostring) const {
    vector<int> temp_nr_buffer;
    int temp_nr;
    std::istringstream iss(ostring);
    char c1, c2;
    iss >> c1 >> c2;
    assert (c1 == '(' and c2 == '(');
    do {
        iss >> temp_nr;
        temp_nr_buffer.push_back(temp_nr);
        iss >> c1;
    } while (c1 == ',');
    assert (c1 == ')');
    iss >> c1 >> c2;
    assert (c1 == ',' and c2 == '(');
    int turn, terminal;
    iss >> turn >> c1 >> terminal >> c2;
    assert (c1 == ',' and c2 == ')');
    iss >> c1;
    assert (c1 == ')');

    assert (temp_nr_buffer.size() >= 2);
    assert (temp_nr_buffer.size() % 2 == 0);

    unsigned int read_index = 0;
    auto* state = new SUPPLY_CHAIN::Gamestate();
    state->epoch = temp_nr_buffer[read_index++];
    state->stock_factory = temp_nr_buffer[read_index++];
    state->warehouses = {};
    while (read_index < temp_nr_buffer.size()) {
        FluentWarehouseData warehouse;
        warehouse.stock = temp_nr_buffer[read_index++];
        warehouse.demand = temp_nr_buffer[read_index++];
        state->warehouses.push_back(warehouse);
    }

    state->turn = turn;
    state->terminal = terminal;
    return state;
}


Model::Model(const std::string& fileName) {
    std::ifstream file(fileName);

    if (!file.is_open()){
        std::cerr << "Could not open file " << fileName << std::endl;
        exit(1);
    }

    std::string line;

    assert (std::getline(file, line) && !(line.empty() || line == "\r"));
    assert (sscanf(
        line.c_str(),
        "%lf %lf %lf %lf %d %d %d",
        &price,
        &production_cost,
        &penalty_cost,
        &storage_cost_factory,
        &capacity_factory,
        &produce_max,
        &max_demand
    ) == 7);
    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        NonFluentWarehouseData warehouse;
        assert (sscanf(
            line.c_str(),
            "%d %lf %d %lf %d",
            &warehouse.capacity,
            &warehouse.storage_cost,
            &warehouse.truck_capacity,
            &warehouse.truck_cost,
            &warehouse.shift_periodic_demand_months
        ) == 5);
        warehouses.push_back(warehouse);
    }

    std::vector<int> action_shape = Model::actionShape();
    int action_count = 1;
    for (int options : action_shape) {
        assert (action_count < std::numeric_limits<int>::max() / options);
        action_count *= options;
    }
    actions_with_max_shipment_sum = std::vector(capacity_factory + 1, std::vector(0, 0));
    for (int action = 0; action < action_count; ++action) {
        //std::cout << static_cast<double>(action) / action_count << std::endl;
        std::vector<int> decoded_action = Model::decodeAction(action);
        int shipment_sum = 0;
        int jump_size = produce_max;
        for (size_t i = warehouses.size(); i > 0; i--) {
            if (shipment_sum > capacity_factory)
                jump_size *= capacity_factory;
            shipment_sum += decoded_action[i];
        }
        for (int i = shipment_sum; i <= capacity_factory; i++) {
            actions_with_max_shipment_sum[i].push_back(action);
        }
        if (shipment_sum > capacity_factory)
            action += jump_size - 1;
    }
}


int Model::encodeAction(int* decoded_action) {
    int res = 0;
    for (size_t i = warehouses.size(); i > 0; i--) {
        res *= capacity_factory;
        res += decoded_action[i];
    }
    res *= produce_max;
    res += decoded_action[0];
    return res;
}



std::vector<int> Model::decodeAction(int action) {
    std::vector res(warehouses.size() + 1, 0);
    res[0] = action % produce_max;
    action /= produce_max;
    for (size_t i = 1; i <= warehouses.size(); i++) {
        res[i] = action % capacity_factory;
        action /= capacity_factory;
    }
    return res;
}


bool Gamestate::operator==(const ABS::Gamestate& other) const{
    const auto* other_supply_chain = dynamic_cast<const SUPPLY_CHAIN::Gamestate*>(&other);
    return (
        other_supply_chain != nullptr &&
        epoch == other_supply_chain->epoch &&
        stock_factory == other_supply_chain->stock_factory &&
        warehouses == other_supply_chain->warehouses
    );
}

size_t Gamestate::hash() const {
    constexpr std::hash<int> hasher;
    size_t res = hasher(epoch) + 0x9e3779b9;
    res ^= hasher(stock_factory) + 0x9e3779b9 + (res << 6) + (res >> 2);
    for (const auto warehouse : warehouses) {
        res ^= hasher(warehouse.stock) + 0x9e3779b9 + (res << 6) + (res >> 2);
        res ^= hasher(warehouse.demand) + 0x9e3779b9 + (res << 6) + (res >> 2);
    }
    return res;
}

void Model::printState(ABS::Gamestate* state) {
    const Gamestate* supply_chain_state = dynamic_cast<SUPPLY_CHAIN::Gamestate*>(state);
    assert (!!supply_chain_state);
    std::cout << "Factory stock: ";
    print_bar(supply_chain_state->stock_factory, capacity_factory);
    std::cout << std::endl;
    for (unsigned int i = 0; i < warehouses.size(); i++) {
        std::cout << "Warehouse " << i << ": ";
        print_bar(supply_chain_state->warehouses[i].stock, warehouses[i].capacity);
        std::cout << std::endl << "Last demand: " << supply_chain_state->warehouses[i].demand << std::endl;
    }
}


double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const auto* supply_chainA = dynamic_cast<const SUPPLY_CHAIN::Gamestate*>(a);
    assert (!!supply_chainA);
    const auto* supply_chainB = dynamic_cast<const SUPPLY_CHAIN::Gamestate*>(b);
    assert (!!supply_chainB);
    int res = abs(supply_chainA->epoch - supply_chainB->epoch);
    res += abs(supply_chainA->stock_factory - supply_chainB->stock_factory);
    for (unsigned int i = 0; i < warehouses.size(); i++) {
        res += abs(supply_chainA->warehouses[i].stock - supply_chainB->warehouses[i].stock);
        res += abs(supply_chainA->warehouses[i].demand - supply_chainB->warehouses[i].demand);
    }
    return res;
}


ABS::Gamestate* Model::getInitialState(int num) {
    auto* res = new SUPPLY_CHAIN::Gamestate();
    switch (num) {
        default:
            assert (false);
    }
    return res;
}


ABS::Gamestate* Model::getInitialState(std::mt19937& rng) {
    auto* state = new SUPPLY_CHAIN::Gamestate();

    state->epoch = 0;
    state->stock_factory = 10;
    state->warehouses = std::vector(warehouses.size(), FluentWarehouseData {
        .stock = 0,
        .demand = 0,
    });

    return state;
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    const auto state = dynamic_cast<SUPPLY_CHAIN::Gamestate*>(uncasted_state);
    const auto new_state = new SUPPLY_CHAIN::Gamestate();
    *new_state = *state;
    return new_state;
}


std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state) {
    const Gamestate* state = dynamic_cast<Gamestate*>(uncasted_state);
    assert (!!state);
    return actions_with_max_shipment_sum[state->stock_factory];
}


//////////////////////////////////////////////////////////



std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<SUPPLY_CHAIN::Gamestate*>(uncasted_state);
    assert (!!state);
    assert(action >= 0);


    // check, that the action is valid
    const std::vector<unsigned int> bounds(state->warehouses.size(), state->stock_factory + 1);
    const std::vector<int> decoded_action = decodeAction(action);
    int ship_sum = 0;
    for (unsigned int i = 0; i < state->warehouses.size(); i++)
        ship_sum += decoded_action[i + 1];
    assert (ship_sum <= state->stock_factory);
    const int produce = decoded_action[0];


    // Update factory stock
    state->stock_factory = min(state->stock_factory + produce - ship_sum, capacity_factory);


    // Update new demand -- periodic with stochastic shocks
    double probability = 1.0;
    size_t decision_point = 0;
    for (unsigned int i = 0; i < state->warehouses.size(); i++) {
        int demand = floor(0.5 * max_demand * (1 + sin(numbers::pi * (2 * state->epoch + warehouses[i].shift_periodic_demand_months) / 12.0)));
        demand = min(demand, max_demand);
        if (demand < max_demand) {
            probability *= 0.5;
            if (decision_outcomes != nullptr) {
                if (getDecisionPoint(decision_point, 0, 1, decision_outcomes) == 1)
                    demand++;
            } else {
                if (std::uniform_int_distribution(0, 1)(rng))
                    demand++;
            }
        }
        state->warehouses[i].demand = demand;
    }


    // Update warehouse stock
    for (unsigned int i = 0; i < state->warehouses.size(); i++) {
        FluentWarehouseData  &warehouse = state->warehouses[i];
        int stock = warehouse.stock;
        stock += decoded_action[i + 1];
        stock -= warehouse.demand;
        warehouse.stock = min(stock, warehouses[i].capacity);
    }


    // Time advances
    state->epoch += 1;


    // revenue from sales, minus production cost, storage cost, penalty cost, and shipping cost
    double revenue = 0.0;
    for (unsigned int i = 0; i < state->warehouses.size(); i++) {
        // sales
        revenue += price * static_cast<double>(state->warehouses[i].demand);
        // storage cost at warehouses
        revenue -= warehouses[i].storage_cost * static_cast<double>(max(state->warehouses[i].stock, 0));
        // penalty cost for negative storage at warehouses (+=, since rhs will not be positive)
        revenue += penalty_cost * static_cast<double>(min(state->warehouses[i].stock, 0));
        // shipping costs
        revenue -= warehouses[i].truck_cost * ceil(static_cast<double>(decoded_action[i + 1]) / static_cast<double>(warehouses[i].truck_capacity));
    }
    // production cost
    revenue -= production_cost * static_cast<double>(produce);
    // storage cost at factory
    revenue -= storage_cost_factory * static_cast<double>(state->stock_factory);



    return {{revenue}, probability};
}



double Model::getMinV(int steps) const {
    double min_reward = 0.0;
    for (const auto warehouse : warehouses) {
        min_reward -= warehouse.storage_cost * warehouse.capacity;
        min_reward -= penalty_cost * steps * max_demand;
        min_reward -= warehouse.truck_cost * ceil(static_cast<double>(capacity_factory) / static_cast<double>(warehouse.truck_capacity));
    }
    min_reward -= production_cost * static_cast<double>(produce_max);
    min_reward -= storage_cost_factory * static_cast<double>(capacity_factory);
    return steps * min_reward;
}

double Model::getMaxV(int steps) const {
    return price * static_cast<double>(max_demand * steps * warehouses.size());
}