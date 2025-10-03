#pragma once

#ifndef J5_H
#define J5_H
#include <bitset>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "../Gamestate.h"
#endif

namespace J5
{

    constexpr int SIZE = 34;

    inline std::hash<std::bitset<SIZE*SIZE>> hash_horizontals;
    inline std::hash<std::bitset<SIZE*SIZE>> hash_verticals;
    inline std::hash<std::bitset<SIZE*SIZE>> hash_diagonals_left;
    inline std::hash<std::bitset<SIZE*SIZE>> hash_diagonals_right;
    inline std::hash<std::bitset<SIZE*SIZE>> hash_crosses;

    struct Gamestate: public ABS::Gamestate{
        std::bitset<SIZE*SIZE> crosses;

        std::bitset<SIZE * SIZE> horizontals;
        std::bitset< SIZE * SIZE> verticals;
        std::bitset<SIZE * SIZE> diagonals_left;
        std::bitset<SIZE * SIZE> diagonals_right;

        int num_horizontals;
        int num_verticals;
        int num_diagonals_left;
        int num_diagonals_right;

        std::set<int> avail_actions;
        std::unordered_map<int,std::vector<int>> verticals_to_actions; //maps to which of the available actions would cross over the vertical
        std::unordered_map<int,std::vector<int>> horizontals_to_actions;
        std::unordered_map<int,std::vector<int>> diagonals_left_to_actions;
        std::unordered_map<int,std::vector<int>> diagonals_right_to_actions;
        std::unordered_map<int,std::vector<int>> holes_to_actions;

        int first_stone_idx = -1; //only needed if decoupled action space is set to true

        //TEMP FOR DEBUG ONLY
        //std::vector<std::pair<std::pair<int,int>,std::pair<int,int>>> drawn_lines;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(bool joint, bool exit_on_out_of_bounds, bool decoupled_action_space);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* getInitialState(int num) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

        [[nodiscard]] std::vector<double> heuristicsValue(ABS::Gamestate* uncasted_state);

    private:
        bool joint;
        bool exit_on_out_of_bounds;
        bool decoupled_action_space;
        std::vector<std::vector<std::tuple<int,int,int>>> actions_lookup_table;
        std::map<std::pair<int,int>,std::tuple<int,int,int>> action_to_dir_code_and_offset;

        int line_segment_to_index(int row_from, int col_from, int row_to, int col_to); //helper
        bool out_of_bounds(int row, int col);
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    };

}

