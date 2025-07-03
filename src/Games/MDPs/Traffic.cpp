#include "../../../include/Games/MDPs/Traffic.h"

namespace TR
{

    std::vector<int> TrafficModel::obsShape() const{
        return {numCells + numIntersections * 2};
    }

    void TrafficModel::getObs(ABS::Gamestate* uncasted_state, int* obs) {
        auto state = dynamic_cast<TrafficState*>(uncasted_state);
        assert (numCells == (int)state->occupiedCells.size());
        for (int i = 0; i < numCells; i++)
            obs[i] = state->occupiedCells[i] ? 1 : 0;
        for (int i = 0; i < numIntersections; i++){
            obs[numCells + i * 2] = state->signal1[i] ? 1 : 0;
            obs[numCells + i * 2 + 1] = state->signal2[i] ? 1 : 0;
        }
    }

    [[nodiscard]] std::vector<int> TrafficModel::actionShape() const {
        auto shape = std::vector<int>();
        for (int i = 0; i < numIntersections; i++)
            shape.push_back(2);
        return shape;
    }

    int TrafficModel::encodeAction(int* decoded_action) {
        int pow = 1;
        int action = 0;
        for (int i = 0; i < numIntersections; i++){
            if (decoded_action[i] == 1)
                action += pow;
            pow *= 2;
        }
        return action;
    }

    bool TrafficState::operator==(const Gamestate& other) const
    {
        // Attempt to cast
        auto const* o = dynamic_cast<TrafficState const*>(&other);
        if (!o) return false;

        if (occupiedCells.size() != o->occupiedCells.size()) return false;
        for (size_t i = 0; i < occupiedCells.size(); i++)
        {
            if (occupiedCells[i] != o->occupiedCells[i]) return false;
        }

        if (signal1.size() != o->signal1.size()) return false;
        if (signal2.size() != o->signal2.size()) return false;
        for (size_t i = 0; i < signal1.size(); i++)
        {
            if (signal1[i] != o->signal1[i]) return false;
            if (signal2[i] != o->signal2[i]) return false;
        }
        return true;
    }

