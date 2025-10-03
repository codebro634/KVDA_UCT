// #include <iostream>
// #include <ostream>
// #include "../include/Games/MDPs/GameOfLife.h"
// #include "../include/Games/MDPs/TriangleTireworld.h"
// #include "../include/Agents/RandomAgent.h"
// #include "../include/Agents/OneStepLookahead.h"
// #include "../include/Games/Wrapper/FiniteHorizon.h"
// #include "../include/Arena.h"
//
// // int main(const int argc, char **argv) {
// //
// //     /**
// //     * In this demo, we run a random agent for a single episode on a Game of Life instance and print the total reward obtained.
// //      */
// //
//      //We require a seeded number generator for reproducibility
//      std::mt19937 rng(static_cast<unsigned int>(42));
//
//      //We create a Game of Life Model and wrap it in a finite horizon model thus ensuring that an episode ends after 50 steps
//      auto base_model = new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::ALL);
//      auto model = new FINITEH::Model(base_model, 50, true);
//
//      //We will be using a random agent to play Game of Life
//      auto agent = new RandomAgent();
//
//      //Sample an initial state. For any Game of Life instance, there is only a single initial state
//      auto state = model->getInitialState(rng);
//      double reward_sum = 0;
//      while (!state->terminal) {
//
//          //Query agent for an action in the current state
//          int action = agent->getAction(model, state, rng);
//
//          //Apply action modifies 'state' and returns the reward and the probability of the sampled transition
//          auto [rewards,prob] = model->applyAction(state, action, rng);
//
//          //Since the Gamestate.h interface supports multiplayer games, rewards is a vector. Since Game of Life is a single player game, rewards has size 1
//          reward_sum += rewards[0];
//      }
//      std::cout << "Total reward: " << reward_sum << std::endl;
//
//      //Cleanup pointers
//      delete state;
//      delete model;
//      delete agent;
// // }
//
//
//
//
//
// int main(const int argc, char **argv) {
//
//     //We require a seeded number generator for reproducibility
//     std::mt19937 rng(static_cast<unsigned int>(42));
//
//     //We create a Triangle Tireworld instance
//     auto base_model = new TRT::Model("../resources/TriangleTireworlds/5_IPPC.txt",true,true,true);
//
//     //We will be using a One-Step-Lookahead agent to play Triangle Tireworld
//     auto agent = new OSLA::OneStepLookaheadAgent();
//
//     /*
//      * We evaluate the One-Step-Lookahead agent on 100 episodes with 50 steps each.
//      * The agent is given a planning horizon of 20 steps, however, in this case the agent only requires only a planning horizon of 1 step.
//      * Note that we did not the model in a finite horizon model. This is automatically done by playGames.
//      */
//     playGames(*base_model, 100, {agent}, rng, VERBOSE, {50,20});
//
//     //Cleanup poiners
//     delete base_model;
//     delete agent;
// }
