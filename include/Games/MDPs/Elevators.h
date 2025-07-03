#pragma once

#ifndef ELE_H
#define ELE_H
#include "../Gamestate.h"
#endif


namespace ELE {

    constexpr  static double PENALTY_WAITING = 1;
    constexpr static double ELEVATOR_PENALTY_RIGHT_DIR = 0.75;
    constexpr static double ELEVATOR_PENALTY_WRONG_DIR = 3.0;

    class Gamestate : public ABS::Gamestate {
    public:
        // State variables
        std::vector<bool> person_waiting_up;
        std::vector<bool> person_waiting_down;
        std::vector<bool> person_in_elevator_going_up;
        std::vector<bool> person_in_elevator_going_down;
        std::vector<bool> elevator_dir_up;
        std::vector<bool> elevator_closed;
        std::vector<int> elevator_floor;

        int state_hash;

        [[nodiscard]] std::string toString() const override;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model : public ABS::Model {
    protected:
        int num_elevators{};
        int num_floors{};
        std::vector<double> ARRIVE_PARAM;
        std::vector<int> action_list;

        std::pair<std::vector<double>, double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName);

        void printState(ABS::Gamestate* uncasted_state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        bool hasTransitionProbs() override {return true;}
        int getNumPlayers() override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;

        [[nodiscard]] double getMinV(int steps) const override {
            return -(num_elevators * 2 * std::max(ELEVATOR_PENALTY_WRONG_DIR,ELEVATOR_PENALTY_RIGHT_DIR) +
                    num_floors * 2 * PENALTY_WAITING)*
                steps;
        }
        [[nodiscard]] double getMaxV(int steps) const override {return 0;}
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;
    };

} // namespace ABS