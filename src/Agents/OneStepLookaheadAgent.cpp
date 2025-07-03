#include "../../include/Agents/OneStepLookahead.h"

using namespace OSLA;

int OneStepLookaheadAgent::getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) {

    //We extract the player we are choosing an action for. This is needed to determine which reward we are maximizing
    int player = gamestate->turn;

    //Keep track of the best action so far
    int best_action = -1;
    double best_value = -std::numeric_limits<double>::infinity();

    //Loop over all legal actions in this state
    for (int action :  model->getActions(gamestate)) {

        //We copy the state as applying any action to the state will modify it
        auto copy = model->copyState(gamestate);

        //Apply the action to the copied state
        auto [rewards, prob] = model->applyAction(copy, action, rng);

        //To avoid memory leaks, close copy
        delete copy;

        //Trick to efficiently break ties
        rewards[player] += 1e-6 * std::uniform_real_distribution<double>(0, 1.0)(rng);

        //Update the best action if the reward is better
        if (rewards[player] > best_value) {
            best_value = rewards[player];
            best_action = action;
        }
    }

    return best_action;
}
