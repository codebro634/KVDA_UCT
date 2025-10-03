#pragma once

#ifndef CTF_H
#define CTF_H
#include <map>
#include <vector>

#include "WinLossGames.h"
#include "../Gamestate.h"
#endif

namespace CTF
{

    class Unit{
        public :
        int x;
        int y;
        int player;
        int health;
        int spawn_x;
        int spawn_y;
        bool has_flag;

        Unit(int x, int y, int player);

        bool operator==(const Unit& other) const {
            return (x == other.x) && (y == other.y) && (player == other.player) && (health == other.health) && (spawn_x == other.spawn_x) && (spawn_y == other.spawn_y) && (has_flag == other.has_flag);
        }
    };

    inline int MAP_WIDTH;
    inline int MAP_HEIGHT;
    inline std::vector<std::vector<int>> map = {};
    inline std::vector<std::vector<int>> init_unit_data = {};

    const int NUM_MOVES = 13;
    std::pair<int,int> hashToDir(int hash);
    int dirToHash(std::pair<int,int> dir);
    bool dirLegal(int x, int y, int dir, std::map<std::pair<int,int>,Unit*>& pos_to_unit);
    const int EMPTY = 0;
    const int WALL = 1;
    const int P0_BASE = 3;
    const int P1_BASE = 4;

    const int UATTACK = 1;
    const int UHEALTH = 2;

    struct Gamestate: public ABS::Gamestate{
        int next_unit_idx=0;
        std::vector<Unit> p0_units;
        std::vector<Unit> p1_units;
        std::pair<int,int> p0_flag_pos;
        std::pair<int,int> p1_flag_pos;

        //redundant information only for efficiency reasons
        std::map<std::pair<int,int>,Unit*> pos_to_unit;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model, public WLG::Model
    {
    public:
        explicit Model(bool zero_sum, const std::string &map);
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

    private:
        std::string MAP_PATH;

        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

