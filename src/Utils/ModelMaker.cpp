#include "../../include/Utils/ModelMaker.h"
#include "../../include/Agents/Mcts/MctsAgent.h"
#include "../../include/Agents/RandomAgent.h"
#include "../../include/Agents/OneStepLookahead.h"
#include "../../include/Agents/Agent.h"

#include "../../include/Utils/AgentMaker.h"

 #include "../../include/Games/TwoPlayerGames/TicTacToe.h"
#include "../../include/Games/TwoPlayerGames/Pusher.h"
#include "../../include/Games/MDPs/SailingWind.h"
#include "../../include/Games/TwoPlayerGames/Chess.h"
#include "../../include/Games/TwoPlayerGames/Constrictor.h"
#include "../../include/Games/TwoPlayerGames/Pylos.h"
#include "../../include/Games/TwoPlayerGames/Quarto.h"
#include "../../include/Games/MDPs/Navigation.h"
#include "../../include/Games/MDPs/SkillsTeaching.h"
#include "../../include/Games/MDPs/PushYourLuck.h"
#include "../../include/Games/MDPs/SysAdmin.h"
#include "../../include/Games/MDPs/TriangleTireworld.h"
#include "../../include/Games/Wrapper/MultiPlayerToMDP.h"
#include "../../include/Games/MDPs/RedFinnedBlueEye.h"
#include "../../include/Games/MDPs/EarthObservation.h"
#include "../../include/Games/MDPs/WildlifePreserve.h"
#include "../../include/Games/MDPs/Manufacturer.h"
#include "../../include/Games/MDPs/GameOfLife.h"
#include "../../include/Games/MDPs/Traffic.h"
#include "../../include/Games/MDPs/Wildfire.h"
#include "../../include/Games/MDPs/JoinFive.h"
#include "../../include/Games/MDPs/Tamarisk.h"
#include "../../include/Games/TwoPlayerGames/CaptureTheFlag.h"
#include "../../include/Games/MDPs/Saving.h"
#include "../../include/Games/MDPs/GraphTraversal.h"
#include "../../include/Games/MDPs/Elevators.h"
#include "../../include/Games/MDPs/RaceTrack.h"
#include "../../include/Games/MDPs/CrossingTraffic.h"
#include "../../include/Games/TwoPlayerGames/KillTheKing.h"
#include "../../include/Games/MDPs/AcademicAdvising.h"
#include "../../include/Games/MDPs/MultiArmedBandit.h"
#include "../../include/Games/MDPs/CooperativeRecon.h"
#include "../../include/Games/TwoPlayerGames/NumbersRace.h"
#include "../../include/Games/MDPs/ToySoccer.h"
#include "../../include/Games/TwoPlayerGames/Connect4.h"
#include "../../include/Games/TwoPlayerGames/Othello.h"
#include "../../include/Games/MDPs/BinPacking.h"
#include "../../include/Games/MDPs/Knapsack.h"
#include "../../include/Games/MDPs/TravellingSalesPerson.h"
#include "../../include/Games/MDPs/SupplyChain.h"

#include <map>
#include <set>
#include <bits/ranges_algo.h>
#include <filesystem>

#include "../../include/Games/Wrapper/Determinization.h"
#include "../../include/Games/Wrapper/HeuristicsAsReward.h"
#include "../../include/Games/Wrapper/RandomStart.h"
namespace fs = std::filesystem;

