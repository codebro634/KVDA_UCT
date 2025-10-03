#include "../../include/Utils/MiscAnalysis.h"
#include "../../include/Arena.h"
#include "../../include/Utils/ValueIteration.h"
#include "../../include/Utils/AgentMaker.h"
#include <fstream>
#include <iomanip>
#include <sstream>

#include "../../include/Agents/RandomAgent.h"
#include "../../include/Games/MDPs/SailingWind.h"
#include "../../include/Games/MDPs/Navigation.h"
#include "../../include/Games/MDPs/SkillsTeaching.h"
#include "../../include/Games/MDPs/SysAdmin.h"
#include "../../include/Games/MDPs/TriangleTireworld.h"
#include "../../include/Games/MDPs/EarthObservation.h"
#include "../../include/Games/MDPs/Manufacturer.h"
#include "../../include/Games/MDPs/GameOfLife.h"
#include "../../include/Games/MDPs/Wildfire.h"
#include "../../include/Games/MDPs/JoinFive.h"
#include "../../include/Games/MDPs/WildlifePreserve.h"
#include "../../include/Games/MDPs/RedFinnedBlueEye.h"
#include "../../include/Games/MDPs/PushYourLuck.h"
#include "../../include/Games/MDPs/WildlifePreserve.h"
#include "../../include/Games/MDPs/CrossingTraffic.h"
#include "../../include/Games/MDPs/Elevators.h"
#include "../../include/Games/MDPs/Tamarisk.h"
#include "../../include/Games/Wrapper/FiniteHorizon.h"
#include "../../include/Games/MDPs/RaceTrack.h"
#include "../../include/Games/MDPs/AcademicAdvising.h"
#include "../../include/Games/MDPs/Traffic.h"
#include "../../include/Games/MDPs/CooperativeRecon.h"
#include "../../include/Utils/Argparse.h"
#include "../../include/Games/MDPs/Saving.h"
#include "../../include/Utils/Distributions.h"
#include "../../include/Agents/Oga/OgaAgent.h"
#include "../../include/Games/Wrapper/MultiPlayerToMDP.h"
#include "../../include/Games/MDPs/ToySoccer.h"
#include "../../include/Games/MDPs/MultiArmedBandit.h"

#include "../../include/Games/TwoPlayerGames/TicTacToe.h"
#include "../../include/Games/TwoPlayerGames/Chess.h"
#include "../../include/Games/TwoPlayerGames/Constrictor.h"
#include "../../include/Games/TwoPlayerGames/Pylos.h"
#include "../../include/Games/TwoPlayerGames/Quarto.h"
#include "../../include/Games/TwoPlayerGames/CaptureTheFlag.h"
#include "../../include/Games/TwoPlayerGames/KillTheKing.h"
#include "../../include/Games/TwoPlayerGames/NumbersRace.h"
#include "../../include/Games/TwoPlayerGames/Connect4.h"
#include "../../include/Games/TwoPlayerGames/Othello.h"
#include "../../include/Games/TwoPlayerGames/Pusher.h"

#include <filesystem>
#include <algorithm>
#include <iostream>

#include "../../include/Agents/OneStepLookahead.h"
#include "../../include/Agents/Mcts/MctsAgent.h"
#include "../../include/Games/Wrapper/Determinization.h"
#include "../../include/Games/Wrapper/HeuristicsAsReward.h"
#include "../../include/Utils/MemoryAnalysis.h"
#include "../../include/Agents/Aupo/AupoAgent.h"

