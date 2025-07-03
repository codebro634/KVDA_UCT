#pragma once

#ifndef PUS_H
#define PUS_H
#include <map>
#include <vector>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

namespace PUS
{
    class Unit{
        public :
            int x;
            int y;
            int player;
            bool active;
        Unit(int x, int y, int player, bool active);

        bool operator==(const Unit& other) const {
            return (x == other.x) && (y == other.y) && (player == other.player) && (active == other.active);
        }
    };

    inline int MAP_WIDTH;
    inline int MAP_HEIGHT;
    inline std::vector<std::vector<bool>> map = {};
    inline std::vector<std::vector<int>> init_unit_data = {};

    const int NUM_MOVES = 5;
    std::pair<int,int> hashToDir(int hash);
    int dirToHash(std::pair<int,int> dir);

    struct Gamestate: public ABS::Gamestate{
        int next_unit_idx=0;
        std::vector<Unit> p0_units; //contains all units even inactive/killed units
        std::vector<Unit> p1_units;

        //Redundant data for efficiency
        int p1_active_units;
        int p0_active_units;
        std::map<std::pair<int,int>,Unit*> pos_to_unit;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model, public WLG::Model
    {
        public:
            explicit Model(bool zero_sum, std::string map);
            ~Model() override = default;
            void printState(ABS::Gamestate* state) override;
            ABS::Gamestate* getInitialState(std::mt19937& rng) override;
            ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
            int getNumPlayers() override;
            bool hasTransitionProbs() override {return true;}
            std::vector<double> heuristicsValue(ABS::Gamestate* state) override;

        private:
        std::string MAP_PATH;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

