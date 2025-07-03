#include "../../include/Utils/MiscAnalysis.h"
#include "../../include/Arena.h"
#include "../../include/Utils/ValueIteration.h"
#include "../../include/Utils/AgentMaker.h"
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../../include/Agents/RandomAgent.h"
#include "../../include/Games/MDPs/SailingWind.h"
#include "../../include/Games/MDPs/SkillsTeaching.h"
#include "../../include/Games/MDPs/SysAdmin.h"
#include "../../include/Games/MDPs/EarthObservation.h"
#include "../../include/Games/MDPs/Manufacturer.h"
#include "../../include/Games/MDPs/GameOfLife.h"
#include "../../include/Games/MDPs/Wildfire.h"
#include "../../include/Games/MDPs/WildlifePreserve.h"
#include "../../include/Games/MDPs/RedFinnedBlueEye.h"
#include "../../include/Games/MDPs/PushYourLuck.h"
#include "../../include/Games/MDPs/Elevators.h"
#include "../../include/Games/MDPs/Tamarisk.h"
#include "../../include/Games/MDPs/AcademicAdvising.h"
#include "../../include/Games/MDPs/Traffic.h"
#include "../../include/Games/MDPs/CooperativeRecon.h"
#include "../../include/Utils/Argparse.h"
#include "../../include/Games/MDPs/Saving.h"
#include "../../include/Utils/Distributions.h"
#include "../../include/Agents/Oga/OgaAgent.h"
#include "../../include/Games/Wrapper/MultiPlayerToMDP.h"

#include "../../include/Games/TwoPlayerGames/Constrictor.h"
#include "../../include/Games/TwoPlayerGames/Connect4.h"
#include "../../include/Games/TwoPlayerGames/Othello.h"
#include "../../include/Games/TwoPlayerGames/Pusher.h"

#include <filesystem>
#include <algorithm>
#include <iostream>

#include "../../include/Agents/OneStepLookahead.h"
#include "../../include/Games/Wrapper/Determinization.h"
#include "../../include/Games/Wrapper/HeuristicsAsReward.h"

namespace MISC{

double round(double d, int precision) {
    return std::round(d * std::pow(10, precision)) / std::pow(10, precision);
}


void estimateQAbstractionsKVDA() {
    std::vector<std::pair<std::string,ABS::Model*>> model_list = {};

    model_list.push_back({"Academic Advising",new  AA::Model("../resources/AcademicAdvisingCourses/2_Anand.txt",false,false)});
    model_list.emplace_back("Connect4", new C4::Model(true));
    model_list.emplace_back("Constrictor",  new CON::Model(10,true));
    model_list.push_back({"Cooperative Recon", new  RECON::ReconModel("../resources/CooperativeReconSetups/3_IPPC.txt")});
    model_list.emplace_back("Earth Observation", new  EO::Model("../resources/EarthObservationMaps/1_IPPC.txt"));
    model_list.emplace_back("Elevators", new  ELE::Model("../resources/ElevatorSetups/10_IPPC.txt"));
    model_list.emplace_back("Game of Life", new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::SAVE_ONLY));
    model_list.emplace_back("Manufacturer", new  MAN::Model("../resources/ManufacturerSetups/3_IPPC.txt"));
    model_list.emplace_back("Othello",  new OTH::Model(true));
    model_list.emplace_back("Pusher",  new PUS::Model(true,"../resources/PusherMaps/standard.txt"));
    model_list.emplace_back("Push Your Luck", new  PushYL::Model("../resources/DiceProbs/10_IPPC.txt"));
    model_list.emplace_back("Red Finned Blue Eye", new  RFBE::Model("../resources/RedFinnedBlueEyesMaps/1_IPPC.txt",false));
    model_list.emplace_back( "Sailing Wind", new  SW::Model(15,15, false));
    model_list.emplace_back("Saving", new  SAVING::Model(-4,4,4,4));
    model_list.emplace_back("Skills Teaching", new  ST::SkillsTeachingModel("../resources/SkillsTeachingSkills/5_IPPC.txt",false, true));
    model_list.emplace_back("SysAdmin ", new  SA::Model("../resources/SysAdminTopologies/4_Anand.txt"));
    model_list.emplace_back("Tamarisk", new  TAM::Model("../resources/TamariskMaps/2_IPPC.txt"));
    model_list.emplace_back("Traffic", new  TR::TrafficModel("../resources/TrafficModels/1_IPPC.txt"));
    model_list.emplace_back("Wildfire", new  WF::Model("../resources/WildfireSetups/1_IPPC.txt"));
    model_list.emplace_back("Wildlife Preserve", new  WLP::Model("../resources/WildlifeSetups/4_IPPC.txt"));

    for (auto & [name, modelx] : model_list) {

        ABS::Model* model;
        if (modelx->getNumPlayers() == 2) {
            model = new MPTOMDP::Model(new HEURISTICSASREWARD::Model(modelx,true), {{1, new OSLA::OneStepLookaheadAgent()}}, 1.0, 0, true);
        }
        else
            model = new DETERMINIZATION::Model(modelx, true);

        std::ostringstream stream1;
        stream1 << name << " & ";

        int horizon = model->getNumPlayers() == 1? 50 : 200;
        auto distr_agent = OGA::OgaAgent({.budget={500,"iterations"},.exploration_parameter=2.0});
        for (int i = 0; i < 3; i++) {
            std::vector<std::string> row = {name};
            auto oga = OGA::OgaAgent({
               .budget={
               1000,
               "iterations"
               },
               .recency_count_limit=3,
               .exploration_parameter=2.0,
               .discount=1.0,
               .num_rollouts = 1,
               .rollout_length = -1,
               .behavior_flags={
               .group_terminal_states=true,
               .group_partially_expanded_states=false,
               .partial_expansion_group_threshold=9999,
               .eps_a = i == 2? 9999999999.0 : 0.0,
               .eps_t = 0,
                .consider_missing_outcomes = false,
                .alpha = 0,
                   .equiv_chance = 1.0,
               .state_abs_alg = "asap",
                .smart_reward_handling = (i == 0),
               },
                .track_statistics = true,
                .Q_map = nullptr,
                .distribution_agent = &distr_agent
               });

            const int num_maps = model->getNumPlayers() == 1? 300: 150;
            std::mt19937 rng1(static_cast<unsigned int>(42));

            if (model->getNumPlayers() == 1)
                playGames(*model, num_maps, {&oga}, rng1, MUTED, {horizon,horizon});
            else {
                auto default_agent = getDefaultAgent(true);
                playGames(*model, num_maps, {&oga,default_agent}, rng1, MUTED, {horizon,horizon / 4});
                delete default_agent;
            }

            stream1 << std::fixed << std::setprecision(2) << round(oga.getStatistics("trivial_q_abs_num",-1) / (double) oga.getStatistics("total_q_abs_num",-1),2);
            if (i == 0 || i == 1)
                stream1 << " & ";
        }
        stream1 << " \\\\";
        std::string result = stream1.str();
        std::ofstream outfile("../nobackup/qrates.txt", std::ios::app); // Open in append mode
        if (outfile.is_open()) {
            outfile << result << std::endl;
            outfile.close();
        } else {
            std::cerr << "Failed to open the file." << std::endl;
        }
        std::cout << std::endl;

    }

    //free models
    for(auto& [name, model] : model_list)
    delete model;
}

}