namespace MISC{

double round(double d, int precision) {
    return std::round(d * std::pow(10, precision)) / std::pow(10, precision);
}

void timeMeasKvda() {
    auto distr_agent = new RandomAgent(); //important to not interfere with time measurements
    std::vector<Agent*> kvdas,ogas;
    for (int its : {100,2000})
    {
        auto oga =  OGA::OgaAgent({
           .budget={
               its,
               "iterations"
           },
           .recency_count_limit=3,
           .exploration_parameter=2.0,
           .discount=1.0,
           .num_rollouts = 1,
            .rollout_length = -1,
           .behavior_flags={
               .q_abs_alg = "eps",
           },
        .distribution_agent = distr_agent,
       });
        ogas.push_back(new OGA::OgaAgent(oga));

        auto kvda =  OGA::OgaAgent({
       .budget={
           its,
           "iterations"
       },
       .recency_count_limit=3,
       .exploration_parameter=2.0,
       .discount=1.0,
       .num_rollouts = 1,
        .rollout_length = -1,
       .behavior_flags={
           .smart_reward_handling = true,
       },
        .distribution_agent = distr_agent,
        });
        kvdas.push_back(new OGA::OgaAgent(kvda));
    }
    generalTimeMeasurements(kvdas,ogas, "kvda");

    delete distr_agent;
    for (auto a : kvdas)
        delete a;
    for (auto a : ogas)
        delete a;
}

void timeMeasOgaEps() {
    auto distr_agent = new RandomAgent(); //important to not interfere with time measurements
    std::vector<Agent*> epses,ogas;
    for (int its : {100,2000})
    {
        auto oga =  OGA::OgaAgent({
           .budget={
               its,
               "iterations"
           },
           .recency_count_limit=3,
           .exploration_parameter=2.0,
           .discount=1.0,
           .num_rollouts = 1,
            .rollout_length = -1,
           .behavior_flags={
               .q_abs_alg = "eps",
           },
        .distribution_agent = distr_agent,
       });
        ogas.push_back(new OGA::OgaAgent(oga));

        auto eps =  OGA::OgaAgent({
       .budget={
           its,
           "iterations"
       },
       .recency_count_limit=3,
       .exploration_parameter=2.0,
       .discount=1.0,
       .num_rollouts = 1,
        .rollout_length = -1,
       .behavior_flags={
           .eps_a = 1,
           .eps_t = 0.4,
       },
        .distribution_agent = distr_agent,
        });
        epses.push_back(new OGA::OgaAgent(eps));
    }
    generalTimeMeasurements(epses,ogas, "eps");

    delete distr_agent;
    for (auto a : epses)
        delete a;
    for (auto a : ogas)
        delete a;
}

void timeMeasDrop() {
    auto distr_agent = new RandomAgent(); //important to not interfere with time measurements
    std::vector<Agent*> drops,ogas;
    for (int its : {100,2000})
    {
        auto oga =  OGA::OgaAgent({
           .budget={
               its,
               "iterations"
           },
           .recency_count_limit=3,
           .exploration_parameter=2.0,
           .discount=1.0,
           .num_rollouts = 1,
            .rollout_length = -1,
           .behavior_flags={
               .q_abs_alg = "eps",
               .eps_t = 0.8,
           },
        .distribution_agent = distr_agent,
       });
        ogas.push_back(new OGA::OgaAgent(oga));

        auto drop =  OGA::OgaAgent({
       .budget={
           its,
           "iterations"
       },
       .recency_count_limit=3,
       .exploration_parameter=2.0,
       .discount=1.0,
       .num_rollouts = 1,
        .rollout_length = -1,
       .behavior_flags={
           .eps_t = 0.8,
           .drop_confidence = 0.5,
       },
        .distribution_agent = distr_agent,
        });
        drops.push_back(new OGA::OgaAgent(drop));
    }
    generalTimeMeasurements(drops,ogas, "drop");

    delete distr_agent;
    for (auto a : drops)
        delete a;
    for (auto a : ogas)
        delete a;
}

void timeMeasIPA()
{
    auto distr_agent = new RandomAgent(); //important to not interfere with time measurements
    std::vector<Agent*> ipas,ogas;
    for (int its : {100,2000})
    {
        auto oga =  OGA::OgaAgent({
           .budget={
               its,
               "iterations"
           },
           .recency_count_limit=3,
           .exploration_parameter=2.0,
           .discount=1.0,
           .num_rollouts = 1,
            .rollout_length = -1,
           .behavior_flags={
               .q_abs_alg = "eps",
           },
        .distribution_agent = distr_agent,
       });
        ogas.push_back(new OGA::OgaAgent(oga));

        auto ipa =  OGA::OgaAgent({
       .budget={
           its,
           "iterations"
       },
       .recency_count_limit=3,
       .exploration_parameter=2.0,
       .discount=1.0,
       .num_rollouts = 1,
        .rollout_length = -1,
       .behavior_flags={
           .q_abs_alg = "eps",
           .state_abs_alg = "exp",
       },
        .distribution_agent = distr_agent,
        });
        ipas.push_back(new OGA::OgaAgent(ipa));
    }
    generalTimeMeasurements(ipas,ogas, "ipa");

    delete distr_agent;
    for (auto a : ipas)
        delete a;
    for (auto a : ogas)
        delete a;
}

void timeMeasIntra()
{
    auto distr_agent = new RandomAgent(); //important to not interfere with time measurements
    std::vector<Agent*> intras,ogas;
    for (int its : {100,2000})
    {
        auto oga =  OGA::OgaAgent({
           .budget={
               its,
               "iterations"
           },
           .recency_count_limit=3,
           .exploration_parameter=2.0,
           .discount=1.0,
           .num_rollouts = 1,
            .rollout_length = -1,
           .behavior_flags={
               .q_abs_alg = "eps",
               .eps_t = 0.8,
           },
        .in_abs_policy = "random",
        .distribution_agent = distr_agent,
       });
        ogas.push_back(new OGA::OgaAgent(oga));

        auto intra =  OGA::OgaAgent({
       .budget={
           its,
           "iterations"
       },
       .recency_count_limit=3,
       .exploration_parameter=2.0,
       .discount=1.0,
       .num_rollouts = 1,
        .rollout_length = -1,
       .behavior_flags={
           .q_abs_alg = "eps",
           .eps_t = 0.8,
       },
        .in_abs_policy = "uct",
        .distribution_agent = distr_agent,
        });
        intras.push_back(new OGA::OgaAgent(intra));
        }
    generalTimeMeasurements(intras,ogas, "intra");

    delete distr_agent;
    for (auto a : intras)
        delete a;
    for (auto a : ogas)
        delete a;
}

void timeMeasAupo()
{
    auto distr_agent = new RandomAgent(); //important to not interfere with time measurements
    std::vector<Agent*> aupos,mctss;
    for (int its : {100,2000})
    {
        auto aupo = new AUPO::AupoAgent(AUPO::AupoArgs({its, "iterations"}, {2} , 1.0, -1, 4, true,0.8,  0.8, true,  false, 0, false, false, 0, -1, -1, false, false, distr_agent));
        aupos.push_back(aupo);
        auto mcts = new AUPO::AupoAgent(AUPO::AupoArgs({its, "iterations"}, {2} , 1.0, -1, 4, true,0.8,  0.8, true,  false, 0, false, false, 0, -1, -1, false, true, distr_agent));
        mctss.push_back(mcts);
    }
    generalTimeMeasurements(aupos,mctss, "aupo");

    delete distr_agent;
    for (auto a : aupos)
        delete a;
    for (auto a : mctss)
        delete a;
}

void generalTimeMeasurements(std::vector<Agent*> a1, std::vector<Agent*> a2, std::string mode){
    std::vector<std::pair<std::string,ABS::Model*>> model_list = {};

    //kvda, aupo, drop, intra, ipa, eps

    //  model_list.push_back({"Academic Advising",new  AA::Model("../resources/AcademicAdvisingCourses/2_Anand.txt",mode == "aupo" || mode=="kvda",false)});
    //  model_list.push_back({"Cooperative Recon", new  RECON::ReconModel("../resources/CooperativeReconSetups/3_IPPC.txt")});
    // if (mode != "aupo" && mode != "kvda")
    //     model_list.emplace_back("Crossing Traffic", new  CT::Model(4,3,0.5,true));
    //  model_list.emplace_back("Earth Observation", new  EO::Model("../resources/EarthObservationMaps/1_IPPC.txt"));

    if (mode == "kvda")
        model_list.emplace_back("Elevators", new  ELE::Model("../resources/ElevatorSetups/10_IPPC.txt"));

     // model_list.emplace_back("Game of Life", new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::SAVE_ONLY));
     // model_list.emplace_back("Manufacturer", new  MAN::Model("../resources/ManufacturerSetups/3_IPPC.txt"));
     // model_list.emplace_back( "Sailing Wind", new  SW::Model(15,15, false));
     // model_list.emplace_back("Saving", new  SAVING::Model(-4,4,4,4));
     // model_list.emplace_back("Skills Teaching", new  ST::SkillsTeachingModel("../resources/SkillsTeachingSkills/5_IPPC.txt",false, true));
     // model_list.emplace_back("SysAdmin ", new  SA::Model("../resources/SysAdminTopologies/4_Anand.txt"));
     // model_list.emplace_back("Tamarisk", new  TAM::Model("../resources/TamariskMaps/2_IPPC.txt"));
     // model_list.emplace_back("Traffic", new  TR::TrafficModel("../resources/TrafficModels/1_IPPC.txt"));
     // if (mode != "aupo" && mode != "kvda")
     //    model_list.emplace_back("Triangle Tireworld", new  TRT::Model("../resources/TriangleTireworlds/5_IPPC.txt",false,true, true));
     // if (mode == "aupo" || mode == "kvda")
     //    model_list.emplace_back("Push Your Luck", new  PushYL::Model("../resources/DiceProbs/10_IPPC.txt"));
     // if (mode == "ipa")
     //    model_list.emplace_back("Racetrack", new  RT::Model("../resources/Racetracks/ring-2.track", 0.0, false, false));

    if (mode == "kvda")
        model_list.emplace_back("Red Finned Blue Eye", new  RFBE::Model("../resources/RedFinnedBlueEyesMaps/1_IPPC.txt",false));

    // if (mode == "aupo") {
    //     std::vector<std::pair<double,double>> arm_distrs = {{10,9},{1,10}};
    //     model_list.push_back({"Multi-armed bandit", new  MAB::Model(arm_distrs,10)});
    // }
    // if (mode != "aupo" && mode != "kvda")
    //     model_list.emplace_back("Navigation", new  Navigation::Model("../resources/NavigationMaps/3_Anand.txt", false, false));
    // if (mode == "kvda")
    //  model_list.emplace_back("Wildfire", new  WF::Model("../resources/WildfireSetups/1_IPPC.txt"));
    if (mode == "kvda")
        model_list.emplace_back("Wildlife Preserve", new  WLP::Model("../resources/WildlifeSetups/4_IPPC.txt"));

    // if (mode == "kvda" || mode == "ipa") {
    //     model_list.emplace_back("Othello",  new OTH::Model(true));
    //     model_list.emplace_back("Connect4", new C4::Model(true));
    //     model_list.emplace_back("Constrictor",  new CON::Model(10,true));
    //     if (mode == "ipa") {
    //         model_list.emplace_back("NumbersRace",new NUM::Model(200,15,true));
    //         model_list.emplace_back("Pylos",  new PYL::Model(true));
    //         model_list.emplace_back("Quarto" , new QUA::Model(true));
    //         model_list.emplace_back("Chess", new CHE::Model(true));
    //         model_list.emplace_back("Tic Tac Toe" , new TTT::Model(true));
    //     }
    // }

    for (const auto& model : model_list){
        int episodes = model.second->getNumPlayers() == 1 ? 100:50;
        std::mt19937 rng(static_cast<unsigned int>(42));
        int horizon = model.second->getNumPlayers() == 1? 50 : 200;
        auto random_agent = new RandomAgent();

        ABS::Model* raw_model;
        if (mode == "kvda") {
            if (model.second->getNumPlayers() == 1)
                raw_model = new DETERMINIZATION::Model(model.second, false);
            else {
                auto tmp = new HEURISTICSASREWARD::Model(model.second, false);
                raw_model = new MPTOMDP::Model(tmp, {{1, new OSLA::OneStepLookaheadAgent()}}, 1.0, 0, true, false);
            }
        }else
            raw_model = model.second;
        auto e_model = new FINITEH::Model(raw_model, horizon, false);

        assert (a1.size() == a2.size());

        std::vector<double> times_a1,times_a2;
        for (int i = 0; i < (int)a1.size(); i++) {
            times_a1.push_back(0.0);
            times_a2.push_back(0.0);
        }
        int state_ctr = 0;
        for (int e = 0; e < episodes; e++) {
            auto start_state = e_model->getInitialState(rng);
            while (!start_state->terminal) {
                auto actions = e_model->getActions(start_state);
                auto action = actions[rng() % actions.size()];

                for (int i = 0; i < (int)a1.size(); i++) {
                    auto rng_copy = rng; //copy rng to not interfere with the agent's rng
                    auto start_time = std::chrono::high_resolution_clock::now();
                    a1[i]->getAction(e_model,start_state, rng_copy);
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
                    times_a1[i] += duration / 1000.0; //convert to milliseconds
                }
                for (int i = 0; i < (int)a2.size(); i++) {
                    auto rng_copy = rng; //copy rng to not interfere with the agent's rng
                    auto start_time = std::chrono::high_resolution_clock::now();
                    a2[i]->getAction(e_model,start_state, rng_copy);
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
                    times_a2[i] += duration / 1000.0; //convert to milliseconds
                }

                e_model->applyAction(start_state, action, rng);
                state_ctr++;
            }
        }
        delete e_model;

        for (int i = 0; i < (int)a1.size(); i++) {
            times_a1[i] /= state_ctr;
            times_a2[i] /= state_ctr;
        }

        std::ostringstream stream;
        std::cout << model.first << " & ";
        stream << model.first << " & ";
        for (int i = 0; i < (int)a1.size(); i++) {
            std::cout << std::fixed << std::setprecision(2) << round(times_a1[i],2) << " & " <<  std::fixed << std::setprecision(2) << round(times_a2[i],2) << (i < ((int)a1.size())-1? " & " : "");
            stream << std::fixed << std::setprecision(2) << round(times_a1[i],2) << " & " <<  std::fixed << std::setprecision(2) << round(times_a2[i],2) << (i < ((int)a1.size())-1? " & " : "");
        }
        std::cout << "\\\\" << std::endl;
        stream << "\\\\" << std::endl;

        std::string result = stream.str();
        std::ofstream outfile("../nobackup/" + mode + "_times.txt", std::ios::app); // Open in append mode
        if (outfile.is_open()) {
            outfile << result << std::endl;
            outfile.close();
        } else {
            std::cerr << "Failed to open the file." << std::endl;
        }

        delete random_agent;
    }

    //free models
    for (auto & [name, model] : model_list) {
        delete model;
    }

}


void estimateQAbstractionsOgaEps() {
    std::vector<std::pair<std::string,ABS::Model*>> model_list = {};

    model_list.push_back({"Academic Advising",new  AA::Model("../resources/AcademicAdvisingCourses/2_Anand.txt",false,false)});
    model_list.push_back({"Cooperative Recon", new  RECON::ReconModel("../resources/CooperativeReconSetups/3_IPPC.txt")});
    model_list.emplace_back("Earth Observation", new  EO::Model("../resources/EarthObservationMaps/1_IPPC.txt"));
    model_list.emplace_back("Game of Life", new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::SAVE_ONLY));
    model_list.emplace_back("Manufacturer", new  MAN::Model("../resources/ManufacturerSetups/3_IPPC.txt"));
    model_list.emplace_back( "Sailing Wind", new  SW::Model(15,15, false));
    model_list.emplace_back("Saving", new  SAVING::Model(-4,4,4,4));
    model_list.emplace_back("Skills Teaching", new  ST::SkillsTeachingModel("../resources/SkillsTeachingSkills/5_IPPC.txt",false, true));
    model_list.emplace_back("SysAdmin ", new  SA::Model("../resources/SysAdminTopologies/4_Anand.txt"));
    model_list.emplace_back("Tamarisk", new  TAM::Model("../resources/TamariskMaps/2_IPPC.txt"));
    model_list.emplace_back("Traffic", new  TR::TrafficModel("../resources/TrafficModels/1_IPPC.txt"));
    model_list.emplace_back("Triangle Tireworld", new  TRT::Model("../resources/TriangleTireworlds/5_IPPC.txt", false, true, true));
    model_list.emplace_back("Racetrack", new  RT::Model("../resources/Racetracks/ring-2.track", 0.0, false, false));
    model_list.emplace_back("Navigation", new  Navigation::Model("../resources/NavigationMaps/3_Anand.txt", false, false));
    model_list.emplace_back("Crossing Traffic", new  CT::Model(4,3,0.5,true));

