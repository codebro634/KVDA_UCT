#ifndef HUMANAGENT_H
#define HUMANAGENT_H
#include "Agent.h"
#endif

class HumanAgent: public Agent {

    public:
        int getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) override;

};
