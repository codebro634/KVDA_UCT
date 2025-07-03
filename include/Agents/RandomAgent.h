#pragma once

#ifndef RANDOMAGENT_H
#define RANDOMAGENT_H
#include "Agent.h"
#endif

class RandomAgent: public Agent {

    public:
        int getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) override;

};


