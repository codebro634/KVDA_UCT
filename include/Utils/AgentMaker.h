#ifndef AGENTMAKER_H
#define AGENTMAKER_H

#include <map>
#include <set>

#include "../Agents/Agent.h"

Agent* getDefaultAgent(bool strong);

Agent* getAgent(const std::string& agent_type, const std::vector<std::string>& a_args);

std::string extraArgs(std::map<std::string, std::string>& given_args, const std::set<std::string>& acceptable_args);

#endif //AGENTMAKER_H