    for (auto & [name, model] : model_list) {

        std::ostringstream stream1;
        stream1 << name << " & ";

        const int horizon = 50;
        const int num_maps = 200;
        auto distr_agent = Mcts::MctsAgent({.budget={500, "iterations"}, .exploration_parameters = {2}, .dynamic_exploration_factor = true});

        //Oga Eps
        for (double eps_t : {0.0, 0.2, 0.4, 0.8, 1.2, 1.6, 2.0}) {
            std::vector<std::string> row = {name};
            auto oga = *dynamic_cast<OGA::OgaAgent*>(getAgent("oga", {
                "iterations=500",
                "eps_t=" + std::to_string(eps_t),
                "track_statistics=1"
            }));
            std::mt19937 rng1(static_cast<unsigned int>(42));
            playGames(model, num_maps, {&oga}, rng1, MUTED, {horizon,horizon});
            stream1 << std::fixed << std::setprecision(2) << round(1.0 - oga.getStatistics("trivial_q_abs_num",-1) / (double) oga.getStatistics("total_q_abs_num",-1),2);
            stream1 << " & ";
            double avg_size = (oga.getStatistics("trivial_q_abs_num",-1) + oga.getStatistics("non_trivial_q_abs_count_sum",-1)) / (oga.getStatistics("trivial_q_abs_num",-1) + oga.getStatistics("non_trivial_q_abs_num",-1));
            stream1 << std::fixed << std::setprecision(2) << round(avg_size,2) << " & ";
        }

        //Oga Eps, ignore reward
        for (double eps_t : {0.0, 0.2, 0.4, 0.8, 1.2, 1.6, 2.0}) {
            std::vector<std::string> row = {name};
            auto oga = *dynamic_cast<OGA::OgaAgent*>(getAgent("oga", {
                "iterations=500",
                "eps_t=" + std::to_string(eps_t),
                "eps_a=999999",
                "track_statistics=1"
            }));
            std::mt19937 rng1(static_cast<unsigned int>(42));
            playGames(model, num_maps, {&oga}, rng1, MUTED, {horizon,horizon});
            stream1 << std::fixed << std::setprecision(2) << round(1.0 - oga.getStatistics("trivial_q_abs_num",-1) / (double) oga.getStatistics("total_q_abs_num",-1),2);
            stream1 << " & ";
            double avg_size = (oga.getStatistics("trivial_q_abs_num",-1) + oga.getStatistics("non_trivial_q_abs_count_sum",-1)) / (oga.getStatistics("trivial_q_abs_num",-1) + oga.getStatistics("non_trivial_q_abs_num",-1));
            stream1 << std::fixed << std::setprecision(2) << round(avg_size,2) << " & ";
        }

        //pruned OGA
        for (double alpha : {0.1,0.2,0.5,0.75,1.0}) {
            std::vector<std::string> row = {name};
            auto oga = *dynamic_cast<OGA::OgaAgent*>(getAgent("oga", {
                "iterations=500",
                "alpha=" + std::to_string(alpha),
                "track_statistics=1"
            }));
            std::mt19937 rng1(static_cast<unsigned int>(42));
            playGames(model, num_maps, {&oga}, rng1, MUTED, {horizon,horizon});
            stream1 << std::fixed << std::setprecision(2) << round(1.0 - oga.getStatistics("trivial_q_abs_num",-1) / (double) oga.getStatistics("total_q_abs_num",-1),2);
            stream1 << " & ";
            double avg_size = (oga.getStatistics("trivial_q_abs_num",-1) + oga.getStatistics("non_trivial_q_abs_count_sum",-1)) / (oga.getStatistics("trivial_q_abs_num",-1) + oga.getStatistics("non_trivial_q_abs_num",-1));
            stream1 << std::fixed << std::setprecision(2) << round(avg_size,2) << " & ";
        }

        stream1 << " \\\\";
        std::string result = stream1.str();
        std::ofstream outfile("../nobackup/qrates_ogaeps.txt", std::ios::app); // Open in append mode
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
            model = new MPTOMDP::Model(new HEURISTICSASREWARD::Model(modelx,true), {{1, new OSLA::OneStepLookaheadAgent()}}, 1.0, 0, true, true);
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
               .drop_confidence = -1,
               .drop_at_visits = 10000000,
               .state_abs_alg = "asap",
               .min_visits = 0,
               .top_n_matches = 1,
                .confidence=0.8,
                .filter_bonus = 0,
                .equiv_chance = 1.0,
                .smart_reward_handling = (i == 0),
               },
               .drop_check_point = 1.1,
               .drop_threshold = 10000000000,
                .in_abs_policy = "random",
                .track_statistics = true,
                .Q_map = nullptr,
                .distribution_agent = &distr_agent
               });

            const int num_maps = model->getNumPlayers() == 1? 300: 150;
            std::mt19937 rng1(static_cast<unsigned int>(42));

            if (model->getNumPlayers() == 1)
                playGames(model, num_maps, {&oga}, rng1, MUTED, {horizon,horizon});
            else {
                auto default_agent = getDefaultAgent(true);
                playGames(model, num_maps, {&oga,default_agent}, rng1, MUTED, {horizon,horizon / 4});
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

void measureIntraAbsRate(){
     std::vector<std::pair<std::string,ABS::Model*>> model_list = {};

    // model_list.emplace_back("wf", new  WF::Model("../resources/WildfireSetups/1_IPPC.txt"));
    // model_list.emplace_back("rfbe", new  RFBE::Model("../resources/RedFinnedBlueEyesMaps/1_IPPC.txt", false));
    // model_list.emplace_back("ele", new  ELE::Model("../resources/ElevatorSetups/10_IPPC.txt", false));

    // model_list.push_back({"Academic Advising",new  AA::Model("../resources/AcademicAdvisingCourses/2_Anand.txt",false,false)});
    // model_list.push_back({"Cooperative Recon", new  RECON::ReconModel("../resources/CooperativeReconSetups/3_IPPC.txt")});
    // model_list.emplace_back( "Crossing Traffic" , new  CT::Model(4,3,0.5, true));
    // model_list.emplace_back("Earth Observation", new  EO::Model("../resources/EarthObservationMaps/1_IPPC.txt"));
    // model_list.emplace_back("Game of Life", new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::SAVE_ONLY));
    // model_list.emplace_back("Manufacturer", new  MAN::Model("../resources/ManufacturerSetups/3_IPPC.txt"));
    // model_list.emplace_back("Navigation", new  Navigation::Model("../resources/NavigationMaps/3_Anand.txt",false));
    // model_list.emplace_back("Racetrack", new  RT::Model("../resources/Racetracks/ring-2.track", 0.0, false, false));
    // model_list.emplace_back( "Sailing Wind", new  SW::Model(15,15, false));
    model_list.emplace_back("Saving", new  SAVING::Model(-4,4,4,4));
    // model_list.emplace_back("Skills Teaching", new  ST::SkillsTeachingModel("../resources/SkillsTeachingSkills/5_IPPC.txt",false, true));
    // model_list.emplace_back("SysAdmin ", new  SA::Model("../resources/SysAdminTopologies/4_Anand.txt"));
    // model_list.emplace_back("Tamarisk", new  TAM::Model("../resources/TamariskMaps/2_IPPC.txt"));
    // model_list.emplace_back("Traffic", new  TR::TrafficModel("../resources/TrafficModels/1_IPPC.txt"));
    // model_list.emplace_back("Triangle Tireworld", new  TRT::Model("../resources/TriangleTireworlds/5_IPPC.txt",false,true, true));

    for (auto & [name, model] : model_list) {

        std::ostringstream stream1;
        stream1 << name << " & ";

        int horizon = model->getNumPlayers() == 1? 50 : 100;

        std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare> Q_map = {};
        //MISC::createQTable(model, horizon, &Q_map, "", false,0);
        //std::cout << "Qtable done!" << std::endl;
        //loadQTable(model, Q_map, "../nobackup/Q_analysis/" + abbreviation_map[name] + ".txt");

        std::vector<std::string> row = {name};
        for (double eps : {0.0,0.8,1.6}) {
            for (bool partial : {false,true}) {
                auto oga = OGA::OgaAgent({
                   .budget={
                   1000,
                   "iterations"
                   },
                   .recency_count_limit=3,
                   .exploration_parameter=4.0,
                   .discount=1.0,
                   .num_rollouts = 1,
                   .rollout_length = -1,
                   .behavior_flags={
                   .group_terminal_states=true,
                   .group_partially_expanded_states=partial,
                   .partial_expansion_group_threshold=9999,
                   .eps_a = 0,
                   .eps_t = eps,
                    .consider_missing_outcomes = false,
                    .alpha = 0,
                   .drop_confidence = -1,
                   .drop_at_visits = 10000000,
                   .min_visits = 0,
                   .top_n_matches = 1,
                    .confidence=0.8,
                    .filter_bonus = 0,
                    .equiv_chance = 1.0,
                   },
                   .drop_check_point = 1.1,
                   .drop_threshold = 10000000000,
                    .in_abs_policy = "random",
                    .track_statistics = true,
                    .Q_map = &Q_map,
                    .distribution_agent = nullptr
                   });

                const int num_maps = 50;
                std::mt19937 rng1(static_cast<unsigned int>(42));

                playGames(model, num_maps, {&oga}, rng1, MUTED, {horizon,horizon});

                stream1 << std::fixed << std::setprecision(2) << round(oga.getStatistics("intra_abs_required",-1) / (double) oga.getStatistics("ucb_calls",-1),2) << " & ";
            }
        }

        std::string result = stream1.str();
        std::ofstream outfile("../nobackup/intra_abs_stats.txt", std::ios::app); // Open in append mode
        if (outfile.is_open()) {
            outfile << result << std::endl;
            outfile.close();
        } else {
            std::cerr << "Failed to open the file." << std::endl;
        }
        std::cout << result <<  " \\\\" << std::endl;

    }

    //free models
    for(auto& [name, model] : model_list)
    delete model;
}

void estimateStateAbstractions() {
    std::vector<std::pair<std::string,ABS::Model*>> model_list = {};

    // model_list.emplace_back("Saving", new  SAVING::Model(-4,4,4,4));
    // model_list.emplace_back("Connect4", new C4::Model(true));
    // model_list.emplace_back("Quarto" , new QUA::Model(true));
    // model_list.emplace_back("CaptureTheFlag",  new CTF::Model(true,"../resources/CTFMaps/standard.txt"));
    // model_list.emplace_back("Pusher",  new PUS::Model(true,"../resources/PusherMaps/standard.txt"));
    // // model_list.emplace_back("chess", new CHE::Model(true));
    // model_list.emplace_back("KillTheKing", new KTK::Model(true,"../resources/KTKMaps/standard.txt"));
    // model_list.emplace_back("Othello",  new OTH::Model(true));
    // model_list.emplace_back("Pylos",  new PYL::Model(true));
    // model_list.emplace_back("NumbersRace",new NUM::Model(200,15,true));
    // model_list.emplace_back("Constrictor",  new CON::Model(10,true));
    // model_list.emplace_back("ttt" , new TTT::Model(true));
    
    // model_list.emplace_back("wf", new  WF::Model("../resources/WildfireSetups/1_IPPC.txt"));
    // model_list.emplace_back("rfbe", new  RFBE::Model("../resources/RedFinnedBlueEyesMaps/1_IPPC.txt", false));
    // model_list.emplace_back("ele", new  ELE::Model("../resources/ElevatorSetups/10_IPPC.txt", false));

    // model_list.emplace_back( "Sailing Wind", new  SW::Model(15,15, false));
    // model_list.push_back({"Academic Advising",new  AA::Model("../resources/AcademicAdvisingCourses/2_Anand.txt",false,false)});
    // model_list.push_back({"Cooperative Recon", new  RECON::ReconModel("../resources/CooperativeReconSetups/3_IPPC.txt")});
    // model_list.emplace_back("Game of Life", new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::SAVE_ONLY));
    // model_list.emplace_back("Earth Observation", new  EO::Model("../resources/EarthObservationMaps/1_IPPC.txt"));
    // model_list.emplace_back("Manufacturer", new  MAN::Model("../resources/ManufacturerSetups/3_IPPC.txt"));
    //
    model_list.emplace_back("Navigation", new  Navigation::Model("../resources/NavigationMaps/3_Anand.txt",false));
    // model_list.emplace_back("Racetrack", new  RT::Model("../resources/Racetracks/ring-2.track", 0.0, false, false));
    // model_list.emplace_back("Skills Teaching", new  ST::SkillsTeachingModel("../resources/SkillsTeachingSkills/5_IPPC.txt",false, true));
    // model_list.emplace_back( "Crossing Traffic" , new  CT::Model(4,3,0.5, true));
    // model_list.emplace_back("SysAdmin ", new  SA::Model("../resources/SysAdminTopologies/4_Anand.txt"));
    //
    // model_list.emplace_back("Tamarisk", new  TAM::Model("../resources/TamariskMaps/2_IPPC.txt"));
    // model_list.emplace_back("Traffic", new  TR::TrafficModel("../resources/TrafficModels/1_IPPC.txt"));
    // model_list.emplace_back("Triangle Tireworld", new  TRT::Model("../resources/TriangleTireworlds/5_IPPC.txt",false,true, true));

    // std::map<std::string,std::string> abbreviation_map = {
    // {"Academic Advising","aa"},
    // {"Cooperative Recon","recon"},
    // {"Game of Life","gol"},
    // {"Earth of Observation","eo"},
    // {"Manufacturer", "man"},
    // {"Navigation","navigation"},
    // {"Racetrack","rt"},
    // {"Sailing Wind","sw"},
    // {"Skills Teaching","st"},
    // {"SysAdmin","sa"},
    // {"Tamarisk","tam"},
    // {"Traffic","tr"},
    // {"Crossing Traffic","ct"},
    // {"Triangle Tireworld","trt"},
    //     {"Wildfire","wf"},
    //     {"Red Finned Blue Eyes","rfbe"},
    //     {"Elevators","ele"},
    //     {"Connect4","c4"},
    //     {"Quarto","qua"},
    //     {"CaptureTheFlag","ctf"},
    //     {"Pusher","pus"},
    //     {"KillTheKing","ktk"},
    //     {"Othello","oth"},
    //     {"Pylos","pyl"},
    //     {"NumbersRace","num"},
    //     {"Constrictor","con"}};

    auto distr_agent = OGA::OgaAgent({.budget={1000,"iterations"},.exploration_parameter=4.0});

    for (auto & [name, model] : model_list) {

        std::ostringstream stream1;
        stream1 << name << " & ";

        int horizon = model->getNumPlayers() == 1? 50 : 100;

        std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare> Q_map = {};
        //MISC::createQTable(model, horizon, &Q_map, "", false,0);
        //std::cout << "Qtable done!" << std::endl;
        //loadQTable(model, Q_map, "../nobackup/Q_analysis/" + abbreviation_map[name] + ".txt");

        std::vector<std::string> row = {name};
        for (double eps : {0.0,0.4}) {
            for (auto abs_framework : {"asap","exp"}) {

                if (eps == 0.4 && model->getNumPlayers() == 2)
                    continue;

                auto oga = OGA::OgaAgent({
                   .budget={
                   1000,
                   "iterations"
                   },
                   .recency_count_limit=3,
                   .exploration_parameter=4.0,
                   .discount=1.0,
                   .num_rollouts = 1,
                   .rollout_length = -1,
                   .behavior_flags={
                   .group_terminal_states=true,
                   .group_partially_expanded_states=false,
                   .partial_expansion_group_threshold=9999,
                   .eps_a = 0,
                   .eps_t = eps,
                    .consider_missing_outcomes = false,
                    .alpha = 0,
                   .drop_confidence = -1,
                   .drop_at_visits = 10000000,
                   .state_abs_alg = abs_framework,
                   .min_visits = 0,
                   .top_n_matches = 1,
                    .confidence=0.8,
                    .filter_bonus = 0,
                    .equiv_chance = 1.0,
                   },
                   .drop_check_point = 1.1,
                   .drop_threshold = 10000000000,
                    .in_abs_policy = "random",
                    .track_statistics = true,
                    .Q_map = &Q_map,
                    .distribution_agent = nullptr
                   });

                const int num_maps = model->getNumPlayers() == 1? 100: 50;
                std::mt19937 rng1(static_cast<unsigned int>(42));

                if (model->getNumPlayers() == 1)
                    playGames(model, num_maps, {&oga}, rng1, VERBOSE, {horizon,horizon});
                else {
                    auto default_agent = getDefaultAgent(true);
                    playGames(model, num_maps, {&oga,default_agent}, rng1, MUTED, {horizon,horizon / 2});
                    delete default_agent;
                }

                stream1 << std::fixed << std::setprecision(2) << round(oga.getStatistics("trivial_state_abs_num",-1) / (double) oga.getStatistics("total_state_abs_num",-1),2);
                if (eps != 0.4 || abs_framework != std::string("exp"))
                    stream1 << " & ";
            }
        }

        std::string result = stream1.str();
        std::ofstream outfile("../nobackup/output.txt", std::ios::app); // Open in append mode
        if (outfile.is_open()) {
            outfile << result << std::endl;
            outfile.close();
        } else {
            std::cerr << "Failed to open the file." << std::endl;
        }
        std::cout << result <<  " \\\\" << std::endl;

    }

    //free models
    for(auto& [name, model] : model_list)
    delete model;
}

void estimateAbsDropNumbers() {

    std::vector<std::pair<std::string,ABS::Model*>> model_list = {};
    model_list.push_back({"Academic Advising",new  AA::Model("../resources/AcademicAdvisingCourses/2_Anand.txt",false,false)});
    model_list.push_back({"Cooperative Recon", new  RECON::ReconModel("../resources/CooperativeReconSetups/3_IPPC.txt")});
    model_list.emplace_back("Crossing Traffic", new  CT::Model(4,3,0.5,true));
    model_list.emplace_back("Earth of Observation", new  EO::Model("../resources/EarthObservationMaps/1_IPPC.txt"));
    model_list.emplace_back("Game of Life", new  GOL::Model("../resources/GameOfLifeMaps/3_Anand.txt", GOL::ActionMode::SAVE_ONLY));
    model_list.emplace_back("Manufacturer", new  MAN::Model("../resources/ManufacturerSetups/3_IPPC.txt"));
    model_list.emplace_back("Navigation", new  Navigation::Model("../resources/NavigationMaps/3_Anand.txt",false));
    model_list.emplace_back( "Sailing Wind", new  SW::Model(15,15, false));
    model_list.emplace_back("Saving", new  SAVING::Model(-4,4,4,4));
    model_list.emplace_back("Skills Teaching", new  ST::SkillsTeachingModel("../resources/SkillsTeachingSkills/5_IPPC.txt",false, true));
    model_list.emplace_back("SysAdmin", new  SA::Model("../resources/SysAdminTopologies/4_Anand.txt"));
    model_list.emplace_back("Tamarisk", new  TAM::Model("../resources/TamariskMaps/2_IPPC.txt"));
    model_list.emplace_back("Traffic", new  TR::TrafficModel("../resources/TrafficModels/1_IPPC.txt"));
    model_list.emplace_back("Triangle Tireworld", new  TRT::Model("../resources/TriangleTireworlds/5_IPPC.txt",false,true, true));


    std::map<std::string,std::string> abbreviation_map = {
        {"Crossing Traffic", "ct"},
        {"Saving","saving"},
        {"Academic Advising","aa"},
        {"Cooperative Recon","recon"},
        {"Game of Life","gol"},
        {"Earth of Observation","eo"},
        {"Manufacturer", "man"},
        {"Navigation","navigation"},
        {"Racetrack","rt"},
        {"Sailing Wind","sw"},
        {"Skills Teaching","st"},
        {"SysAdim","sa"},
        {"Tamarisk","tam"},
        {"Traffic","tr"},
        {"Triangle Tireworld","trt"}};

    std::cout << R"(\begin{table*}[]\centering \scalebox{1.0}{\setlength{\tabcolsep}{1mm}\begin{tabular}{ |c|c|c|c|c|c|c|} \hline Domain & C-1 & C-2 & C-T  & F-1 & F-2 & F-T\\ \hline)" << std::endl;

    for(auto& [name, model] : model_list) {
        //std::cout << "----------- Model:" << name << " ----------- " << std::endl;

        std::cout << name << " & ";

        /*
         * Find the parameters for the oga agent for this environment
         */

        int rolloutLength = 5;
        if (name == "Tamarisk" || name == "Sailing Wind")
            rolloutLength = 10;
        else if (name == "Earth Observation" || name == "Academic Advising" || name == "Navigation" || name == "Triangle Tireworld")
            rolloutLength = 20;
        else if (name == "Cooperative Recon")
            rolloutLength = 50;

        /*
         * Setup the OGA agents
         */
        double p = 0.75;
        auto distr_agent = OGA::OgaAgent({.budget={500,"iterations"},.exploration_parameter=2.0});
        auto oga_fine = OGA::OgaAgent({
                       .budget={
                           1000,
                           "iterations"
                       },
                       .recency_count_limit=3,
                       .exploration_parameter=2.0,
                       .discount=1.0,
                       .num_rollouts = 10,
                        .rollout_length = rolloutLength,
                       .behavior_flags={
                           .group_terminal_states=true,
                           .group_partially_expanded_states=false,
                           .partial_expansion_group_threshold=9999,
                           .eps_a = 0,
                           .eps_t = 0.0,
                           .consider_missing_outcomes = false,
                           .alpha = 0,
                           .drop_confidence = p,
                           .drop_at_visits = 10000000
                       },
                        .drop_check_point = 1.1,
                        .drop_threshold = 10000000000,
                        .in_abs_policy = "random",
                        .track_statistics = true,
            .distribution_agent = &distr_agent,
                   });

        auto oga_coarse = OGA::OgaAgent({
                        .budget={
                           1000,
                           "iterations"
                        },
                        .recency_count_limit=3,
                        .exploration_parameter=2.0,
                        .discount=1.0,
                        .num_rollouts = 10,
                        .rollout_length = rolloutLength,
                        .behavior_flags={
                           .group_terminal_states=true,
                           .group_partially_expanded_states=true,
                           .partial_expansion_group_threshold=9999,
                           .eps_a = 999999,
                           .eps_t = 1.6,
                            .consider_missing_outcomes = false,
                            .alpha = 0,
                           .drop_confidence = p,
                           .drop_at_visits = 10000000
                        },
                        .drop_check_point = 1.1,
                        .drop_threshold = 10000000000,
                        .in_abs_policy = "random",
                        .track_statistics = true,
                        .distribution_agent = &distr_agent
                        });

        const int num_maps = 50;
        const double shift = 100;

        std::mt19937 rng1(static_cast<unsigned int>(43));
        playGames(model, num_maps, {&oga_coarse}, rng1, MUTED, {50,50});
        auto drops = oga_coarse.getLayerwiseStatistic("abs_drops");
        auto abs = oga_coarse.getLayerwiseStatistic("non_trivial_q_abs_count_sum");
        double root_rate, depth1_rate,total_drops=0, total_abs=0;
        root_rate = abs.contains(0)? drops[0] / (double) abs[0] : std::nan("");
        depth1_rate = abs.contains(1)? drops[1] / (double) abs[1] : std::nan("");
        for (auto& [depth, drop] : drops){
            if (depth >= 2) {
                total_drops += drop;
                total_abs += abs[depth];
            }
        }
        double tree_rate = total_abs > 0? total_drops / (double) total_abs  : std::numeric_limits<double>::quiet_NaN();

        //std::cout << "Coarse average drop rates: " << shift * root_rate << " " << shift * depth1_rate << " " << shift * tree_rate << std::endl;
        std::cout <<  round(shift * root_rate,4) << "\\% & " << round(shift * depth1_rate,4) << "\\%  & " << round(shift * tree_rate,4)<< "\\% & ";

        std::mt19937 rng2(static_cast<unsigned int>(43));
        playGames(model, num_maps, {&oga_fine}, rng2, MUTED, {50,50});
        drops = oga_fine.getLayerwiseStatistic("abs_drops");
        abs = oga_fine.getLayerwiseStatistic("non_trivial_q_abs_count_sum");
        root_rate = abs.contains(0)? drops[0] / (double) abs[0] : std::nan("");
        depth1_rate = abs.contains(1)? drops[1] / (double) abs[1] : std::nan("");
        total_drops = 0;
        total_abs = 0;
        for (auto& [depth, drop] : drops){
            if (depth >= 2) {
                total_drops += drop;
                total_abs += abs[depth];
            }
        }
        tree_rate = total_abs > 0? total_drops / (double) total_abs  : std::nan("");

        //std::cout << "Fine average drop rates: "  << shift * root_rate << " " << shift * depth1_rate << " " << shift * total_drops / (double) total_abs << std::endl;
        std::cout <<  round(shift * root_rate,4) << "\\% & " << round(shift * depth1_rate,4) << "\\% & " << round(shift * tree_rate,4) << R"(\% \\ \hline)" << std::endl;
    }

    std::cout << R"(\end{tabular}} \caption{Average ratio of abstraction-dropped Q nodes in OGA-CAD with $p=0.9$ that are part of non-trivial abstractions. The average is denoted for the entire tree, the first, or the second layer only. The columns have the format in which the first entry denotes whether a coarse (C) or fine abstraction has been used (F) and the second entry denotes the layer where T denotes that the entire tree starting at layer 3 has been taken.}  \label{tab:oga_cad_stats} \end{table*})" << std::endl;


    //free models
    for(auto& [name, model] : model_list)
        delete model;
   
}

std::string to_string_with_precision(double value, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

std::tuple<ABS::Model*, int, std::string,std::string> getModel(int idx) {
      std::vector<std::string> model_list = {"aa","saving","ts","j5", "sw" , "ct" ,"recon","gol","eo", "man", "navigation", "rt", "st", "sa", "tam", "tr", "trt", "wf", "rfbe","ele", "wlp","pushyl","pyl",
        "ctf","che","c4","con","ktk","num","oth","pus","qua","ttt"};

    std::map<std::string,std::string> model_to_folder = {};
    model_to_folder.insert({"aa","../resources/AcademicAdvisingCourses"});
    model_to_folder.insert({"recon","../resources/CooperativeReconSetups"});
    model_to_folder.insert({"gol","../resources/GameOfLifeMaps"});
    model_to_folder.insert({"eo","../resources/EarthObservationMaps"});
    model_to_folder.insert({"man","../resources/ManufacturerSetups"});
    model_to_folder.insert({"navigation","../resources/NavigationMaps"});
    model_to_folder.insert({"rt", "../resources/Racetracks"});
    model_to_folder.insert({"st","../resources/SkillsTeachingSkills"});
    model_to_folder.insert({"sa","../resources/SysAdminTopologies"});
    model_to_folder.insert({"tam","../resources/TamariskMaps"});
    model_to_folder.insert({"tr","../resources/TrafficModels"});
    model_to_folder.insert({"trt","../resources/TriangleTireworlds"});
    model_to_folder.insert({"wf","../resources/WildfireSetups"});
    model_to_folder.insert({"rfbe","../resources/RedFinnedBlueEyesMaps"});
    model_to_folder.insert({"ele","../resources/ElevatorSetups"});
    model_to_folder.insert({"wlp","../resources/WildlifeSetups"});
    model_to_folder.insert({"pushyl","../resources/DiceProbs"});
    model_to_folder.insert({"ktk","../resources/KTKMaps"});
    model_to_folder.insert({"ctf","../resources/CTFMaps"});
    model_to_folder.insert({"pus","../resources/PusherMaps"});
    // if (model_to_folder.find(model) == model_to_folder.end())
    //     throw std::runtime_error("Model not found");


        
    std::map<std::string,std::vector<std::string>> model_to_files;
    std::map<std::string,std::map<std::string,std::string>> file_to_stem;
    for (const std::string& model: model_list) {
        if (model_to_folder.contains(model)) {
            for (const auto& entry : std::filesystem::directory_iterator(model_to_folder[model])) {
                if (entry.is_regular_file()) {
                    model_to_files[model].push_back(entry.path().filename().string());
                    file_to_stem[model][entry.path().filename().string()] = entry.path().stem().string();
                }
            }
            std::sort(model_to_files[model].begin(), model_to_files[model].end());
        }
    }
    
    std::string instance_name;
    ABS::Model* model_instance = nullptr;
    int horizon = 50;
    int m_idx = 0;
    int instance_num = -1;
    bool inc_model = false;

    while (idx >= 0) {
        if (model_instance != nullptr)
            delete model_instance;

        if (inc_model) {
            instance_num = 0;
            m_idx++;
            if (m_idx >= (int)model_list.size())
                return {nullptr,0,"",""};
            inc_model = false;
        }else
            instance_num++;
        
        const auto& model = model_list[m_idx];

        auto stems = file_to_stem[model];
        auto files = model_to_files[model];
        auto instance_path = !files.empty()? model_to_folder[model] + "/" + files[instance_num % files.size()] : "";
        instance_name = !files.empty()? stems[files[instance_num % files.size()]] : "";
        
        if (model == "aa") {
            if (instance_num >= 4 * (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    size_t param_num = instance_num / (int) files.size();
                    bool dense_rewards = param_num % 2 == 0;
                    bool idle_action = param_num / 2 == 0;
                    instance_name = stems[files[instance_num % (int) files.size()]] + "," + std::to_string(dense_rewards) + "," + std::to_string(idle_action);
                    model_instance = new AA::Model(instance_path, dense_rewards, idle_action);
                }
        }
        else if (model == "recon") {
            if (instance_num >= (int) files.size()){inc_model = true; model_instance=nullptr;continue;}
                if(idx == 0) {
                    instance_name = stems[files[instance_num % (int) files.size()]];
                    model_instance = new RECON::ReconModel(instance_path);
                }
        }
        else if (model == "gol") {
            if (instance_num >= 3 * (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    size_t param_num = instance_num / (int) files.size();
                    GOL::ActionMode action_mode = GOL::ActionMode::SAVE_ONLY;
                    if (param_num == 1)
                        action_mode = GOL::ActionMode::ALL;
                    else if (param_num == 2)
                        action_mode = GOL::ActionMode::REVIVE_ONLY;
                    instance_name = stems[files[instance_num % (int) files.size()]] + "," + (action_mode == GOL::ActionMode::ALL ? "all" : action_mode == GOL::ActionMode::SAVE_ONLY ? "save_only" : "revive_only");
                    model_instance = new GOL::Model(instance_path, action_mode);
                }
        }
        else if (model == "eo") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new EO::Model(instance_path);
                }
        }
        else if (model == "man") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new MAN::Model(instance_path);
                }
        }
        else if (model == "navigation") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new Navigation::Model(instance_path,false);
                }
        }
        else if (model == "rt") {
            if (instance_num >= 6 * (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    size_t param_num = instance_num / (int) files.size();
                    bool reset_at_crash = param_num % 2 == 1;
                    double fail_prob = 0.0 + 0.1 * (param_num / 2);

                    instance_name = stems[files[instance_num % (int) files.size()]] + "," + std::to_string(reset_at_crash) + "," + to_string_with_precision(fail_prob,1);
                    model_instance = new RT::Model(instance_path, fail_prob, reset_at_crash, false);
                }
        }
        else if (model == "sw") {
            if (instance_num >= 3)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    size_t param = instance_num;
                    size_t size = 10 + 5 * param;
                    instance_name = std::to_string(size);
                    model_instance = new SW::Model(size,size, false);
                }
        }
        else if (model == "st") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new ST::SkillsTeachingModel(instance_path,false, true);
                }
        }
        else if (model == "sa") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new SA::Model(instance_path);
                }
        }
        else if (model == "tam") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new TAM::Model(instance_path);
                }
        }
        else if (model == "tr") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new TR::TrafficModel(instance_path);
                }
        }
        else if (model == "trt") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new TRT::Model(instance_path,false,true, true);
                }
        }
        else if (model == "wf") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new WF::Model(instance_path);
                }
        }
        else if (model == "rfbe") {
            if (instance_num >= 2*(int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    bool det_spread = instance_num / (int) files.size() == 1;
                    instance_name = stems[files[instance_num % (int) files.size()]] + "," + std::to_string(det_spread);
                    model_instance = new RFBE::Model(instance_path,det_spread);
                }
        }
        else if (model == "ct") {
            if (instance_num >= 9)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    double spawn_rate = 0.4 + 0.2 * (instance_num % 3);
                    size_t size = 4 + 2 * (instance_num / 3);
                    instance_name = std::to_string(size) + "," + to_string_with_precision(spawn_rate,1);
                    model_instance = new CT::Model(size,size,spawn_rate, true);
                }
        }
        else if (model == "ele") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new ELE::Model(instance_path);
                }
        }
        else if (model == "wlp") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new WLP::Model(instance_path);
                }
        }
        else if (model == "pushyl") {
            if (instance_num >= (int) files.size())
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new PushYL::Model(instance_path);
                }

        }
        else if (model == "ts") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    model_instance = new TS::Model();
                }
        }
        else if (model == "saving") {
            if (instance_num >= 9)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    int price = 4 + (instance_num % 3 - 1);
                    int time = 4 + (instance_num / 3 - 1);
                    instance_name = std::to_string(-price) + "," + std::to_string(price) + "," + std::to_string(time) + "," + std::to_string(time);
                    model_instance = new SAVING::Model(-price,price,time,time);
                }
        }
        else if (model == "j5") {
            if (instance_num >= 2)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    bool joint = instance_num % 2 == 0;
                    instance_name = joint? "joint" : "single";
                    horizon = 200;
                    model_instance = new J5::Model(joint,true,false);
                }
        }
        else if (model == "che") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new CHE::Model(true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        }else if (model == "pyl") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new PYL::Model(true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "ctf") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new CTF::Model(true,model_to_folder["ctf"] + std::string("/standard.txt")),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "c4") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new C4::Model(true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "con") {
            if (instance_num >= 3)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    int size = 10 + 5*(instance_num);
                    instance_name = std::to_string(size);
                    model_instance = new  MPTOMDP::Model( new CON::Model(size,true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "ktk") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new KTK::Model(true,model_to_folder["ktk"] + std::string("/standard.txt")),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "num") {
            if (instance_num >= 4)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    int max_action = instance_num  % 2 == 0? 10 : 5;
                    int goal = instance_num  / 2 == 0? 100 : 20;
                    instance_name =std::to_string(goal) + "," + std::to_string(max_action);
                    model_instance = new  MPTOMDP::Model( new NUM::Model(goal,max_action,true,false),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "oth") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new OTH::Model(true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "pus") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new PUS::Model(true, model_to_folder["pus"] + std::string("/standard.txt")),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "qua") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new QUA::Model(true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        } else if (model == "ttt") {
            if (instance_num >= 1)
                {inc_model = true; model_instance=nullptr;continue;} else if(idx == 0) {
                    horizon = 200;
                    instance_name = "standard";
                    model_instance = new  MPTOMDP::Model( new TTT::Model(true),{{1, new RandomAgent()}},1.0,0, false, true);
                }
        }

        idx--;
    }
    
    instance_name = "(" + instance_name + ")";
    size_t pos = 0;
    while ((pos = instance_name.find("_", pos)) != std::string::npos) {
        instance_name.replace(pos, 1, "\\_");
        pos += 2;
    }

    return {model_instance, horizon, model_list[m_idx], instance_name};
}
    