void checkArguments(std::string model, std::map<std::string, std::string> model_args) {
    std::map<std::string,std::pair<std::set<std::string>,std::set<std::string>>> model_arg_map;

    //wrapper
    model_arg_map["mptomdp"] = {{"model_type", "model_args", "player"}, {"model_type", "model_args", "agents", "player", "discount", "deterministic_opponents"}};
    model_arg_map["determinization"] = {{"model_type","model_args"}, {"model_type", "model_args"}};
    model_arg_map["heuristicsasreward"] = {{"model_type","model_args"}, {"model_type", "model_args"}};
    model_arg_map["randomstart"] = {{"model_type","model_args", "steps"}, {"model_type", "model_args", "steps"}};

    //standard envs
    model_arg_map["num"] = {{"goal", "max_action", "zero_sum"}, {"zero_sum", "goal", "max_action", "one_hot_obs"}};
    model_arg_map["saving"] = { {"p", "t"}, {"p", "t"}};
    model_arg_map["ts"] = { {}, {}};
    model_arg_map["che"] = {{"zero_sum"}, {"zero_sum"}};
    model_arg_map["con"] = {{"zero_sum", "arena_size"}, {"zero_sum", "arena_size"}};
    model_arg_map["pyl"] = {{"zero_sum"}, {"zero_sum"}};
    model_arg_map["qua"] = {{"zero_sum"}, {"zero_sum"}};
    model_arg_map["ct"] = {{"width", "height", "spawn_rate"}, {"width", "height", "spawn_rate", "idle_action"}};
    model_arg_map["mab"] = {{"repeats", "means", "stds"}, {"repeats", "means", "stds"}};
    model_arg_map["pushyl"] = {{"map"}, {"map"}};
    model_arg_map["rfbe"] = {{"map"}, {"map", "deterministic_spread"}};
    model_arg_map["aa" ] = {{"map", "dense_rewards"}, {"map", "dense_rewards", "idle_action"}};
    model_arg_map["sa"] = {{"map"}, {"map"}};
    model_arg_map["eo"] = {{"map"}, {"map"}};
    model_arg_map["sw"] = {{"size"}, {"size", "deterministic"}};
    model_arg_map["rt"] = {{"map"}, {"map", "reset_at_crash", "fail_prob", "simplified_observation_space"}};
    model_arg_map["gol"] = {{"map"}, {"map", "action_mode"}};
    model_arg_map["j5"] = {{"joint","decoupled_action_space"}, {"joint","decoupled_action_space"}};
    model_arg_map["trt"] = {{"map"}, {"map", "idle_action", "reduced_action_space", "big_payoff"}};
    model_arg_map["wf"] = {{"map"}, {"map"}};
    model_arg_map["wlp"] = {{"map"}, {"map"}};
    model_arg_map["ctf"] = {{"zero_sum","map"}, {"zero_sum","map"}};
    model_arg_map["c4"] = {{"zero_sum"}, {"zero_sum"}};
    model_arg_map["oth"] = {{"zero_sum"}, {"zero_sum"}};
    model_arg_map["pus"] = {{"zero_sum","map"}, {"zero_sum","map"}};
    model_arg_map["ktk"] = {{"zero_sum","map"}, {"zero_sum","map"}};
    model_arg_map["ttt"] = {{"zero_sum"}, {"zero_sum"}};
    model_arg_map["st"] = {{"map"}, {"map", "idle_action", "reduced_action_space"}};
    model_arg_map["tr"] = {{"map"}, {"map"}};
    model_arg_map["man"] = {{"map"}, {"map"}};
    model_arg_map["gtr"] = {{"map"}, {"map"}};
    model_arg_map["ele"] = {{"map"}, {"map"}};
    model_arg_map["tam"] = {{"map"}, {"map"}};
    model_arg_map["recon"] = {{"map"}, {"map"}};
    model_arg_map["navigation"] = {{"map"}, {"map", "idle_action"}};
    model_arg_map["binpacking"] = {{"setup"}, {"setup"}};
    model_arg_map["knapsack"] = {{"setup"}, {"setup"}};
    model_arg_map["tsp"] = {{"map"}, {"map"}};
    model_arg_map["sc"] = {{"setup"}, {"setup"}};

    //check if model exists
    if (!model_arg_map.contains(model)) {
        std::cerr << "The model '" << model << "' is not recognized. Please choose one of the following values:" << std::endl;
        std::cerr << "aa (Academic Advising), recon (Cooperative Recon), ct (Crossing Traffic), eo (Earth Observation), ele (Elevators), gol (Game of Life), gtr (Graph Traversal), "
                     "man (Manufacturer), j5 (Join Five), mab (Multi-armed bandit), navigation (Navigation), "
                     "pushyl (Push Your Luck), rt (Racetrack), rfbe (Red Finned Blue Eye), sw (Sailing Wind), saving (Saving), st (Skill Teaching), sa (SysAdmin), "
                     "tam (Tamarisk), ts (Toy Soccer), tr (Traffic), trt (Triangle Tireworld), wf (Wildfire), "
                     "wlp (Wildlife Preserve), ctf (Capture the Flag), che (Chess), c4 (Connect 4), con (Constrictor), ktk (Kill the King), num (Numbers Race),"
                     "oth (Othello), pus (Pusher), pyl (Pylos), qua (Quarto), ttt (TicTacToe), binpacking (Bin-Packing), knapsack (Knapsack), tsp (Travelling Salesperson), sc (Supply Chain)" << std::endl;
        throw std::runtime_error("");
    }

    //check for missing required arguments
    for (auto& arg : model_arg_map[model].first){
        if (!model_args.contains(arg)){
            std::cerr <<"Model " << model << " is missing required argument: " << arg << std::endl;
            std::cerr<< "Required arguments are: ";
            for (auto& arg : model_arg_map[model].first){
                std::cerr << arg << " ";
            }
            std::cerr << std::endl;
            throw std::runtime_error("");
        }
    }

    //check for invalid arguments
    for (auto& arg : model_args){
        if (!model_arg_map[model].second.contains(arg.first)){
            std::cerr <<"Model " << model << " has invalid argument: " << arg.first <<"." << std::endl;
            std::cerr << "Valid arguments are: ";
            for (auto& arg : model_arg_map[model].second){
                std::cerr << arg << " ";
            }
            std::cerr << std::endl;
            throw std::runtime_error("");
        }
    }
}

