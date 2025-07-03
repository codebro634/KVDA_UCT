#include "../../../include/Games/MDPs/CooperativeRecon.h"
#include <fstream>
#include <iostream>
#include <cassert>
#include <random>

namespace RECON
{

    std::vector<int> ReconModel::obsShape() const {
        return {10+NUM_TOOLS,gridW_,gridH_};
    }

    void ReconModel::getObs(ABS::Gamestate* uncasted_state, int* obs) {
        auto *st = dynamic_cast<ReconState*>(uncasted_state);
        auto msize = gridW_ * gridH_;
        assert(st != nullptr);

        // 0: agent position
        for(int i=0; i<gridW_; i++){
            for(int j=0; j<gridH_; j++){
                obs[0*msize + j*gridW_ + i] = st->agent_x == i && st->agent_y == j ? 1 : 0;
                obs[1*msize + j*gridW_ + i] = hazard_[j*gridW_ + i] ? 1 : 0;
                obs[2*msize + j*gridW_ + i] = base_[j*gridW_ + i] ? 1 : 0;
                obs[3*msize + j*gridW_ + i] = 0;
                obs[4*msize + j*gridW_ + i] = 0;
                obs[5*msize + j*gridW_ + i] = 0;
                obs[6*msize + j*gridW_ + i] = 0;
                obs[7*msize + j*gridW_ + i] = 0;
                obs[8*msize + j*gridW_ + i] = 0;
                for(int t=0; t<NUM_TOOLS; t++){
                    obs[(9+t)*msize + j*gridW_ + i] = 0;
                }
            }
        }

        for (int i = 0; i < (int)objectPositions_.size(); i++){
            auto [ox, oy] = objectPositions_[i];
            obs[3*msize + oy*gridW_ + ox] = st->waterChecked[i] ? 1 : 0;
            obs[4*msize + oy*gridW_ + ox] = st->waterDetected[i] ? 1 : 0;
            obs[5*msize + oy*gridW_ + ox] = st->lifeChecked[i] ? 1 : 0;
            obs[6*msize + oy*gridW_ + ox] = st->lifeChecked2[i] ? 1 : 0;
            obs[7*msize + oy*gridW_ + ox] = st->lifeDetected[i] ? 1 : 0;
            obs[8*msize + oy*gridW_ + ox] = st->pictureTaken[i] ? 1 : 0;
        }

        for(int t=0; t<NUM_TOOLS; t++)
            obs[(9+t)*msize + st->agent_y*gridW_ + st->agent_x] = st->damaged[t] ? 1 : 0;

    }

    [[nodiscard]] std::vector<int> ReconModel::actionShape() const {
            return {5 + NUM_TOOLS*2};
    }

    int ReconModel::encodeAction(int* decoded_action) {
        return decoded_action[0]-1;
    }

    bool ReconState::operator==(const Gamestate& other) const
    {
        const auto* o = dynamic_cast<const ReconState*>(&other);
        if(!o) return false;

        // Compare our domain-specific fields
        if(this->agent_x != o->agent_x || this->agent_y != o->agent_y) return false;
        for(size_t i=0; i<waterChecked.size(); i++){
            if(waterChecked[i]  != o->waterChecked[i])  return false;
            if(waterDetected[i] != o->waterDetected[i]) return false;
            if(lifeChecked[i]   != o->lifeChecked[i])   return false;
            if(lifeChecked2[i]  != o->lifeChecked2[i])  return false;
            if(lifeDetected[i]  != o->lifeDetected[i])  return false;
            if(pictureTaken[i]  != o->pictureTaken[i])  return false;
        }
        for(int t=0; t<NUM_TOOLS; t++){
            if(damaged[t] != o->damaged[t]) return false;
        }
        return true;
    }

    size_t ReconState::hash() const
    {
        size_t hash = agent_x | (agent_y << 3) | damaged[0] << 6 | damaged[1] << 7 | damaged[2] << 8;

        size_t obj_hash = 0;
        for(size_t i=0; i<lifeChecked.size(); i++){
            obj_hash = obj_hash << 1 | waterChecked[i];
            obj_hash = obj_hash << 1 | waterDetected[i];
            obj_hash = obj_hash << 1 | lifeChecked[i];
            obj_hash = obj_hash << 1 | lifeChecked2[i];
            obj_hash = obj_hash << 1 | lifeDetected[i];
            obj_hash = obj_hash << 1 | pictureTaken[i];
        }

        return hash | (obj_hash << 9);
    }