    [[nodiscard]] size_t TrafficState::hash() const
    {
        size_t h = 0x9e3779b97f4a7c15ULL;
        auto mix = [&](size_t val) {
            val ^= (val << 13);
            val ^= (val >> 7);
            val ^= (val << 17);
            h ^= val + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        };

        for (bool b : occupiedCells)
        {
            mix(std::hash<bool>()(b));
        }
        for (size_t i = 0; i < signal1.size(); i++)
        {
            mix(std::hash<bool>()(signal1[i]));
            mix(std::hash<bool>()(signal2[i]));
        }
        return h;
    }


void TrafficModel::parseModelFile(const std::string& filepath){
    //
    // We'll assume a simplistic file format:
    //  line1: <numIntersections>
    //  line2: <numCells>
    //  line3: <number_of_input_cells>
    //         next lines (that many): "cellIndex inputRate"
    //  line?: <number_of_exit_cells>
    //         next lines (that many): "cellIndex"
    //  line?: <number_of_flows_cell_to_cell>
    //         next lines (that many): "fromCell toCell"
    //  line?: <number_of_flows_cell_to_intersection_NS>
    //         next lines (that many): "cell intersection"
    //  line?: <number_of_flows_cell_to_intersection_EW>
    //         next lines (that many): "cell intersection"
    //  line?: <number_of_initially_occupied_cells>
    //         next lines (that many): "cells"
    //
    // Example:
    //  2
    //  5
    //  1
    //  0 0.7
    //  1
    //  4
    //  0 1
    //  1 2
    //  2 3
    //  3 4
    //  1
    //  0 0
    //  1
    //  1 1
    // 0

    std::ifstream ifs(filepath);
    if (!ifs.is_open())
    {
        throw std::runtime_error("Could not open model file: " + filepath);
    }

    // Basic top-level counts
    ifs >> numIntersections;
    ifs >> numCells;

    // Initialize data structures
    isPerimeterInputCell.resize(numCells, false);
    perimeterRate.resize(numCells, 0.0);
    isPerimeterExitCell.resize(numCells, false);
    cellOutflows.resize(numCells);
    cellInflows.resize(numCells);
    flowsCellToNS.resize(numCells);
    flowsCellToEW.resize(numCells);

    // Parse input cells
    int nInputCells;
    ifs >> nInputCells;
    for(int i = 0; i < nInputCells; i++)
    {
        int cellIdx;
        double rate;
        ifs >> cellIdx >> rate;
        isPerimeterInputCell[cellIdx] = true;
        perimeterRate[cellIdx] = rate;
    }

    // Parse exit cells
    int nExitCells;
    ifs >> nExitCells;
    for(int i = 0; i < nExitCells; i++)
    {
        int cellIdx;
        ifs >> cellIdx;
        isPerimeterExitCell[cellIdx] = true;
    }

    // Parse flows from cell -> cell
    int nFlowsCellToCell;
    ifs >> nFlowsCellToCell;
    for(int i = 0; i < nFlowsCellToCell; i++)
    {
        int fromC, toC;
        ifs >> fromC >> toC;
        cellOutflows[fromC].push_back(toC);
        cellInflows[toC].push_back(fromC);
    }

    // Parse flows from cell -> intersection (NS)
    int nFlowsNS;
    ifs >> nFlowsNS;
    for(int i = 0; i < nFlowsNS; i++)
    {
        int cellIdx, interIdx;
        ifs >> cellIdx >> interIdx;
        flowsCellToNS[cellIdx].push_back(interIdx);
    }

    // Parse flows from cell -> intersection (EW)
    int nFlowsEW;
    ifs >> nFlowsEW;
    for(int i = 0; i < nFlowsEW; i++)
    {
        int cellIdx, interIdx;
        ifs >> cellIdx >> interIdx;
        flowsCellToEW[cellIdx].push_back(interIdx);
    }

    //parse initially occupied cells
    int nInitOccupiedCells;
    ifs >> nInitOccupiedCells;
    for(int i = 0; i < nInitOccupiedCells; i++)
    {
        int cellIdx;
        ifs >> cellIdx;
        init_occupied_cells.push_back(cellIdx);
    }

    int total = (1 << numIntersections); // 2^numIntersections
    for (int a = 0; a < total; a++)
        actions.push_back(a);
}


TrafficModel::TrafficModel(const std::string& filepath)
{
    parseModelFile(filepath);
}

void TrafficModel::printState(ABS::Gamestate* uncasted_state)
{
    auto* st = dynamic_cast<TrafficState*>(uncasted_state);
    if (!st)
    {
        throw std::runtime_error("printState: Invalid state type.");
    }
    std::ostringstream oss;
    // Occupancy
    oss << "Occupied cells: ";
    for (size_t i = 0; i < st->occupiedCells.size(); i++)
    {
        oss << (st->occupiedCells[i] ? "1" : "0");
        if (i < st->occupiedCells.size() - 1) oss << ",";
    }
    oss << "\nSignals:\n";
    for (size_t i = 0; i < st->signal1.size(); i++)
    {
        oss << " Intersection " << i << ": ("
            << (st->signal1[i] ? 1 : 0) << ","
            << (st->signal2[i] ? 1 : 0) << ")\n";
    }
    std::cout << oss.str() << std::endl;
}


ABS::Gamestate* TrafficModel::getInitialState(std::mt19937& /*rng*/)
{
    auto* st = new TrafficState();

    st->occupiedCells.resize(numCells, false);
    for (int c : init_occupied_cells)
        st->occupiedCells[c] = true;
    st->signal1.resize(numIntersections, false);
    st->signal2.resize(numIntersections, false);

    return st;
}


ABS::Gamestate* TrafficModel::copyState(ABS::Gamestate* uncasted_state){
        auto state = dynamic_cast<TrafficState*>(uncasted_state);
        auto new_state = new TrafficState();
        *new_state = *state; //default copy constructor should work
        return new_state;
}


std::vector<int> TrafficModel::getActions_(ABS::Gamestate* /*uncasted_state*/){
    return actions;
}


inline bool TrafficModel::existsUnoccupiedOutflow(TrafficState& oldState, int cell) {
    for (int c2 : cellOutflows[cell])
        if (!oldState.occupiedCells[c2]) return true;
    return false;
}


inline bool TrafficModel::existsOccupiedInflow (TrafficState& oldState, int c) {
    for (int c2 : cellInflows[c])
        if (oldState.occupiedCells[c2]) return true;
    return false;
}


inline bool TrafficModel::flowsIntoIntersectionAndOccupied (TrafficState& oldState, int c) {
    if (!oldState.occupiedCells[c]) return false;
    return !flowsCellToNS[c].empty() || !flowsCellToEW[c].empty();
}

bool TrafficModel::canFlowThroughGreen(TrafficState& oldState, int c) {
    bool conditionNS = false;
    bool conditionEW = false;

    bool anyUnoccupiedOutflow = existsUnoccupiedOutflow(oldState,c);
    if(!anyUnoccupiedOutflow) return false;

    // NS?
    for (int i : flowsCellToNS[c]){
        bool s1 = oldState.signal1[i];
        bool s2 = oldState.signal2[i];
        if (s2 && !s1){
            conditionNS = true;
            break;
        }
    }
    // EW?
    for (int i : flowsCellToEW[c]){
        bool s1 = oldState.signal1[i];
        bool s2 = oldState.signal2[i];
        if (s1 && !s2){
            conditionEW = true;
            break;
        }
    }
    return (conditionNS || conditionEW);
}

std::pair<std::vector<double>, double>
TrafficModel::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes)
{
    size_t decision_point = 0;
    auto* currentState = dynamic_cast<TrafficState*>(uncasted_state);
        
    TrafficState oldState = *currentState;

    // For safety, we reset them here so we don't accidentally reuse old occupant values.
    std::fill(currentState->occupiedCells.begin(), currentState->occupiedCells.end(), false);
    std::fill(currentState->signal1.begin(), currentState->signal1.end(), false);
    std::fill(currentState->signal2.begin(), currentState->signal2.end(), false);

    // 1) Decode action into booleans "advance(i)" for each intersection i.
    std::vector<bool> advanceIntersections(numIntersections, false);
    for(int i = 0; i < numIntersections; i++)
    {
        // If the i-th bit of action is set => advance
        int bit = (action >> i) & 1;
        advanceIntersections[i] = (bit == 1);
    }

    //Update signals :
    for(int i = 0; i < numIntersections; i++)
    {
        bool s1 = oldState.signal1[i];
        bool s2 = oldState.signal2[i];

        bool bothTrue  = (s1 && s2);
        bool bothFalse = (!s1 && !s2);
        bool condition = (advanceIntersections[i] || bothTrue || bothFalse);

        if (condition){
            currentState->signal1[i] = s2;    // new signal1 = old signal2
            currentState->signal2[i] = !s1;   // new signal2 = NOT old signal1
        }
        else
        {
            currentState->signal1[i] = s1;    // remain
            currentState->signal2[i] = s2;    // remain
        }
    }

    double sampleProbability = 1.0;

    for (int c = 0; c < numCells; c++)
    {
        bool oldOcc = oldState.occupiedCells[c];
        bool newOcc = false; // we'll compute

        // (A) If PERIMETER-INPUT-CELL(c)
        if (isPerimeterInputCell[c]){
            if (!oldOcc){
                std::bernoulli_distribution dist(perimeterRate[c]);
                bool outcome = decision_outcomes == nullptr || perimeterRate[c] == 0  || perimeterRate[c] == 1? dist(rng) : getDecisionPoint(decision_point,0,1,decision_outcomes) == 0;
                newOcc = outcome;
                if (outcome){
                    sampleProbability *= perimeterRate[c];
                }
                else {
                    sampleProbability *= (1.0 - perimeterRate[c]);
                }
            }
            else
                newOcc = !existsUnoccupiedOutflow(oldState, c);
        }

        // (B) Else if c can flow through a green intersection
        else if (canFlowThroughGreen(oldState, c)){
            if (!oldOcc)
                newOcc = existsOccupiedInflow(oldState,c);
            else
                newOcc = false;
        }
        // (C) else if occupant(c) is true and c flows into an intersection => occupant'(c)=true (stuck at red)
        else if (flowsIntoIntersectionAndOccupied(oldState, c))
            newOcc = true;

        // (D) else if c is in region that might receive from an intersection
        else
        {
            bool flowsFromIntersection = false;
            for(int c2 : cellInflows[c]){
                if (!flowsCellToNS[c2].empty() || !flowsCellToEW[c2].empty()){
                    flowsFromIntersection = true;
                    break;
                }
            }
            if (flowsFromIntersection){
                if (oldOcc)
                    newOcc = !existsUnoccupiedOutflow(oldState, c);
                else {
                    newOcc = false;
                    for (int c2 : cellInflows[c]){
                        if (canFlowThroughGreen(oldState,c2)){
                            newOcc = true;
                            break;
                        }
                    }
                }

            }
            else{
                // (E) normal cell logic from the domain
                if (oldOcc)
                    newOcc = ! (isPerimeterExitCell[c] || existsUnoccupiedOutflow(oldState, c));
                else
                    newOcc = existsOccupiedInflow(oldState,c);
            }
        }

        currentState->occupiedCells[c] = newOcc;
    }

    // 4) Compute reward
    double rewardVal = 0.0;
    for (int c = 0; c < numCells; c++){
        if (oldState.occupiedCells[c]){
            if (existsOccupiedInflow(oldState,c))
                rewardVal -= 1.0;
        }
    }

    std::vector<double> rewards(1, rewardVal);
    return { rewards, sampleProbability };
}

} // end namespace Traffic
