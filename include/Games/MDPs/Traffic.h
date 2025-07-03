
#ifndef TRAFFIC_H
#define TRAFFIC_H


#pragma once

#include "../Gamestate.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <random>
#include <stdexcept>
#include <cmath>

namespace TR
{

struct TrafficState : public ABS::Gamestate
{
    // Occupancy for each cell
    std::vector<bool> occupiedCells;
    std::vector<bool> signal1;
    std::vector<bool> signal2;

    bool operator==(const Gamestate& other) const override;
    [[nodiscard]] size_t hash() const override;
};

class TrafficModel : public ABS::Model
{
private:

    int numIntersections{};
    int numCells{};

    std::vector<bool> isPerimeterInputCell;
    std::vector<double> perimeterRate;  // Bernoulli param if it's perimeter input
    std::vector<bool> isPerimeterExitCell;

    std::vector<std::vector<int>> cellOutflows;
    std::vector<std::vector<int>> cellInflows;

    std::vector<std::vector<int>> flowsCellToNS;
    std::vector<std::vector<int>> flowsCellToEW;

    std::vector<int> init_occupied_cells;

    void parseModelFile(const std::string& filepath);

private:
    std::vector<int> actions;

    bool existsUnoccupiedOutflow(TrafficState& oldState, int cell);
    bool existsOccupiedInflow (TrafficState& oldState, int c);
    bool flowsIntoIntersectionAndOccupied (TrafficState& oldState, int c);
    bool canFlowThroughGreen (TrafficState& oldState, int c);

protected:

    std::pair<std::vector<double>,double>  applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
    std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

public:

    explicit TrafficModel(const std::string& filepath);
    ~TrafficModel() override = default;
    void printState(ABS::Gamestate* uncasted_state) override;
    ABS::Gamestate* getInitialState(std::mt19937& rng) override;
    ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
    int getNumPlayers() override { return 1; }
    bool hasTransitionProbs() override {return true;}

    [[nodiscard]] std::vector<int> obsShape() const override;
    void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
    [[nodiscard]] std::vector<int> actionShape() const override;
    [[nodiscard]] int encodeAction(int* decoded_action) override;
};

}

#endif
