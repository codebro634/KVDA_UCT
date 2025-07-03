#ifndef CONSTR_H
#define CONSTR_H
#include <map>

#include "WinLossGames.h"
#include "../Gamestate.h"

namespace CON {

    const static int EMPTY = -1;
    const static int BODY = 0;

    struct Gamestate: public ABS::Gamestate{
        std::vector<std::vector<int>> arena;
        std::pair<int,int> head0, head1;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model: public ABS::Model, public WLG::Model
    {
    public:
        explicit Model(int arena_size, bool zero_sum);
        ~Model() override = default;
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs) override;
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

        std::vector<double> heuristicsValue(ABS::Gamestate* state) override;

    private:
        int arena_size;

        bool isValid(Gamestate* state, int x, int y) const;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
    };

}

#endif