#pragma once

#ifndef SUPPLY_CHAIN_H
#define SUPPLY_CHAIN_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace SUPPLY_CHAIN
{

    struct FluentWarehouseData {
        int stock = 0;
        int demand = 0;

        bool operator==(const FluentWarehouseData & other) const {
            return stock == other.stock && demand == other.demand;
        }
    };

    struct Gamestate: public ABS::Gamestate{
        int epoch = 0;
        int stock_factory = 10;
        std::vector<FluentWarehouseData> warehouses;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override;
    };


    struct NonFluentWarehouseData {
        int capacity = 100;
        double storage_cost = 0.1;
        int truck_capacity = 5;
        double truck_cost = 0.05;
        int shift_periodic_demand_months = 0;
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* getInitialState(int num) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] ABS::Gamestate* deserialize(std::string &ostring) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;



        [[nodiscard]] double getMinV(int steps) const override;
        [[nodiscard]] double getMaxV(int steps) const override;
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

        [[nodiscard]] std::vector<int> decodeAction(int action) override;

    private:
        double price = 1.0;
        double production_cost = 0.1;
        double penalty_cost = 0.5;
        double storage_cost_factory = 0.1;

        int capacity_factory = 100;
        int produce_max = 50;

        int max_demand = 20;

        std::vector<NonFluentWarehouseData> warehouses;

        std::vector<std::vector<int>> actions_with_max_shipment_sum;


    protected:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}