std::string resolve_file_path(const std::string& input_path, const std::string& relative_dir) {
    fs::path original_path(input_path);
    if (fs::exists(original_path))
        return original_path.string();

    fs::path filename = original_path.filename();
    fs::path fallback_dir = fs::path(__FILE__).parent_path() / "../../" / relative_dir;
    fs::path fallback_path = fallback_dir / filename;

    if (fs::exists(fallback_path))
        return fallback_path.string();

    std::cerr << "File not found: " << input_path << " or fallback " << fallback_path.string() << std::endl;
    throw std::runtime_error("File not found: " + input_path + " or fallback " + fallback_path.string());
}

std::string decode_alternative_name_formulation(const std::string& model_type) {
    std::map<std::string,std::string> alternatives_map;
    alternatives_map["numbers race"] = "num";
    alternatives_map["saving"] = "saving";
    alternatives_map["tictactoe"] = "ts";
    alternatives_map["tic tac toe"] = "ts";
    alternatives_map["chess"] = "che";
    alternatives_map["constrictor"] = "con";
    alternatives_map["pylos"] = "pyl";
    alternatives_map["quarto"] = "qua";
    alternatives_map["crossing traffic"] = "ct";
    alternatives_map["multi-armed bandit"] = "mab";
    alternatives_map["multi armed bandit"] = "mab";
    alternatives_map["multiarmedbandit"] = "mab";
    alternatives_map["push your luck"] = "pushyl";
    alternatives_map["pushyourluck"] = "pushyl";
    alternatives_map["red finned blue eye"] = "rfbe";
    alternatives_map["redfinnedblueeye"] = "rfbe";
    alternatives_map["academic advising"] = "aa";
    alternatives_map["academicadvising"] = "aa";
    alternatives_map["sysadmin"] = "sa";
    alternatives_map["sys admin"] = "sa";
    alternatives_map["earth observation"] = "eo";
    alternatives_map["earthobservation"] = "eo";
    alternatives_map["sailing wind"] = "sw";
    alternatives_map["sailingwind"] = "sw";
    alternatives_map["racetrack"] = "rt";
    alternatives_map["game of life"] = "gol";
    alternatives_map["gameoflife"] = "gol";
    alternatives_map["join five"] = "j5";
    alternatives_map["joinfive"] = "j5";
    alternatives_map["triangle tireworld"] = "trt";
    alternatives_map["triangletireworld"] = "trt";
    alternatives_map["wildfire"] = "wf";
    alternatives_map["wildlife preserve"] = "wlp";
    alternatives_map["wildlifepreserve"] = "wlp";
    alternatives_map["capture the flag"] = "ctf";
    alternatives_map["capturetheflag"] = "ctf";
    alternatives_map["connect 4"] = "c4";
    alternatives_map["connect4"] = "c4";
    alternatives_map["othello"] = "oth";
    alternatives_map["pusher"] = "pus";
    alternatives_map["kill the king"] = "ktk";
    alternatives_map["killtheking"] = "ktk";
    alternatives_map["tic tac toe"] = "ttt";
    alternatives_map["tictactoe"] = "ttt";
    alternatives_map["skill teaching"] = "st";
    alternatives_map["skillteaching"] = "st";
    alternatives_map["traffic"] = "tr";
    alternatives_map["manufacturer"] = "man";
    alternatives_map["graph traversal"] = "gtr";
    alternatives_map["graphtraversal"] = "gtr";
    alternatives_map["elevators"] = "ele";
    alternatives_map["tamarisk"] = "tam";
    alternatives_map["cooperative recon"] = "recon";
    alternatives_map["cooperativerecon"] = "recon";
    alternatives_map["navigation"] = "navigation";
    alternatives_map["bin packing"] = "binpacking";
    alternatives_map["bp"] = "binpacking";
    alternatives_map["ks"] = "knapsack";
    alternatives_map["travellingsalesperson"] = "tsp";
    alternatives_map["travelling salesperson"] = "tsp";
    alternatives_map["travelling salesman"] = "tsp";
    alternatives_map["travellingsalesman"] = "tsp";
    alternatives_map["supplychain"] = "sc";
    alternatives_map["supply chain"] = "sc";

    //model type to lowercase
    std::string model_type_lower = model_type;
    std::transform(model_type_lower.begin(), model_type_lower.end(), model_type_lower.begin(), ::tolower);

    return alternatives_map.contains(model_type_lower) ? alternatives_map[model_type_lower] : model_type_lower;
}

