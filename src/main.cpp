#define DEBUG

#include <random>
#include "../include/Arena.h"
#include "../include/Utils/AgentMaker.h"
#include "../include/Utils/ModelMaker.h"
#include "../include/Agents/Oga/OgaAgent.h"
#include "../include/Agents/Mcts/MctsAgent.h"
#include "../include/Agents/RandomAgent.h"
#include "../include/Games/MDPs/Traffic.h"
#include "../include/Agents/HumanAgent.h"
#include "../include/Agents/OneStepLookahead.h"
#include "../include/Agents/SparseSamplingAgent.h"
#include "../include/Agents/Mcts/MctsAgent.h"
#include "../include/Games/TwoPlayerGames/TicTacToe.h"
#include "../include/Games/MDPs/SailingWind.h"
#include "../include/Games/TwoPlayerGames/Chess.h"
#include "../include/Games/TwoPlayerGames/Constrictor.h"
#include "../include/Games/TwoPlayerGames/Pylos.h"
#include "../include/Games/TwoPlayerGames/Quarto.h"
#include "../include/Games/MDPs/Navigation.h"
#include "../include/Games/MDPs/SkillsTeaching.h"
#include "../include/Games/MDPs/PushYourLuck.h"
#include "../include/Games/MDPs/SysAdmin.h"
#include "../include/Games/MDPs/TriangleTireworld.h"
#include "../include/Games/Wrapper/MultiPlayerToMDP.h"
#include "../include/Games/MDPs/RedFinnedBlueEye.h"
#include "../include/Games/MDPs/EarthObservation.h"
#include "../include/Games/MDPs/WildlifePreserve.h"
#include "../include/Games/MDPs/Manufacturer.h"
#include "../include/Games/MDPs/GameOfLife.h"
#include "../include/Games/MDPs/Wildfire.h"
#include "../include/Games/MDPs/JoinFive.h"
#include "../include/Games/MDPs/Tamarisk.h"
#include "../include/Games/TwoPlayerGames/CaptureTheFlag.h"
#include "../include/Games/Wrapper/FiniteHorizon.h"
#include "../include/Games/MDPs/Saving.h"
#include "../include/Games/MDPs/GraphTraversal.h"
#include "../include/Games/MDPs/Elevators.h"
#include "../include/Games/MDPs/RaceTrack.h"
#include "../include/Games/MDPs/CrossingTraffic.h"
#include "../include/Games/TwoPlayerGames/KillTheKing.h"
#include "../include/Games/MDPs/AcademicAdvising.h"
#include "../include/Games/TwoPlayerGames/Constrictor.h"
#include "../include/Games/MDPs/MultiArmedBandit.h"
#include "../include/Games/MDPs/CooperativeRecon.h"
#include "../include/Games/TwoPlayerGames/NumbersRace.h"
#include "../include/Games/MDPs/ToySoccer.h"
#include "../include/Utils/Argparse.h"
#include "../include/Games/TwoPlayerGames/Connect4.h"
#include "../include/Games/TwoPlayerGames/Othello.h"
#include "../include/Games/Wrapper/RandomStart.h"
#include "../include/Games/Wrapper/Determinization.h"
#include "../include/Games/Wrapper/HeuristicsAsReward.h"
#include "../include/Utils/Distributions.h"
#include "../include/Utils/ValueIteration.h"

#include <cstdlib>

void debug(){

}

