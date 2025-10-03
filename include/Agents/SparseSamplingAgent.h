#pragma once

#ifndef SS_H
#define SS_H
#include <unordered_set>
#include "Agent.h"
#endif

namespace SS {

    struct GSHash {
        size_t operator()(const ABS::Gamestate* p) const {
            return p==nullptr? -1 : p->hash();
        }
    };

    struct GSCompare {
        bool operator()(const ABS::Gamestate* lhs, const ABS::Gamestate* rhs) const {
            return (lhs == nullptr && rhs == nullptr) || (lhs != nullptr && rhs != nullptr && *lhs == *rhs);
        }
    };

    using gsSet = std::unordered_set<ABS::Gamestate*, GSHash, GSCompare>;

    struct SSArgs
    {
        int depth{};
        int samples{};
        bool perfect_sampling = false;
        double discount = 1.0;
    };

    class SparseSamplingAgent final : public Agent{

    public:
        explicit SparseSamplingAgent(const SSArgs& args) : depth(args.depth), samples(args.samples),  perfect_sampling(args.perfect_sampling), discount(args.discount) {}
        int getAction(ABS::Model* model, ABS::Gamestate* state, std::mt19937& rng) override;

    private:

        std::pair<int,double> stateActionValue(ABS::Model* model, ABS::Gamestate* state, int current_depth, std::mt19937& rng);

        int depth,samples;
        bool perfect_sampling;
        double discount;
        constexpr static double TIEBREAKER_NOISE = 1e-6;

    };

}