bool randomAnalysis(size_t idx, int q_time_limit) {

    std::string out_path = q_time_limit <= 0? "../nobackup/random_analysis/" : "../nobackup/Q_analysis/";
    if (!std::filesystem::exists(out_path)) {
        throw std::runtime_error("Error: Path does not exist -> " + out_path);
    }
    auto [model_instance, horizon, model, instance_name] = getModel(idx);

    if (model_instance == nullptr)
        return false;
    std::cout << "Model:" << model << " Instance:" << instance_name << std::endl;
    MEMORY::PrintUsedMemory();
    auto res = randomAnalysis(model_instance,instance_name, horizon, true, q_time_limit, dynamic_cast<MPTOMDP::Model*>(model_instance) == nullptr);

    //write to file
    std::ofstream file;
    file.open(out_path + model + "_stats.txt", std::ios::app);
    file << res;
    file.close();

    return true;
}

std::string randomAnalysis(ABS::Model* ground_model, const std::string& instance_name, int horizon, bool verbose, int time_limit_qtable, bool outcome_sampling){
    //Gather abstraction statistics
    std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare> Q_map = {};
    if (ground_model->hasTransitionProbs() && time_limit_qtable > 0) {
        if (verbose)
            std::cout << "Creating Q-table" << std::endl;
        createQTable(ground_model, 50, &Q_map,"", verbose, time_limit_qtable);
        if (verbose)
            std::cout << "Created Q-table with " << Q_map.size() << " entries" << std::endl;
    }

    //Gather state statistics by random play
    std::mt19937 rng(static_cast<unsigned int>(42));
    auto random = new RandomAgent();
    auto* model = new FINITEH::Model(ground_model,horizon, true);

    int num_games = 100000;
    Set<ABS::Gamestate> unique_states = {};
    double return_sum = 0;
    double total_avail_actions = 0;
    int num_decisions = 0;
    std::vector<std::pair<FINITEH::Gamestate*,int>> state_action_pair_random_probe = {};

    if (verbose)
        std::cout << "Playing random games ..." << std::endl;
    for(int i = 0; i < num_games; i++){
        if (verbose && i % (num_games / 10) == 0) {
            std::cout << "Game:" << i << "/" << num_games <<" " << unique_states.size() << " " << total_avail_actions << " " << num_decisions << " " << (total_avail_actions / (double)num_decisions)  <<std::endl;
        }

        std::vector<std::pair<FINITEH::Gamestate*,int>> episode_saps = {};

        auto state = dynamic_cast<FINITEH::Gamestate*>(model->getInitialState(rng));
        int step = 0;
        while(!state->terminal){
            total_avail_actions += model->getActions(state).size();
            num_decisions++;
            auto action = random->getAction(model,state,rng);
            episode_saps.emplace_back(dynamic_cast<FINITEH::Gamestate*>(model->copyState(state)),action);
            auto [reward, prob] = model->applyAction(state,action,rng, nullptr);

            auto cpy = dynamic_cast<FINITEH::Gamestate *>(model->copyState(state))->ground_state;
            if(!unique_states.contains(cpy))
                unique_states.insert(cpy);
            else
                delete cpy;
            step++;
            return_sum += reward[0];
        }
        delete state;

        //sample random sap from episode
        auto dist = std::uniform_int_distribution<unsigned>(0,episode_saps.size()-1);
        auto [state_sample, action] = episode_saps[dist(rng)];
        state_action_pair_random_probe.emplace_back(dynamic_cast<FINITEH::Gamestate*>(model->copyState(state_sample)),action);
        for(auto [state_tmp, action] : episode_saps)
            delete state_tmp;
    }


    //Calculate v_rndm and v_star
    double v_rndm = return_sum / num_games;
    auto tmp_init =  dynamic_cast<FINITEH::Gamestate *>(model->getInitialState(rng));
    double v_star = Q_map.empty()? std::nan("") : std::numeric_limits<double>::lowest();
    for (int a : model->getActions(tmp_init)){
        if(!Q_map.empty() && !Q_map.contains({tmp_init,a}) && ground_model->hasTransitionProbs())
            throw std::runtime_error("Q_map does not contain all actions for initial state");
        if (!Q_map.empty())
            v_star = std::max(v_star,Q_map.at({tmp_init,a}));
    }
    delete tmp_init;

    //Calculate ratio of value-equivalent actions/states
    double TOL = 1e-5;
    const int l = 3;
    auto num_value_equivalent_states = std::vector<int>(l,0);
    auto total_state_pairs = std::vector<int>(l,0);

    auto num_value_equivalent_Qs = std::vector<int>(l,0);
    auto total_Q_pairs = std::vector<int>(l,0);


    int num_samples = 100000;
    if (verbose)
        std::cout << "Sampling state-action-pairs" << std::endl;

    if (!Q_map.empty()){
        for(int i = 0; i < num_samples; i++){
            if (verbose && i % (num_samples / 10) == 0)
                std::cout << "Sample:" << i << "/" << num_samples << std::endl;
            //sample random state-action pair from state_action_pairs
            auto dist = std::uniform_int_distribution<unsigned>(0,state_action_pair_random_probe.size()-1);
            auto [root, _] = state_action_pair_random_probe[dist(rng)];

            auto rollout_state1 = dynamic_cast<FINITEH::Gamestate *>(model->copyState(root));
            auto rollout_state2 = dynamic_cast<FINITEH::Gamestate *>(model->copyState(root));
            for (int j = 0; j < static_cast<int>(num_value_equivalent_states.size()); j++) {
                int a1 = random->getAction(model, rollout_state1, rng);
                int a2 = random->getAction(model, rollout_state2, rng);

                if (a1 != a2) {
                    total_Q_pairs[j]++;
                    if(std::fabs(Q_map.at({rollout_state1,a1}) - Q_map.at({rollout_state2,a2})) < TOL)
                        num_value_equivalent_Qs[j]++;
                }

                model->applyAction(rollout_state1, a1, rng, nullptr);
                model->applyAction(rollout_state2, a2, rng, nullptr);

                if (rollout_state1->terminal || rollout_state2->terminal)
                    break;

                if (*rollout_state1 != *rollout_state2){
                    total_state_pairs[j]++;
                    double V1 = std::numeric_limits<double>::lowest();
                    double V2 = std::numeric_limits<double>::lowest();
                    for (int a : model->getActions(rollout_state1))
                        V1 = std::max(V1,Q_map.at({rollout_state1,a}));
                    for (int a : model->getActions(rollout_state2))
                        V2 = std::max(V2,Q_map.at({rollout_state2,a}));
                    if(std::fabs(V1 - V2) < TOL){
                        num_value_equivalent_states[j]++;
                    }
                }
            }
            delete rollout_state1;
            delete rollout_state2;
        }
    }

    unsigned states = unique_states.size();
    std::vector<double> ratio_value_equivalent_states;
    std::vector<double> ratio_value_equivalent_Qs;
    for (int i = 0; i < static_cast<int>(num_value_equivalent_states.size()); i++){
        if (Q_map.empty()) {
            ratio_value_equivalent_states.push_back(std::nan(""));
            ratio_value_equivalent_Qs.push_back(std::nan(""));
        }
        else {
            ratio_value_equivalent_states.push_back(num_value_equivalent_states[i] / (double)total_state_pairs[i]);
            ratio_value_equivalent_Qs.push_back(num_value_equivalent_Qs[i] / (double)total_Q_pairs[i]);
        }
    }

    //Average actions/outcomes
    double avg_avail_actions = total_avail_actions / (double)num_decisions;

    double sum_unique_outcomes = 0;
    bool too_many_to_sample = false;
    if (verbose)
        std::cout << "Sampling unique outcomes" << std::endl;
    if (time_limit_qtable <= 0 && outcome_sampling) {
        for(int i = 0; i < static_cast<int>(state_action_pair_random_probe.size()); i++){
            if (verbose && i % (state_action_pair_random_probe.size() / 10) == 0)
                std::cout << "Sample:" << i << "/" << state_action_pair_random_probe.size() << std::endl;
            auto [state, action] = state_action_pair_random_probe[i];
            auto [unique_outcomes,p] = model->getOutcomes(state,action, 600 * 1000 / state_action_pair_random_probe.size());
            if (std::fabs(p - 1 ) > 1e-6)
                too_many_to_sample = true;
            else
                sum_unique_outcomes += unique_outcomes.size();

            for(auto state_succ : unique_outcomes)
                delete state_succ.first;

            if (too_many_to_sample)
                break;
        }
    }
    if (verbose)
        std::cout << "Done!" << std::endl;
    double avg_unique_outcomes = too_many_to_sample? std::nan("") : (sum_unique_outcomes / (double)state_action_pair_random_probe.size());


    std::ostringstream output;

    output << std::fixed << std::setprecision(2);
    if (time_limit_qtable <= 0) {
        output  << instance_name << " & nan & " << v_rndm << std::setprecision(2) << " & "
               << avg_avail_actions << " & ";
        if (outcome_sampling)
            output << avg_unique_outcomes << " & ";
        output << (num_decisions / (double)num_games) << " & " << states << " ";
    }else {
        output << instance_name << " & " << v_star << " & ";
        for (int i = 0; i < static_cast<int>(ratio_value_equivalent_states.size()); i++)
            output << ratio_value_equivalent_states[i] << " & " << ratio_value_equivalent_Qs[i] << " & ";
    }
    output << " \\\\" << std::endl;

    if (verbose)
        std::cout << output.str() << std::endl;

    for(auto state : unique_states)
        delete state;
    for(auto [state, action] : state_action_pair_random_probe)
        delete state;
    delete random;
    delete model;

    return output.str();
}