ABS::Model* getModel(std::string model_type, const std::vector<std::string>& m_args, int horizon){
    model_type = decode_alternative_name_formulation(model_type);

    std::map<std::string, std::string> model_args;
    for(auto &arg : m_args) {
        //split at '='
        auto pos = arg.find('=');
        if (pos == std::string::npos) {
            std::cout << "Invalid model argument: " << arg << ". It must be of the form arg_name=arg_val" << std::endl;
            return nullptr;
        }
        model_args[arg.substr(0, pos)] = arg.substr(pos + 1);
    }
    checkArguments(model_type, model_args);



    ABS::Model *model = nullptr;
    std::set<std::string> acceptable_args;

    if (model_type == "randomstart") {
        std::string model_args_str = model_args["model_args"];
        std::vector<std::string> ground_model_args;
        std::stringstream ss2(model_args_str);
        std::string arg;
        while (std::getline(ss2, arg, ',')) {
            std::ranges::replace(arg, ':', '='); // replace all ':' to '='
            std::ranges::replace(arg, '?', ':'); // replace all '?' to ':'
            std::ranges::replace(arg, ';', ','); // replace all ';' to ','
            ground_model_args.push_back(arg);
        }
        auto ground_model = getModel(model_args["model_type"], ground_model_args);
        model = new RANDOMSTART::Model(ground_model, std::stoi(model_args["steps"]) ,true);
    }
    else if (model_type == "determinization"){
        std::string model_args_str = model_args["model_args"];
        std::vector<std::string> ground_model_args;
        std::stringstream ss2(model_args_str);
        std::string arg;
        while (std::getline(ss2, arg, ',')) {
            std::ranges::replace(arg, ':', '='); // replace all ':' to '='
            std::ranges::replace(arg, '?', ':'); // replace all '?' to ':'
            std::ranges::replace(arg, ';', ','); // replace all ';' to ','
            ground_model_args.push_back(arg);
        }
        auto ground_model = getModel(model_args["model_type"], ground_model_args);
        model = new DETERMINIZATION::Model(ground_model, true);
    }else if (model_type == "heuristicsasreward") {
        std::string model_args_str = model_args["model_args"];
        std::vector<std::string> ground_model_args;
        std::stringstream ss2(model_args_str);
        std::string arg;
        while (std::getline(ss2, arg, ',')) {
            std::ranges::replace(arg, ':', '='); // replace all ':' to '='
            std::ranges::replace(arg, '?', ':'); // replace all '?' to ':'
            std::ranges::replace(arg, ';', ','); // replace all ';' to ','
            ground_model_args.push_back(arg);
        }
        auto ground_model = getModel(model_args["model_type"], ground_model_args);
        model = new HEURISTICSASREWARD::Model(ground_model, true);
    }
    else if (model_type == "mptomdp"){
        //Model parsing
        std::string model_args_str = model_args["model_args"];
        std::vector<std::string> ground_model_args;
        std::stringstream ss2(model_args_str);
        std::string arg;
        while (std::getline(ss2, arg, ',')) {
            //replace ALL : with =
            std::ranges::replace(arg, ':', '='); // replace all ':' to '='
            std::ranges::replace(arg, '?', ':'); // replace all '?' to ':'
            std::ranges::replace(arg, ';', ','); // replace all ';' to ','
            ground_model_args.push_back(arg);
        }
        auto ground_model = getModel(model_args["model_type"], ground_model_args);

        //Agents parsing
        std::map<int,Agent*> agents = {};
        if (model_args.contains("agents"))
        {
            std::string agent_str = model_args["agents"];
            std::stringstream ss(agent_str);
            std::string agent;
            while (std::getline(ss, agent, ',')) {
                auto pos = agent.find(':');
                std::string agent_type = agent.substr(pos+1);
                int agent_player = std::stoi(agent.substr(0,pos));
                Agent* agent_obj;
                if (agent_type == "mcts")
                    agent_obj = new Mcts::MctsAgent({500, "iterations"});
                else if (agent_type == "random")
                    agent_obj = new RandomAgent();
                else if (agent_type == "osla")
                    agent_obj = new OSLA::OneStepLookaheadAgent();
                else
                    throw std::runtime_error("Invalid agent type");
                agents[agent_player] = agent_obj;
            }
        }
        for (int i = 1; i < ground_model->getNumPlayers(); i++){
            if (!agents.contains(i))
                agents[i] = getDefaultAgent(false);
        }

        double discount = model_args.contains("discount") ? std::stod(model_args["discount"]) : 1.0;
        int player = std::stoi(model_args["player"]);
        bool deterministic_opponents = model_args.contains("deterministic_opponents") ? std::stoi(model_args["deterministic_opponents"]) : false;

        model = new MPTOMDP::Model(ground_model, agents, discount, player, deterministic_opponents, true);

    } else if (model_type == "num"){
        int goal = std::stoi(model_args["goal"]);
        int max_action = std::stoi(model_args["max_action"]);
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        bool one_hot_obs = model_args.contains("one_hot_obs") ? std::stoi(model_args["one_hot_obs"]) : false;
        model = new NUM::Model(goal,max_action,zero_sum,one_hot_obs);
    } else if (model_type == "che"){
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model = new CHE::Model(zero_sum);
    } else if (model_type == "con"){
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model = new CON::Model(std::stoi(model_args["arena_size"]), zero_sum);
    }else if (model_type == "pyl"){
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model = new PYL::Model(zero_sum);
    } else if (model_type == "qua"){
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model = new QUA::Model(zero_sum);
    }
    else if (model_type == "ct") {
        bool idle_action = model_args.contains("idle_action") ? std::stoi(model_args["idle_action"]) : true;
        model =  new CT::Model(std::stoi(model_args["width"]), std::stoi(model_args["height"]), std::stof(model_args["spawn_rate"]), idle_action);
    }
    else if (model_type == "mab") {
        int arm_copies = std::stoi(model_args["repeats"]);
        std::vector<std::pair<double,double>> arm_distributions = {};
        std::stringstream ss(model_args["means"]);
        std::stringstream ss2(model_args["stds"]);
        double i;
        while (ss >> i){
            double j;
            ss2 >> j;
            arm_distributions.emplace_back(i, j);
            if (ss.peek() == ';')
                ss.ignore();
            if (ss2.peek() == ';')
                ss2.ignore();
        }
        model =  new MAB::Model(arm_distributions,arm_copies);
    }
    else if (model_type == "pushyl") {
        auto map = resolve_file_path(model_args["map"], "resources/DiceProbs");
        model =  new PushYL::Model(map);
    }else if(model_type == "rfbe"){
        auto map = resolve_file_path(model_args["map"], "resources/RedFinnedBlueEyesMaps");
        bool deterministic_spread = model_args.contains("deterministic_spread") ? std::stoi(model_args["deterministic_spread"]) : false;
        model = new RFBE::Model(map,deterministic_spread);
    }else if(model_type == "man"){
        auto map = resolve_file_path(model_args["map"], "resources/ManufacturerSetups");
        model = new MAN::Model(map);
    }
    else if (model_type == "wf") {
        auto map = resolve_file_path(model_args["map"], "resources/WildfireSetups");
        model =  new WF::Model(map);
    }
    else if (model_type == "gtr") {
        auto map = resolve_file_path(model_args["map"], "resources/GraphTraversalGraphs");
        model =  new GTR::Model(map);
    }
    else if (model_type == "ele") {
        auto map = resolve_file_path(model_args["map"], "resources/ElevatorSetups");
        model =  new ELE::Model(map);
    }
    else if (model_type == "tam") {
        auto map = resolve_file_path(model_args["map"], "resources/TamariskMaps");
        model =  new TAM::Model(map);
    }else if(model_type == "recon") {
        auto map = resolve_file_path(model_args["map"], "resources/CooperativeReconSetups");
        model = new RECON::ReconModel(map);
    }
    else if(model_type == "wlp") {
        auto map = resolve_file_path(model_args["map"], "resources/WildlifeSetups");
        model = new WLP::Model(map);
    }
    else if(model_type == "tr") {
        auto map = resolve_file_path(model_args["map"], "resources/TrafficModels");
        model = new TR::TrafficModel(map);
    }
    else if(model_type == "st") {
        auto map = resolve_file_path(model_args["map"], "resources/SkillsTeachingSkills");
        bool idle_action = model_args.contains("idle_action") ? std::stoi(model_args["idle_action"]) : false;
        bool reduced_action_space = model_args.contains("reduced_action_space") ? std::stoi(model_args["reduced_action_space"]) : true;
        model = new ST::SkillsTeachingModel(map,idle_action, reduced_action_space);
    }else if(model_type == "trt") {
        auto map = resolve_file_path(model_args["map"], "resources/TriangleTireworlds");
        bool idle_action = model_args.contains("idle_action") ? std::stoi(model_args["idle_action"]) : false;
        bool reduced_action_space = model_args.contains("reduced_action_space") ? std::stoi(model_args["reduced_action_space"]) : true;
        bool big_payoff = model_args.contains("big_payoff") ? std::stoi(model_args["big_payoff"]) : true;
        model = new TRT::Model(map,idle_action, reduced_action_space, big_payoff);
    }
    else if (model_type == "ttt") {
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model =  new TTT::Model(zero_sum);
    }else if (model_type == "pus") {
        auto map = resolve_file_path(model_args["map"], "resources/PusherMaps");
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model =  new PUS::Model(zero_sum, map);
    }
    else if (model_type == "c4") {
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model =  new C4::Model(zero_sum);
    }
    else if (model_type == "oth") {
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model =  new OTH::Model(zero_sum);
    }
    else if (model_type == "ktk") {
        auto map = resolve_file_path(model_args["map"], "resources/KTKMaps");
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model =  new KTK::Model(zero_sum,map);
    }
    else if (model_type == "ctf") {
        auto map = resolve_file_path(model_args["map"], "resources/CTFMaps");
        bool zero_sum = std::stoi(model_args["zero_sum"]);
        model =  new CTF::Model(zero_sum, map);
    }
    else if (model_type == "sa") {
        auto map = resolve_file_path(model_args["map"], "resources/SysAdminTopologies");
        model = new SA::Model(map);
    }
    else if (model_type == "eo"){
        auto map = resolve_file_path(model_args["map"], "resources/EarthObservationMaps");
        model = new EO::Model(map);
    }
    else if (model_type == "sw") {
        bool deterministic = model_args.contains("deterministic") ? std::stoi(model_args["deterministic"]) : false;
        model =  new SW::Model(std::stoi(model_args["size"]), std::stoi(model_args["size"]), deterministic);
    }
    else if (model_type == "rt") {
        auto map = resolve_file_path(model_args["map"], "resources/Racetracks");
        bool reset_at_crash = model_args.contains("reset_at_crash") ? std::stoi(model_args["reset_at_crash"]) : false;
        double fail_prob = model_args.contains("fail_prob") ? std::stod(model_args["fail_prob"]) : 0.0;
        bool simplified_observation = model_args.contains("simplified_observation_space") ? std::stoi(model_args["simplified_observation_space"]) : false;
        model =  new RT::Model(map, fail_prob, reset_at_crash, simplified_observation);
    }
    else if (model_type == "gol") {
        auto map = resolve_file_path(model_args["map"], "resources/GameOfLifeMaps");
        GOL::ActionMode action_mode = GOL::ActionMode::SAVE_ONLY;
        if (model_args.contains("action_mode")){
            if (model_args["action_mode"] == "all")
                action_mode = GOL::ActionMode::ALL;
            else if (model_args["action_mode"] == "save_only")
                action_mode = GOL::ActionMode::SAVE_ONLY;
            else if (model_args["action_mode"] == "revive_only")
                action_mode = GOL::ActionMode::REVIVE_ONLY;
            else{
                std::cout << "Invalid action mode" << std::endl;
                throw std::runtime_error("Invalid action mode");
            }
        }
        model =  new GOL::Model(map,action_mode);
    } else if (model_type == "j5") {
        model =  new J5::Model(std::stoi(model_args["joint"]), true, std::stoi(model_args["decoupled_action_space"]));
    }
    else if (model_type == "aa") {
        auto map = resolve_file_path(model_args["map"], "resources/AcademicAdvisingCourses");
        bool idle_action = model_args.contains("idle_action") ? std::stoi(model_args["idle_action"]) : false;
        model =  new AA::Model(map, std::stoi(model_args["dense_rewards"]), idle_action);
    }
    else if (model_type == "navigation") { //
        auto map = resolve_file_path(model_args["map"], "resources/NavigationMaps");
        bool idle_action = model_args.contains("idle_action") ? std::stoi(model_args["idle_action"]) : false;
        model =  new Navigation::Model(map,idle_action);
    }
    else if (model_type == "ts") {
        model =  new TS::Model();
    }else if (model_type == "saving") {
        int price = std::stoi(model_args["p"]);
        int time = std::stoi(model_args["t"]);
        model =  new SAVING::Model(-price,price,time,time);
    }
    else if (model_type == "binpacking") {
        std::string fileName = resolve_file_path(model_args["setup"], "resources/BinPackingSetups");
        model = new BIN_PACKING::Model(fileName);
    }
    else if (model_type == "knapsack") {
        std::string fileName = resolve_file_path(model_args["setup"], "resources/KnapsackSetups");
        model = new KNAPSACK::Model(fileName);
    }
    else if (model_type == "tsp") {
        std::string fileName = resolve_file_path(model_args["map"], "resources/TravellingSalesPersonMaps");
        model = new TRAVELLING_SALES_PERSON::Model(fileName);
    }
    else if (model_type == "sc") {
        std::string filePath = resolve_file_path(model_args["setup"], "resources/SupplyChainSetups");
        model = new SUPPLY_CHAIN::Model(filePath);
    }

    assert (model != nullptr);

    //wrap in finite horizon
    if (horizon > 0){
        auto wrapped_model = new FINITEH::Model(model, (size_t)horizon, true);
        return wrapped_model;
    }else
        return model;

}
