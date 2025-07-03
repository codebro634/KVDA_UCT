#ifndef ONESTEPLOOK_H
#define ONESTEPLOOK_H
#include "Agent.h"
#endif

namespace OSLA
{
    class OneStepLookaheadAgent: public Agent {

    public:
        int getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) override;

    };

}