    // File format:
    // ---------------------------------------------------
    //   gridW gridH
    //   init_x init_y
    //   nObjects
    //   (objX_0 objY_0)
    //   (objX_1 objY_1)
    //   ...
    //   hazardCount
    //   (hazardX_0 hazardY_0)
    //   ...
    //   baseCount
    //   (baseX_0 baseY_0)
    //   ...
    //   damageProbCam damageProbLife damageProbWater
    //   goodPicWeight
    //   badPicWeight
    // ---------------------------------------------------
    ReconModel::ReconModel(const std::string& filename)
    {
        std::ifstream in(filename);
        if(!in.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        // 1) Read grid size
        in >> gridW_ >> gridH_;
        if(!in.good()) {
            throw std::runtime_error("Error reading grid W/H from file.");
        }

        // We will store hazard/base layout in row-major form
        hazard_.resize(gridW_ * gridH_, false);
        base_.resize(gridW_   * gridH_, false);

        // Read initial agent position

        in >> init_x >> init_y;

        // Number of objects, then read object positions
        int nObjects;
        in >> nObjects;
        if(!in.good() || nObjects < 0) {
            throw std::runtime_error("Invalid nObjects in file.");
        }
        objectPositions_.clear();
        objectPositions_.reserve(nObjects);
        objects.resize(gridW_   * gridH_, -1);
        for(int i=0; i<nObjects; i++){
            int ox, oy;
            in >> ox >> oy;
            if(!in.good()) {
                throw std::runtime_error("Error reading object position from file.");
            }
            objectPositions_.emplace_back(ox, oy);
            objects[oy * gridW_ + ox] = objectPositions_.size()-1;
        }

        // Number of hazards, then read hazard positions
        int hazardCount;
        in >> hazardCount;
        for(int i=0; i<hazardCount; i++){
            int hx, hy;
            in >> hx >> hy;
            if(hx < 0 || hx >= gridW_ || hy < 0 || hy >= gridH_) {
                throw std::runtime_error("Invalid hazard cell in file.");
            }
            hazard_[hy * gridW_ + hx] = true;
        }

        // Number of base squares, then read them
        int baseCount;
        in >> baseCount;
        for(int i=0; i<baseCount; i++){
            int bx, by;
            in >> bx >> by;
            if(bx < 0 || bx >= gridW_ || by < 0 || by >= gridH_) {
                throw std::runtime_error("Invalid base cell in file.");
            }
            base_[by * gridW_ + bx] = true;
        }

        // Tool damage probabilities
        in >> damageProb_[0] >> damageProb_[1] >> damageProb_[2];
        if(!in.good()) {
            throw std::runtime_error("Error reading damageProb for the 3 tools.");
        }

        // Reward weights
        in >> goodPicWeight_ >> badPicWeight_;
        if(!in.good()) {
            throw std::runtime_error("Error reading goodPicWeight / badPicWeight.");
        }
    }

    int ReconModel::getNumPlayers(){
        return 1;
    }

    // Create a new initial ReconState, with agent at (0,0), no damage, etc.
    Gamestate* ReconModel::getInitialState(std::mt19937& /*rng*/)
    {
        auto* st = new ReconState();

        st->agent_x = init_x;
        st->agent_y = init_y;

        // All booleans are false initially
        st->waterChecked.resize(objectPositions_.size(), false);
        st->waterDetected.resize(objectPositions_.size(), false);
        st->lifeChecked.resize(objectPositions_.size(), false);
        st->lifeChecked2.resize(objectPositions_.size(), false);
        st->lifeDetected.resize(objectPositions_.size(), false);
        st->pictureTaken.resize(objectPositions_.size(), false);


        // Not damaged
        for(int t=0; t<NUM_TOOLS; t++)
            st->damaged[t] = false;
        return st;
    }

    // Make a deep copy of a ReconState
    Gamestate* ReconModel::copyState(Gamestate* state)
    {
        auto* src = dynamic_cast<ReconState*>(state);
        auto* cp  = new ReconState(*src); // default copy constructor
        return cp;
    }

    // Print the state in a human-readable format
    void ReconModel::printState(Gamestate* state)
    {
        auto* st = dynamic_cast<ReconState*>(state);
        std::cout << "ReconState:\n";
        std::cout << "  agent=("    << st->agent_x << "," << st->agent_y << ")\n";
        for(int t=0; t<NUM_TOOLS; t++){
            std::cout << "  Tool[" << t << "] damaged=" << st->damaged[t] << "\n";
        }
        for(size_t i=0; i<objectPositions_.size(); i++){
            std::cout << "  Object[" << i << "]: waterChecked="   << st->waterChecked[i]
                      << ", waterDetected="  << st->waterDetected[i]
                      << ", lifeChecked="    << st->lifeChecked[i]
                      << ", lifeChecked2="   << st->lifeChecked2[i]
                      << ", lifeDetected="   << st->lifeDetected[i]
                      << ", pictureTaken="   << st->pictureTaken[i]
                      << "\n";
        }
    }


    std::vector<int> ReconModel::getActions_(Gamestate* state)
    {
        auto* st = dynamic_cast<ReconState*>(state);
        std::vector<int> actions;

        // We encode actions as:
        //   0=UP,1=DOWN,2=LEFT,3=RIGHT
        //   4=REPAIR_CAMERA, 5=REPAIR_LIFE, 6=REPAIR_WATER
        //   7 + (toolIndex * MAX_OBJECTS) + objIndex = useToolOn
        //       where toolIndex in {0=camera,1=life,2=water}
        //       and objIndex in [0..nObjects-1]

        //idle action
        actions.push_back(-1);

        // Movement if agent not on boundary
        actions.push_back(0);  // up
        actions.push_back(1);  // down
        actions.push_back(2);  // left
        actions.push_back(3);  // right

        // If on a base cell, we can do repair
        bool onBase = base_[st->agent_y * gridW_ + st->agent_x];
        if(onBase) {
            actions.push_back(4); // repair camera
            actions.push_back(5); // repair life
            actions.push_back(6); // repair water
        }

        if (objects[st->agent_y * gridW_ + st->agent_x] != -1) {
            actions.push_back(7); //use camera
            actions.push_back(8); //use life
            actions.push_back(9); //use water
        }

        return actions;
    }

    // Helper to decode an action integer
    void ReconModel::actionToTools(int action, int x, int y,
                                           bool& usedCamera, bool& usedLife, bool& usedWater,
                                           int& objIndex) const
    {
        usedCamera = false;
        usedLife   = false;
        usedWater  = false;
        objIndex   = -1;

        // Actions < 7 are movement or repair
        if(action < 7) {
            return;
        }
        // Otherwise, it's "use tool on object"
        int toolIdx  = action - 7;
        switch(toolIdx) {
            case 0: usedCamera = true; break;
            case 1: usedLife   = true; break;
            case 2: usedWater  = true; break;
            default: /* unexpected */  break;
        }
        objIndex = objects[y * gridW_ + x];
    }

    std::pair<std::vector<double>, double>
    ReconModel::applyAction_(Gamestate* state, int action, std::mt19937& rng,std::vector<std::pair<int,int>>* decision_outcomes)
    {
        auto* st = dynamic_cast<ReconState*>(state);

        auto ost = *st;
        st->pictureTaken = std::vector<bool>(objectPositions_.size(), false);

        double reward = 0.0;
        size_t decision_point = 0;

        // If the action is "use camera on object i", we check the domain's formula
        bool usedCamera, usedLife, usedWater;
        int  usedObj = -1;
        actionToTools(action,st->agent_x,st->agent_y, usedCamera, usedLife, usedWater, usedObj);

        if(usedCamera && usedObj >= 0) {
            bool lifeDet   = ost.lifeDetected[usedObj];
            bool camDamaged = ost.damaged[0]; // camera tool is index 0

            // + goodPicWeight if ~pictureTaken & lifeDetected & camera not damaged
            if(!ost.pictureTaken[usedObj] && lifeDet && !camDamaged) {
                reward += goodPicWeight_;
            }
            // - badPicWeight if ~lifeDetected
            // The domain formula penalizes pictures when life is not detected,
            // regardless of camera damage. 
            if(!lifeDet) {
                reward -= badPicWeight_;
            }
        }

        switch(action) {
            case -1: //do-nothing
                break;
            case 0: // up
                if(ost.agent_y > 0) st->agent_y--;
                break;
            case 1: // down
                if(ost.agent_y < gridH_ - 1) st->agent_y++;
                break;
            case 2: // left
                if(ost.agent_x > 0) st->agent_x--;
                break;
            case 3: // right
                if(ost.agent_x < gridW_ - 1) st->agent_x++;
                break;
            case 4: // repair camera
            {
                // only valid if on a base
                bool onBase = base_[ost.agent_y * gridW_ + ost.agent_x];
                if(onBase) {
                    st->damaged[0] = false;
                }
            }
            break;
            case 5: // repair life
            {
                bool onBase = base_[ost.agent_y * gridW_ + ost.agent_x];
                if(onBase) {
                    st->damaged[1] = false;
                }
            }
            break;
            case 6: // repair water
            {
                bool onBase = base_[ost.agent_y * gridW_ + ost.agent_x];
                if(onBase) {
                    st->damaged[2] = false;
                }
            }
            break;
            default:
            {
                // Possibly using one of the tools on object
                if(usedCamera && usedObj >= 0) {
                    // pictureTaken' = true if camera not damaged
                    if(!ost.damaged[0]) {
                        st->pictureTaken[usedObj] = true;
                    }
                }
                else if(usedLife && usedObj >= 0) {
                    // first check sets lifeChecked, second sets lifeChecked2
                    if(!ost.lifeChecked[usedObj]) {
                        st->lifeChecked[usedObj] = true;
                    } else if(!ost.lifeChecked2[usedObj]) {
                        st->lifeChecked2[usedObj] = true;
                    }
                }
                else if(usedWater && usedObj >= 0) {
                    // waterChecked' = true on first use
                    if(!ost.waterChecked[usedObj]) {
                        st->waterChecked[usedObj] = true;
                    }
                }
            }
            break;
        }

        double outcomeProb = 1.0;
        // 3a) Check if we are in or adjacent to hazard => tool damage
        double damageFactor = 0.0;
        {
            int idx = ost.agent_y * gridW_ + ost.agent_x;
            bool inHazard = hazard_[idx];
            if(inHazard){
                damageFactor = 1.0; 
            } else {
                // Check adjacency in manhattan sense
                bool nearHazard = false;
                // Up/down/left/right
                if(ost.agent_y > 0) {
                    if(hazard_[(ost.agent_y-1)*gridW_ + ost.agent_x]) nearHazard = true;
                }
                if(ost.agent_y < gridH_-1) {
                    if(hazard_[(ost.agent_y+1)*gridW_ + ost.agent_x]) nearHazard = true;
                }
                if(ost.agent_x > 0) {
                    if(hazard_[ost.agent_y * gridW_ + (ost.agent_x-1)]) nearHazard = true;
                }
                if(ost.agent_x < gridW_-1) {
                    if(hazard_[ost.agent_y * gridW_ + (ost.agent_x+1)]) nearHazard = true;
                }
                if(nearHazard) damageFactor = 0.5;
            }
        }

        // For each tool, if not already damaged, sample damage
        for(int t=0; t<NUM_TOOLS; t++){
            if(!ost.damaged[t]) {
                double p = damageFactor * damageProb_[t];
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                double r = dist(rng);
                bool newDamaged = (r < p);
                if( (decision_outcomes == nullptr && newDamaged) || (decision_outcomes != nullptr && (p==1 || (p != 0 &&  0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))) {
                    st->damaged[t] = true;
                    outcomeProb *= p;
                }else
                    outcomeProb *= (1.0 - p);
            }
        }

        // 3b) If we used water tool, sample detection if not yet forced false
        if(usedWater && usedObj >= 0) {
            if(!ost.waterDetected[usedObj] && !ost.waterChecked[usedObj]) {
                // Probability depends on tool damage
                double p = ost.damaged[2] ? detectProbDamaged_ : detectProb_;
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                double r = dist(rng);
                bool detect = (r < p);
                if( (decision_outcomes == nullptr && detect) || (decision_outcomes != nullptr && (p==1 || (p != 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))) {
                    st->waterDetected[usedObj] = true;
                    outcomeProb *= p;
                }else
                    outcomeProb *= 1.0 - p;
            }
        }

        // 3c) If we used life tool, sample detection if possible
        //     domain says: only if waterDetected and not yet 2 checks
        if(usedLife && usedObj >= 0) {
            bool canDetect = (!ost.lifeDetected[usedObj]) &&
                             (!ost.lifeChecked2[usedObj]) &&
                              ost.waterDetected[usedObj];
            if(canDetect){
                double p = ost.damaged[1] ? detectProbDamaged_ : detectProb_;
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                double r = dist(rng);
                bool detect = (r < p);
                if( (decision_outcomes == nullptr && detect) || (decision_outcomes != nullptr && (p==1 || (p != 0 &&  0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))) {
                    st->lifeDetected[usedObj] = true;
                    outcomeProb *= p;
                }else
                    outcomeProb *= 1.0 - p;
            }
        }

        // Build the final return
        std::vector<double> rewardVec(1, reward);

        return {rewardVec, outcomeProb};
    }
} // end namespace ReconDomain