#pragma once

#ifndef BIN_PACKING_H
#define BIN_PACKING_H
#include <vector>

#include "../Gamestate.h"
#endif

namespace BIN_PACKING
{

    struct Gamestate: public ABS::Gamestate{
        std::vector<int> total_size_in_bin; // the total size of objects for each bin
        int current_item_size;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override;
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


        [[nodiscard]] double getMinV(int steps) const override {return 0;}
        [[nodiscard]] double getMaxV(int steps) const override {return steps * nr_of_bins;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

    private:
        int nr_of_bins;
        std::vector<int> bin_capacity;
        std::vector<int> item_sizes;
        std::vector<double> item_probabilities;
        int initial_item_size;
        size_t random_item(std::mt19937& rng) const;

    protected:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    };

}