void createQTable(ABS::Model* ground_model, int horizon, std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map_ptr, std::string save_path, bool verbose, int time_limit){
    FINITEH::Model* model = new FINITEH::Model(ground_model,horizon, true);

    std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare> Q_map = {};
    std::mt19937 rng(static_cast<unsigned int>(42));
    int num_start_samples = 50000;
    VALUE_IT::runValueIteration(model,num_start_samples,1.0,Q_map,rng,verbose, time_limit);
    if (!save_path.empty())
        VALUE_IT::saveQTable(Q_map,save_path, true);
    if (Q_map_ptr != nullptr)
        *Q_map_ptr = Q_map;

    // double val = 0;
    // int num_samples = 1;
    // int done = 0;
    // while(done < num_samples) {
    //     if(done % 1000 == 0)
    //         std::cout << "Done:" << done << std::endl;
    //     auto init_state = dynamic_cast<FINITEH::Gamestate *>(model->getInitialState(rng));
    //     double max_aval = std::numeric_limits<double>::lowest();
    //     bool broke = false;
    //     for(int action : model->getActions(init_state)) {
    //         if(!Q_map.contains({init_state,action})){
    //             broke = true;
    //              break;
    //         }
    //         max_aval = std::max(max_aval,Q_map.at({init_state,action}));
    //     }
    //     if(!broke) {
    //         done++;
    //         val += max_aval;
    //     }
    // }
    // std::cout << "Value:" << val/num_samples << std::endl;
    // auto init_state = dynamic_cast<FINITEH::Gamestate *>(model->getInitialState(rng));
    // for(int action : model->getActions(init_state)) {
    //     std::cout << "Action:" << Q_map.at({init_state,action}) << std::endl;
    // }
}


