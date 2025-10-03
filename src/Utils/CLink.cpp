#include <utility>

#include "../../include/Games/Gamestate.h"
#include "../../include/Utils/ModelMaker.h"
#include "../../include/Utils/AgentMaker.h"
#include "../../include/Agents/Oga/OgaAgent.h"
#include "../../include/Agents/Oga/OgaTree.h"

extern "C" {

    /*
     * For game interface
     */

    void get_obs(ABS::Model* model, ABS::Gamestate *state, int* ptr_obs) {
        model->getObs(state,ptr_obs);
    }

    int* get_obs_shape(ABS::Model* model, int* size, bool *implemented) {
       std::vector<int> obs_shape;
        try{
            obs_shape = model->obsShape();
        }catch (const std::exception& e){
            *implemented = false;
            return nullptr;
        }
        *implemented = true;
        *size = obs_shape.size();
        int* obs_shape_ptr = new int[*size];
        for (int i = 0; i < *size; i++)
            obs_shape_ptr[i] = obs_shape[i];
        return obs_shape_ptr;
    }

    void free_obs_shape(int* obs_shape_ptr) {
        delete[] obs_shape_ptr;
    }

    int* get_action_shape(ABS::Model* model, int* size) {
        auto action_shape = model->actionShape();
        *size = action_shape.size();
        int* action_shape_ptr = new int[*size];
        for (int i = 0; i < *size; i++)
            action_shape_ptr[i] = action_shape[i];
        return action_shape_ptr;
    }

    void free_action_shape(int* action_shape_ptr) {
        delete[] action_shape_ptr;
    }

    ABS::Gamestate** get_outcomes(ABS::Model* model, ABS::Gamestate* state, int action, double*** rewards, double** probs, int* size) {
        auto [outcomes,psum] = model->getOutcomes(state, action, (int)1000000000);
        if (std::fabs(1 - psum) > 1e-6)
            throw std::runtime_error("Timeout in getOutcomes");

        int idx = 0;
        auto** outcomes_ptr = new ABS::Gamestate*[outcomes.size()];
        *rewards = new double*[outcomes.size()];
        *probs = new double[outcomes.size()];
        *size = outcomes.size();

        for (auto [outcome, prob_reward] : outcomes) {
            outcomes_ptr[idx] = outcome;
            (*rewards)[idx] = new double[model->getNumPlayers()];
            for (int i = 0; i < model->getNumPlayers(); i++)
                (*rewards)[idx][i] = prob_reward.second[i];
            (*probs)[idx] = prob_reward.first;
            idx++;
        }

        return outcomes_ptr;
    }

    void free_outcomes(double** rewards, double* probs, int size){
        for (int i = 0; i < size; i++)
            delete[] rewards[i];
        delete rewards;
        delete probs;
    }

  int* get_available_actions(ABS::Model* model, ABS::Gamestate* state, int* num_actions, int* action_dim, bool* format_actions){
    auto actions = model->getActions(state);
    *num_actions = actions.size();
    *action_dim = format_actions? model->actionShape().size() : 1;
    int dims = *action_dim;
    int* aptr = new int[(*num_actions) * (*action_dim)];
    for (int i = 0; i < *num_actions; i++) {
        auto decoded = format_actions? model->decodeAction(actions[i]) : std::vector<int>(1, actions[i]);
        for (int j = 0; j < dims; j++)
            aptr[i * (*action_dim) + j] = decoded[j];
    }
    return aptr;
  }

  void free_action_ptr(int* aptr){
    delete[] aptr;
  }

  ABS::Model* create_model(char** args, int num_args, int max_steps) {
      std::vector<std::string> args_as_string;
      std::string mname = args[0];
      for (int i = 1; i < num_args; i++) {
          if (args[i] != nullptr) {
              args_as_string.emplace_back(args[i]);
          }
      }
      return getModel(mname, args_as_string, max_steps);
  }

    std::mt19937* create_rng(int seed) {
        return new std::mt19937(seed);
    }

  void close_model(ABS::Model* model) {
      delete model;
  }

  ABS::Gamestate* get_initial_state(ABS::Model* model, std::mt19937* rng) {
      return model->getInitialState(*rng);
  }

  void print_state(ABS::Model* model, ABS::Gamestate* state) {
      model->printState(state);
  }

  ABS::Gamestate* copy_state(ABS::Model* model, ABS::Gamestate* state) {
      return model->copyState(state);
  }

    bool is_action_legal(ABS::Model* model, ABS::Gamestate* state, int* action){
        int enc_action = model->encodeAction(action);
        for (int a : model->getActions(state)){
            if (a == enc_action)
                return true;
        }
        return false;
    }

  void apply_action(ABS::Model* model, ABS::Gamestate* state, int * action, double * rewards, double * prob, std::mt19937* rng, bool encode_before_apply){
        int enc_action = encode_before_apply? model->encodeAction(action) : action[0];
        auto result = model->applyAction(state, enc_action, *rng);
        for (int i = 0; i < (int)result.first.size(); i++)
            rewards[i] = result.first[i];
        *prob = result.second;
  }

    int num_players(ABS::Model* model) {
        return model->getNumPlayers();
    }

    int player_at_turn(ABS::Gamestate* state) {
        return state->turn;
    }

  bool is_terminal(ABS::Gamestate* state){
       return state->terminal;
  }

  void close_state(ABS::Gamestate* state){
      delete state;
  }

  void close_rng(std::mt19937* rng){
        delete rng;
  }

  bool equality(ABS::Gamestate* state1, ABS::Gamestate* state2){
      return *state1 == *state2;
  }

  int hash(ABS::Gamestate* state){
      return state->hash();
  }

    /*
     * For alpha zero interface
     */

    void idx_to_multi_discrete(ABS::Model* model, int idx, int* multi_discrete) {
        auto multi = model->idx_to_multi_discrete(idx);
        for (int i = 0; i < (int)multi.size(); i++)
            multi_discrete[i] = multi[i];
    }

    Agent* get_oga(char** args, int num_args) {
        //setup oga agent
        std::vector<std::string> args_as_string;
        std::string mname = args[0];
        for (int i = 0; i < num_args; i++) {
            if (args[i] != nullptr) {
                args_as_string.emplace_back(args[i]);
            }
        }
        auto oga = dynamic_cast<OGA::OgaAgent*>(getAgent("oga", args_as_string));
        return oga;
    }

    void free_oga(OGA::OgaAgent* oga) {
        delete oga;
    }

    void get_mcts_stats(ABS::Model* model, ABS::Gamestate* state, OGA::OgaAgent* oga, double* distribution, int* greedy_action) {

        //get action distribution
        auto** treeptr = new OGA::OgaTree*[1];
        std::random_device rd;
        std::mt19937 rng(rd());
        int chosen_action = oga->getAction(model, state, rng, treeptr);
        int vsum = 0;
        for (auto child : treeptr[0]->getRoot()->getChildren())
            vsum += child->getVisits();

        int max_actions = 1;
        for (auto dim : model->actionShape())
            max_actions *= dim;
        for (int i = 0; i < max_actions; i++)
            distribution[i] = 0.0;

        for (auto child : treeptr[0]->getRoot()->getChildren()) {
            int action = child->getAction();
            auto decoded = model->decodeAction(action);
            int decoded_idx =  model->multi_discrete_to_idx(decoded);
            distribution[decoded_idx] = (double)child->getVisits() / (double)vsum;
            if (action == chosen_action)
                *greedy_action = decoded_idx;
        }

        //memory cleanup
        delete treeptr[0];
        delete[] treeptr;
    }

}
