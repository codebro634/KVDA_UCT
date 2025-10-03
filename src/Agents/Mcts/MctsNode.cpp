#include <utility>
#include <random>
#include <algorithm>
#include <ranges>
#include "../../../include/Agents/Mcts/MctsNode.h"

using namespace Mcts;

MctsNode::MctsNode(
    ABS::Model* model,
    ABS::Gamestate* state,
    int depth,
    std::mt19937& rng
): model(model), state(state), depth(depth){
    visits = 0;
    untried_actions = state->terminal? std::vector<int>() : model->getActions(state);
    std::ranges::shuffle(untried_actions.begin(), untried_actions.end(), rng);
}

int MctsNode::popUntriedAction(double vinit){
    int a = untried_actions.back();
    untried_actions.pop_back();
    tried_actions.push_back(a);
    action_values[a] = std::vector<double>(model->getNumPlayers(), vinit);
    action_visits[a] = 0;
    children[a] = {};
    return a;
}

void MctsNode::addVisit()
{
    visits++;
}

void MctsNode::addActionVisit(const int action)
{
    action_visits[action]++;
}

void MctsNode::addActionValues(const int action, const std::vector<double>& values, bool max_backup)
{
    for (size_t i = 0; i < values.size(); i++) {
        if (max_backup)
            action_values[action][i] = std::max(action_values[action][i], values[i]);
        else
            action_values[action][i] += values[i];
    }
}

bool MctsNode::isFullyExpanded() const
{
    return untried_actions.empty();
}

int MctsNode::getDepth() const
{
    return depth;
}

ABS::Model* MctsNode::getModel() const
{
    return model;
}

std::vector<double>* MctsNode::getActionValues(const int action)
{
    return &action_values[action];
}

int MctsNode::getActionVisits(const int action)
{
    return action_visits[action];
}

std::vector<int>* MctsNode::getTriedActions()
{
    return &tried_actions;
}

std::map<int, gsToNodeMap<MctsNode*>>* MctsNode::getChildren()
{
    return &children;
}

int MctsNode::getVisits() const
{
    return visits;
}

bool MctsNode::isTerminal() const
{
    return state->terminal;
}

ABS::Gamestate* MctsNode::getStateCopy() const
{
    return model->copyState(state);
}

int MctsNode::getPlayer() const
{
    return state->turn;
}

const ABS::Gamestate* MctsNode::getState() const {
    return state;
}