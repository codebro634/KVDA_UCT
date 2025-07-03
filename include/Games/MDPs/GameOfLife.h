#pragma once

#ifndef GOL_H
#define GOL_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace GOL
{

    struct Gamestate: public ABS::Gamestate{
        int board; // bits represent the alive/dead array

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override;
    };

    enum class ActionMode{
        ALL,
        SAVE_ONLY,
        REVIVE_ONLY
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName, ActionMode action_space);
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

    private:
        ActionMode action_mode;
        std::vector<std::vector<bool>> loaded_map;
        int loaded_map_hash;
        int map_width, map_height;
        std::vector<std::vector<float>> noise_map;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

        [[nodiscard]] double getMinV(int steps) const override {return 0;}
        [[nodiscard]] double getMaxV(int steps) const override {return (map_width*map_width)*steps;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;
    };

}

