#pragma once

#ifndef GAMESTATECONTROLLER_H
#define GAMESTATECONTROLLER_H
#include <cassert>
#include <chrono>
#include <vector>
#include <iostream>
#include <random>
#include <unordered_map>
#include <map>

namespace ABS
{

    struct Gamestate{
        int turn=0; //player whose turn it is
        bool terminal=false;
        virtual ~Gamestate() = default;

        [[nodiscard]] virtual std::string toString() const { //optional
            return std::string("(") + std::to_string(turn) + std::string(",") + std::to_string(terminal) + std::string(")");
        }

        friend std::ostream& operator<<(std::ostream& os, const Gamestate& state) { //optional
            // Dynamically execute the function that converts the state to a string
            os << state.toString();
            return os;
        }

        // Functions for hashing and comparison needed for unordered_map and unordered_set
        virtual bool operator==(const Gamestate& other) const // required
        {
            throw std::runtime_error("Equality not implemented");
        }
        [[nodiscard]] virtual size_t hash() const //required
        {
            throw std::runtime_error("Hash not implemented");
        }

    };

    struct OutcomeHash { size_t operator()(const Gamestate* p) const {return p==nullptr? -1 : p->hash();} };
    struct OutcomeCompare {bool operator()(const Gamestate* lhs, const Gamestate* rhs) const {return (lhs == nullptr && rhs == nullptr) || (lhs != nullptr && rhs != nullptr && *lhs == *rhs);}};
    using outcomeMap = std::unordered_map<Gamestate*,std::pair<double,std::vector<double>>, OutcomeHash, OutcomeCompare>;

    class Model {

        protected:
            virtual std::pair<std::vector<double>,double> applyAction_(Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes)=0; //required
            virtual std::vector<int> getActions_(Gamestate* uncasted_state)=0; //required
            long total_forward_calls = 0;

            std::map<int,std::vector<int>> action_encoding_map;

        public:
            virtual ~Model() = default;
            virtual void printState(Gamestate* uncasted_state)=0; //required

            virtual Gamestate* getInitialState(std::mt19937& rng)=0; //Random init state, required
            virtual Gamestate* getInitialState(int num){ throw std::runtime_error("Deterministic initial state not implemented.");} //Deterministic init state
            virtual int getNumPlayers()=0; //Required
            virtual bool hasTransitionProbs()=0; //Required

            virtual Gamestate* copyState(Gamestate* uncasted_state)=0; //Required

            virtual std::vector<int> getActions(Gamestate* uncasted_state) final {
                assert (!uncasted_state->terminal); //state must not be terminal
                return getActions_(uncasted_state);
            };

            //Assertions:
            //1. Reward depends only on the state and action and NOT the sampled successor, i.e. R(s,a)
            virtual std::pair<std::vector<double>,double> applyAction(Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes = nullptr) final { //Required
                assert (!uncasted_state->terminal); //state must not be terminal
                total_forward_calls++;
                auto result = applyAction_(uncasted_state, action, rng, decision_outcomes);
                return result;
            }; //return value is reward +  probability of sample.


            static int getDecisionPoint(size_t& point, int min_outcome, int max_outcome, std::vector<std::pair<int,int>>* decision_outcomes) {
                if (decision_outcomes->size() <= point)
                    decision_outcomes->emplace_back(min_outcome, max_outcome);
                return decision_outcomes->at(point++).first;
            }

            std::pair<outcomeMap,double> getOutcomes(Gamestate* state, int action, int timeout_in_ms) {
                assert (getNumPlayers() == 1); //Only single player games are supported

                auto start = std::chrono::high_resolution_clock::now();
                outcomeMap outcomes = {};
                auto decision_outcomes = std::vector<std::pair<int,int>>();
                double psum = 0;
                bool timed_out = false;
                while (!timed_out) {
                    auto* successor = copyState(state);
                    std::mt19937 rng;
                    auto [reward, prob] = applyAction(successor, action, rng, &decision_outcomes);
                    if(!outcomes.contains(successor)) {
                        psum += prob;
                        outcomes.insert({successor,{prob,reward}});
                    }
                    else
                        delete successor;

                    while (!decision_outcomes.empty() && ++(decision_outcomes.at(decision_outcomes.size()-1).first) > decision_outcomes.at(decision_outcomes.size()-1).second)
                        decision_outcomes.pop_back();

                    timed_out = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() > timeout_in_ms;
                    if (decision_outcomes.empty())
                        break;
                }
                if (!timed_out && hasTransitionProbs() && std::fabs(1-psum) > 1e-6) {
                    printState(state);
                    std::cout << action << std::endl;
                    throw std::runtime_error(std::string("Probabilities do not sum to 1 but rather to ") + std::to_string(psum));
                }
                return {outcomes,psum};
            }