std::vector<ABS::Gamestate*> gatherSmallQDiffStates(ABS::Model& uncasted_model, unsigned int num_states,
            std::unordered_map<std::pair<FINITEH::Gamestate*,int>, double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare>* Q_map,
            Agent* agent, std::mt19937& rng) {
    assert (dynamic_cast<FINITEH::Model*>(&uncasted_model));
    auto model = dynamic_cast<FINITEH::Model*>(&uncasted_model);

    auto cmp = [](std::pair<ABS::Gamestate*,double> a, std::pair<ABS::Gamestate*,double> b) {
        return a.second < b.second;
    };
    std::set<std::pair<ABS::Gamestate*,double>,decltype(cmp)> states;

    if(num_states >= Q_map->size()) {
        for(auto& [key, value] : *Q_map) {
            if (key.first->remaining_steps == model->getHorizonLength())
                states.insert({model->unwrapState(key.first), value});
        }
    }else {

        double TOL = 1e-4;

        //filter for states with planning horizon steps
        auto reduced_map = std::vector<std::pair<std::pair<FINITEH::Gamestate*,int>, double>>();
        for(auto& [key, value] : *Q_map)
            if(key.first->remaining_steps == model->getHorizonLength())
                reduced_map.emplace_back(key,value);

        double top_ratio = 0.1;
        int num_samples = std::ceil(num_states / top_ratio);

        for(int i = 0; i < num_samples; i++){
            //sample from reduced map using rng
            assert (!reduced_map.empty());
            auto dist = std::uniform_int_distribution<int>(0,reduced_map.size()-1);
            auto& [key, value] = reduced_map[dist(rng)];

            //get value diff between optimal and second best action
            double best_val = std::numeric_limits<double>::lowest();
            double second_best_val = std::numeric_limits<double>::lowest();
            for(int action : model->getActions(key.first)) {
                double aval = Q_map->at(std::make_pair(key.first, action));
                if(aval > best_val + TOL) {
                    second_best_val = best_val;
                    best_val = aval;
                } else if(std::fabs(aval - best_val) <= TOL) {
                    //pass
                }
                else if(aval > second_best_val + TOL)
                    second_best_val = aval;
            }
            double diff = best_val - second_best_val;

            //insert into sorted list
            states.insert({key.first,diff});
            if(states.size() > num_states)
                states.erase(--states.end());

        }
    }

    auto states_vec = std::vector<ABS::Gamestate*>();
    for(auto& [state, diff] : states){
        std::cout << "State: " << state << " Diff: " << diff << std::endl;
        states_vec.push_back(state);
    }

    return states_vec;
}

void estimateConfIntervalsAupo() {
    auto model = new  FINITEH::Model(new SA::Model("../resources/SysAdminTopologies/4_Anand.txt"), 50, true );
    auto rng = std::mt19937(static_cast<unsigned int>(42));
    auto state = dynamic_cast<FINITEH::Gamestate*>(model->getInitialState(rng));
    auto ground = dynamic_cast<SA::Gamestate*>(state->ground_state);
    ground->machine_statuses = 1015; //all running except machine with index 3

    for (int its: {200,500,1000,2000}){
    //for (int its : {1000,2000,3000,4000}) {
        auto aupo = AUPO::AupoAgent(AUPO::AupoArgs({its, "iterations"}, {-1,2} , 1.0, -1, 4, true,0.8,  -1, true,  false, 0, false, false, 0.4));
        for (int i = 0; i < 100; i++) {
            aupo.getAction(model,state,rng);
        }
        std::cout << std::endl;
    }

}

}
