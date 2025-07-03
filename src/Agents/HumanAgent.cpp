#include "../../include/Agents/HumanAgent.h"
#include "../../include/Games/Gamestate.h"

int HumanAgent::getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) {
    model->printState(gamestate);
    auto available_actions = model->getActions(gamestate);
    std::cout << "Available actions: ";
    for (int available_action : available_actions)
        std::cout << available_action <<" ";
    std::cout << std::endl;
    std::cout << "(Player " << gamestate->turn << ") Enter action: ";
    int action;
    std::cin >> action;
    std::cout << std::endl;
    return action;
}
