#pragma once

#ifndef AGENT_H
#define AGENT_H
#include <random>

#include "../Games/Gamestate.h"
#endif


class Agent {

    public:
        Agent()= default;
        virtual int getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng)=0;
        virtual ~Agent() = default;
};
