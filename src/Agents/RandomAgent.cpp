#include "../../include/Agents/RandomAgent.h"
#include <cstdlib>

int RandomAgent::getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) {
    auto available_actions = model->getActions(gamestate);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(available_actions.size())-1);
    return available_actions[dist(rng)];
}
