
#ifndef COOPERATIVERECON_H
#define COOPERATIVERECON_H

#pragma once

#include <vector>
#include <string>
#include <random>
#include <stdexcept>
#include <cstddef>
#include "../Gamestate.h"

namespace RECON
{
    using namespace ABS;

    static constexpr int NUM_TOOLS = 3; // Camera, Life detector, Water detector

    static constexpr double detectProb_ = 0.8;
    static constexpr double detectProbDamaged_ = 0.4;

    struct ReconState : public Gamestate{
        int agent_x = 0;
        int agent_y = 0;

        std::vector<bool> waterChecked;
        std::vector<bool> waterDetected;
        std::vector<bool> lifeChecked;
        std::vector<bool> lifeChecked2;
        std::vector<bool> lifeDetected;
        std::vector<bool> pictureTaken;
        bool damaged[NUM_TOOLS] = {false, false, false};

        bool operator==(const Gamestate& other) const override;
        size_t hash() const override;
    };


    class ReconModel : public Model
    {
    private:

        int gridW_ = 0;
        int gridH_ = 0;
        std::vector<bool> hazard_;
        std::vector<bool> base_;
        std::vector<std::pair<int,int>> objectPositions_;
        std::vector<int> objects;

        double damageProb_[NUM_TOOLS];
        double goodPicWeight_;
        double badPicWeight_;
        int init_x, init_y;

    public:

        explicit ReconModel(const std::string& filename);
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}
        Gamestate* getInitialState(std::mt19937& rng) override;
        Gamestate* copyState(Gamestate* state) override;
        void printState(Gamestate* state) override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    protected:
        std::pair<std::vector<double>,double> applyAction_(Gamestate* state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(Gamestate* state) override;

    private:
        void actionToTools(int action, int x, int y, bool& usedCamera, bool& usedLife, bool& usedWater,int& objIndex) const;

    };
}



#endif //COOPERATIVERECON_H