int main(const int argc, char **argv) {

    argparse::ArgumentParser program("Executable");

    program.add_argument("-s", "--seed")
        .help("Seed for the random number generator")
        .action([](const std::string &value) { return std::stoi(value); })
        .required();

    program.add_argument("-c", "--required_conf_range")
        .help("Optional specification of the confidence range. Only if the performance confidence interval is less than this value, the evaluation stops.")
        .action([](const std::string &value) { return std::stod(value); })
        .default_value(std::numeric_limits<double>::max());

    program.add_argument("-a", "--agent")
        .help("Agent to benchmark")
        .required();

    program.add_argument("--aargs")
        .help("Extra arguments for agent")
        .default_value(std::vector<std::string>{})
        .append();

    program.add_argument("-m", "--model")
        .help("Model to benchmark")
        .required();

    program.add_argument("--margs")
        .help("Extra arguments for model")
        .default_value(std::vector<std::string>{})
        .append();

    program.add_argument("-n", "--n_games")
        .help("Number of games to play")
        .action([](const std::string &value) { return std::stoi(value); })
        .required();

    program.add_argument("-v", "--csv")
        .help("CSV mode")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-omit_times", "--omit_times")
        .help("Whether to omit times in the output")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("-p_horizon", "--p_horizon")
        .help("Planning horizon")
        .action([](const std::string &value) { return std::stoi(value); })
        .default_value(50);

    program.add_argument("-rng_save_path","--rng_save_path")
    .help("Path for saving the latest rng state")
    .action([](const std::string &value) { return value; })
    .default_value("");

    program.add_argument("-rng_load_path","--rng_load_path")
    .help("If specified the rng state will be loaded from this file. If not specified, the rng will be seeded with the seed argument.")
    .action([](const std::string &value) { return value; })
    .default_value("");

    program.add_argument("-e_horizon", "--e_horizon")
    .help("Execution horizon")
    .action([](const std::string &value) { return std::stoi(value); })
    .default_value(50);

    program.add_argument("--qtable")
    .help("If available, the path to the qtable to load.")
    .default_value("");

    program.add_argument("--planning_beyond_execution_horizon")
    .help("Whether the agent should plan beyond the execution horizon, i.e. always plan for the full planning horizon.")
    .default_value(false)
    .implicit_value(true);

    program.add_argument("--deterministic_init")
    .help("Whether to cycle through the same deterministic init states or sample random ones.")
    .default_value(false)
    .implicit_value(true);

    program.add_argument("-episode_num_offset", "--episode_num_offset")
            .help("The episode number of the output is shifted by this offset.")
            .action([](const std::string &value) { return std::stoi(value); })
            .default_value(0);

    if (argc == 1) {
        std::cout << "Since no arguments were provided, for IDE convenience, the debug function will be called." << std::endl;
        debug();
        return 0;
    }

    program.parse_args(argc, argv);

    /*
     * Setup RNG state
     */

    //Default is seeded
    const auto seed = program.get<int>("--seed");
    std::mt19937 rng(seed);

    //If --rng_load_path is specified, load the RNG state from the file
    const auto rng_load_path = program.get<std::string>("--rng_load_path");
    if (!rng_load_path.empty()) {

        std::ifstream in(rng_load_path);
        if (!in) {
            std::cerr << "Failed to open file for reading the RNG state.\n";
            throw std::runtime_error("Failed to open file for reading the RNG state.");
            return 1;
        }
        in >> rng;
        in.close();
    }

    auto* model = getModel(program.get<std::string>("--model"), program.get<std::vector<std::string>>("--margs"));
    if (model == nullptr ) {
        throw std::runtime_error("Invalid model");
        return 1;
    }

    Agent* agent = getAgent(program.get<std::string>("--agent"), program.get<std::vector<std::string>>("--aargs"));
    if (agent == nullptr) {
        delete model;
        return 1;
    }
    auto agent_list = std::vector<Agent*>{agent};
    while ((int)agent_list.size() < model->getNumPlayers())
        agent_list.push_back(getDefaultAgent(true));

    auto horizons = std::make_pair(program.get<int>("--e_horizon"), program.get<int>("--p_horizon"));
    bool planning_beyond_execution_horizon = program.get<bool>("--planning_beyond_execution_horizon");
    bool random_init_state = !program.get<bool>("--deterministic_init");
    std::unordered_map<std::pair<FINITEH::Gamestate*,int> , double, VALUE_IT::QMapHash, VALUE_IT::QMapCompare> Q_map = {};
    if (!program.get<std::string>("--qtable").empty()) {
        auto tmp_model = FINITEH::Model(model,1 << 16, false);
        VALUE_IT::loadQTable(&tmp_model, &Q_map, program.get<std::string>("--qtable"));
    }

    const auto conf_range = program.get<double>("--required_conf_range");
    const auto rng_save_path = program.get<std::string>("--rng_save_path");
    playGames(model, program.get<int>("--n_games"),
        agent_list, rng, program.get<bool>("--csv") ? ( program.get<bool>("--omit_times")? CSV_OMIT_TIMES : CSV) : VERBOSE, horizons,
        planning_beyond_execution_horizon, random_init_state,conf_range, &Q_map, rng_save_path, program.get<int>("--episode_num_offset"));

    //Cleanup
    delete model;
    for (auto agent_ : agent_list)
        delete agent_;
    for (auto& [key, val] : Q_map)
        delete key.first;

    return 0;
}