            std::vector<int> idx_to_multi_discrete(int idx){
                    auto multi = std::vector<int>();
                    for (int dim = 0; dim < (int)actionShape().size(); dim++) {
                        multi.push_back(idx % actionShape()[dim]);
                        idx /= actionShape()[dim];
                    }
                    return multi;
                }

            int multi_discrete_to_idx(std::vector<int> multi_discrete) {
                    int idx = 0;
                    for (int i = 0; i < (int) multi_discrete.size(); i++) {
                        int prod = 1;
                        for (int j = 0; j < i; j++)
                            prod *= actionShape()[j];
                        idx += multi_discrete[i] * prod;
                    }
                    return idx;
                }

            virtual long getForwardCalls() {
                return total_forward_calls;
            };

            //For potential ML applications
            [[nodiscard]] virtual std::vector<int> obsShape() const { //optional [wär aber nett]
                throw std::runtime_error("Observation Shape not implemented."); //Assuming obs can always be represented as a multi-dim box
            }

            virtual void getObs(ABS::Gamestate* uncasted_state, int* obs) { //optional [wär aber nett]
                throw std::runtime_error("Observation not implemented.");
            }

            [[nodiscard]] virtual std::vector<int> actionShape() const { //optional [wär aber nett]
                throw std::runtime_error("Action Shape not implemented."); //Assuming theres always a multi-discrete action space
            }

            [[nodiscard]] virtual int encodeAction(int* decoded_action) { //optional [wär aber nett]. This is to bring the action in the correct format to be used for applyAction. Valid is set to false, if the action is not legal in the given state.
                throw std::runtime_error("Action encoding not implemented.");
            }

            [[nodiscard]] virtual std::vector<int> decodeAction(int action) {
                auto iterator = action_encoding_map.find(action);
                if (iterator != action_encoding_map.end()) {
                    return iterator->second;
                } else {
                    //iterate over entire actionspace
                    auto ashape = actionShape();
                    auto aitr = new int[ashape.size()];
                    int max_actions = 1;
                    for (int i = 0; i < (int)ashape.size(); i++) {
                        aitr[i] = 0;
                        max_actions *= ashape[i];
                    }

                    for (int i = 0; i < max_actions; i++) {
                        if (encodeAction(aitr) == action) {
                            std::vector<int> result(aitr, aitr + ashape.size());
                            action_encoding_map[action] = result;
                            delete[] aitr;
                            return result;
                        }
                        //advance iterator
                        for (int j = (int)ashape.size()-1; j >= 0; j--){
                            aitr[j]++;
                            if (aitr[j] < ashape[j])
                                break;
                            else
                                aitr[j] = 0;
                        }
                    }
                }
                throw std::runtime_error("Couldnt decode action " + std::to_string(action));
            }

            //Functions for algorithms that need a heuristic value of the state
            [[nodiscard]] virtual std::vector<double> heuristicsValue(Gamestate* uncasted_state) { //optional
                throw std::runtime_error("Heuristics not implemented.");
            }

            [[nodiscard]] virtual double getMaxV(int remaining_steps) const { //optional
                throw std::runtime_error("MaxV not implemented.");
            }

            [[nodiscard]] virtual double getMinV(int remaining_steps) const { //optional
                throw std::runtime_error("MinV not implemented.");
            }

            [[nodiscard]] virtual double getDistance(const Gamestate* a, const Gamestate* b) const { //optional
                throw std::runtime_error("Distance not implemented.");
            }

            //Deserialization
            [[nodiscard]] virtual ABS::Gamestate* deserialize(std::string& ostring) const { //optional
                throw std::runtime_error("Deserialization not implemented.");
            }


    };
}

#endif