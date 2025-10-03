#include "../../include/Agents/SparseSamplingAgent.h"
#include <cstdlib>
#include <set>

using namespace SS;

int SparseSamplingAgent::getAction(ABS::Model* model, ABS::Gamestate* gamestate, std::mt19937& rng) {
    assert (model->getNumPlayers() == 1); // Only supports single player games
    return stateActionValue(model, gamestate, 0, rng).first;
}

std::pair<int,double> SparseSamplingAgent::stateActionValue(ABS::Model *model, ABS::Gamestate *state, int current_depth, std::mt19937 &rng) {
    assert (!perfect_sampling || model->hasTransitionProbs());

    if(current_depth == depth || state->terminal)
        return {-1,0};

    auto dist = std::uniform_real_distribution<double>(0,1);
    double best_value = std::numeric_limits<double>::lowest();
    double noisy_best_value = std::numeric_limits<double>::lowest();
    int best_action = -42;

    auto actions = model->getActions(state);
    for(auto action : actions){
        double action_val = 0;
        if(perfect_sampling) {
            double psum = 0;
            gsSet sampled_outcomes = {};
            while(psum < 1 - 1e-6) {
                auto copy = model->copyState(state);
                auto [rewards, prob] = model->applyAction(copy, action, rng, nullptr);
                if(sampled_outcomes.contains(copy)) {
                    delete copy;
                    continue;
                }
                sampled_outcomes.insert(copy);
                psum += prob;
                action_val +=  prob*(rewards[0] + discount * stateActionValue(model, copy,current_depth+1, rng).second);
            }
            for(auto s : sampled_outcomes)
                delete s;
        }else {
            double act_val_sum = 0;
            for(int i = 0; i < samples; i++) {
                auto copy = model->copyState(state);
                auto [rewards, outcome_and_probability] = model->applyAction(copy, action, rng, nullptr);
                act_val_sum +=  rewards[0] + discount * stateActionValue(model, copy,current_depth+1, rng).second;
                delete copy;
            }
            action_val = act_val_sum / samples;
        }

        double noisy_action_val = action_val + TIEBREAKER_NOISE * dist(rng);
        if(noisy_action_val > noisy_best_value){
            best_value = action_val;
            noisy_best_value = noisy_action_val;
            best_action = action;
        }
    }

    return {best_action, best_value};

}

