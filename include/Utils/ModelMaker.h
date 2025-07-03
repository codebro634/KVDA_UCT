#ifndef MODELMAKER_H
#define MODELMAKER_H

#include "../Games/Gamestate.h"

ABS::Model* getModel(std::string model_type, const std::vector<std::string>& m_args, int horizon=-1);

#endif //MODELMAKER_H
