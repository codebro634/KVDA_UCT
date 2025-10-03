#pragma once

#ifndef KTK_H
#define KTK_H
#include <map>
#include <vector>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

namespace KTK
{

    class Unit{
        public :
            int x;
            int y;
            int player;
            int health;
            int ability_id; //0 = attack, 1 = heal
            int ability_range;
            int ability_amount;
            bool king;
            Unit(int x, int y, int health, int player, int ability_id, int ability_range, int ability_amount, bool king);

        bool operator==(const Unit& other) const {
            return (x == other.x) && (y == other.y) && (player == other.player) && (health == other.health) && (ability_id == other.ability_id) && (ability_range == other.ability_range) && (ability_amount == other.ability_amount) && (king == other.king);
        }
    };

    inline int MAP_WIDTH;
    inline int MAP_HEIGHT;
    inline std::vector<std::vector<bool>> map = {};
    inline std::vector<std::vector<std::vector<int>>> init_unit_data = {};

    const int NUM_MOVES = 5;
    std::pair<int,int> hashToDir(int hash);
    int dirToHash(std::pair<int,int> dir);

    struct Gamestate: public ABS::Gamestate{
        int next_unit_idx=0;
        std::vector<Unit> p0_units;
        std::vector<Unit> p1_units;

        //redundant only for efficiency reasons
        std::map<std::pair<int,int>,Unit*> pos_to_unit;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model, public WLG::Model
    {
    public:
        explicit Model(bool zero_sum, const std::string& map_path);
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* getInitialState(int num) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

    private:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

