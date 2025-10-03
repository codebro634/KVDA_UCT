#pragma once

#ifndef MCTSNODE_H
#define MCTSNODE_H
#include <map>
#include <random>
#include <unordered_set>
#include <vector>
#include "../../Arena.h"

namespace Mcts
{
    struct GSHash {
        size_t operator()(const ABS::Gamestate* p) const {
            return p==nullptr? -1 : p->hash();
        }
    };

    struct GSCompare {
        bool operator()(const ABS::Gamestate* lhs, const ABS::Gamestate* rhs) const {
            return (lhs == nullptr && rhs == nullptr) || (lhs != nullptr && rhs != nullptr && *lhs == *rhs);
        }
    };

    class MctsNode;

    template<class T>
    using gsToNodeMap = std::unordered_map<ABS::Gamestate*, T, GSHash, GSCompare>;

    inline long global_id = 0;

    class MctsNode
    {

    public:
        MctsNode(
            ABS::Model* model,
            ABS::Gamestate* state,
            int depth,
            std::mt19937& rng
        );

        int popUntriedAction(double vinit);

        void addVisit();
        void addActionVisit(int action);
        void addActionValues(int action, const std::vector<double>& values, bool max_backup);

        [[nodiscard]] std::vector<double>* getActionValues(int action);

        [[nodiscard]] ABS::Model* getModel() const;
        [[nodiscard]] ABS::Gamestate* getStateCopy() const;
        [[nodiscard]] const ABS::Gamestate* getState() const;
        [[nodiscard]] std::map<int, gsToNodeMap<MctsNode*>>* getChildren();
        [[nodiscard]] int getPlayer() const;


        [[nodiscard]] int getDepth() const;
        [[nodiscard]] int getVisits() const;
        [[nodiscard]] int getActionVisits(int action);
        [[nodiscard]] bool isFullyExpanded() const;
        [[nodiscard]] bool isTerminal() const;
        [[nodiscard]] std::vector<int>* getTriedActions();

        ~MctsNode() = default;

        long id = Mcts::global_id++;

    private:
        // Model related stats
        ABS::Model* model;
        ABS::Gamestate* state;
        std::map<int, gsToNodeMap<MctsNode*>> children;

        // MCTS stats
        int depth;
        int visits;
        std::map<int, int> action_visits;
        std::map<int, std::vector<double>> action_values;
        std::vector<int> tried_actions;
        std::vector<int> untried_actions;
    };

}

#endif