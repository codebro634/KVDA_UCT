cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 20)

project(kvda LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wno-unused-parameter")

set(SOURCE_FILES
        src/main.cpp
        src/Arena.cpp
        src/Agents/Mcts/MctsAgent.cpp
        src/Agents/Mcts/MctsNode.cpp
        src/Agents/RandomAgent.cpp
        include/Games/Gamestate.h
        include/Agents/Agent.h
        include/Agents/Mcts/MctsAgent.h
        include/Agents/Mcts/MctsNode.h
        include/Agents/RandomAgent.h
        include/Arena.h
        include/Agents/HumanAgent.h
        src/Agents/HumanAgent.cpp
        include/Games/TwoPlayerGames/TicTacToe.h
        src/Games/TwoPlayerGames/TicTacToe.cpp
        src/Games/TwoPlayerGames/Connect4.cpp
        include/Games/TwoPlayerGames/Connect4.h
        include/Games/TwoPlayerGames/Othello.h
        src/Games/TwoPlayerGames/Othello.cpp
        include/Games/TwoPlayerGames/Chess.h
        src/Games/TwoPlayerGames/Chess.cpp
        include/Games/TwoPlayerGames/Quarto.h
        src/Games/TwoPlayerGames/Quarto.cpp
        include/Games/TwoPlayerGames/Pylos.h
        src/Games/TwoPlayerGames/Pylos.cpp
        include/Games/TwoPlayerGames/NumbersRace.h
        src/Games/TwoPlayerGames/NumbersRace.cpp
        include/Games/TwoPlayerGames/Constrictor.h
        src/Games/TwoPlayerGames/Constrictor.cpp
        include/Games/TwoPlayerGames/KillTheKing.h
        src/Games/TwoPlayerGames/KillTheKing.cpp
        src/Games/TwoPlayerGames/CaptureTheFlag.cpp
        src/Games/TwoPlayerGames/Pusher.cpp
        include/Games/TwoPlayerGames/Pusher.h
        include/Games/TwoPlayerGames/CaptureTheFlag.h
        include/Games/MDPs/GameOfLife.h
        src/Games/MDPs/GameOfLife.cpp
        src/Games/MDPs/Navigation.cpp
        src/Games/MDPs/Navigation.cpp
        include/Games/MDPs/SailingWind.h
        src/Games/MDPs/SailingWind.cpp
        include/Agents/Oga/OgaAgent.h
        src/Agents/Oga/OgaAgent.cpp
        include/Utils/UnitTest.h
        include/Games/MDPs/SysAdmin.h
        src/Games/MDPs/SysAdmin.cpp
        include/Games/MDPs/RaceTrack.h
        src/Games/MDPs/RaceTrack.cpp
        include/Games/MDPs/AcademicAdvising.h
        src/Games/MDPs/AcademicAdvising.cpp
        include/Games/MDPs/ToySoccer.h
        src/Games/MDPs/ToySoccer.cpp
        include/Utils/Argparse.h
        include/Games/MDPs/Elevators.h
        src/Games/MDPs/Elevators.cpp
        include/Games/MDPs/Tamarisk.h
        src/Games/MDPs/Tamarisk.cpp
        include/Games/MDPs/CrossingTraffic.h
        src/Games/MDPs/CrossingTraffic.cpp
        include/Games/MDPs/Saving.h
        src/Games/MDPs/Saving.cpp
        include/Games/MDPs/FixedActionSpaceEnv.h
        src/Games/MDPs/FixedActionSpaceEnv.cpp
        include/Games/MDPs/GraphTraversal.h
        src/Games/MDPs/GraphTraversal.cpp
        include/Agents/SparseSamplingAgent.h
        src/Agents/SparseSamplingAgent.cpp
        include/Utils/Distributions.h
        src/Utils/Distributions.cpp
        include/Utils/ValueIteration.h
        src/Utils/ValueIteration.cpp
        include/Games/MDPs/GraphTraversal.h
        src/Agents/Oga/TestOga.cpp
        src/Games/MDPs/GraphTraversal.cpp
        include/Games/MDPs/Wildfire.h
        src/Games/MDPs/Wildfire.cpp
        include/Games/MDPs/MultiArmedBandit.h
        src/Games/MDPs/MultiArmedBandit.cpp
        include/Games/MDPs/CooperativeRecon.h
        src/Games/MDPs/CooperativeRecon.cpp
        include/Games/MDPs/Traffic.h
        src/Games/MDPs/Traffic.cpp
        include/Games/MDPs/SkillsTeaching.h
        src/Games/MDPs/SkillsTeaching.cpp
        include/Games/MDPs/TriangleTireworld.h
        src/Games/MDPs/TriangleTireworld.cpp
        include/Games/MDPs/EarthObservation.h
        src/Games/MDPs/EarthObservation.cpp
        include/Games/MDPs/PushYourLuck.h
        src/Games/MDPs/PushYourLuck.cpp
        include/Games/MDPs/RedFinnedBlueEye.h
        src/Games/MDPs/RedFinnedBlueEye.cpp
        include/Games/MDPs/WildlifePreserve.h
        src/Games/MDPs/WildlifePreserve.cpp
        src/Games/MDPs/Manufacturer.cpp
        include/Games/MDPs/Manufacturer.h
        include/Games/Wrapper/MultiPlayerToMDP.h
        src/Games/Wrapper/MultiPlayerToMDP.cpp
        include/Games/Wrapper/FiniteHorizon.h
        src/Games/Wrapper/FiniteHorizon.cpp
        include/Agents/Oga/OgaAbstractNodes.h
        include/Agents/Oga/OgaGroundNodes.h
        src/Agents/Oga/OgaTree.cpp
        include/Agents/Oga/OgaUtils.h
        include/Agents/Oga/OgaTree.h
        src/Agents/Oga/OgaGroundNodes.cpp
        src/Agents/Oga/OgaAbstractNodes.cpp
        src/Agents/Oga/OgaUtils.cpp
        include/Games/MDPs/JoinFive.h
        src/Games/MDPs/JoinFive.cpp
        include/Games/TwoPlayerGames/WinLossGames.h
        src/Games/TwoPlayerGames/WinLossGames.cpp
        include/Utils/MemoryAnalysis.h
        src/Utils/MemoryAnalysis.cpp
        src/demo.cpp
        src/Utils/CLink.cpp
        include/Utils/ModelMaker.h
        src/Utils/ModelMaker.cpp
        include/Utils/AgentMaker.h
        src/Utils/AgentMaker.cpp
        include/Games/Wrapper/RandomStart.h
        src/Games/Wrapper/RandomStart.cpp
        include/Games/Wrapper/Determinization.h
        src/Games/Wrapper/Determinization.cpp
        include/Games/Wrapper/HeuristicsAsReward.h
        src/Games/Wrapper/HeuristicsAsReward.cpp
        include/Games/MDPs/BinPacking.h
        src/Games/MDPs/BinPacking.cpp
        include/Games/MDPs/Knapsack.h
        src/Games/MDPs/Knapsack.cpp
        include/Games/MDPs/TravellingSalesPerson.h
        src/Games/MDPs/TravellingSalesPerson.cpp
        include/Games/MDPs/SupplyChain.h
        src/Games/MDPs/SupplyChain.cpp
        include/Games/Wrapper/ScriptedStart.h
        src/Games/Wrapper/ScriptedStart.cpp
        src/Agents/OneStepLookaheadAgent.cpp
        include/Agents/OneStepLookahead.h
)

#Shared lib
add_library(liblink SHARED ${SOURCE_FILES})
target_compile_options(liblink PRIVATE
        -Wall
        -fPIC
        -O3
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # For MinGW / GCC
    target_link_options(liblink PRIVATE
            $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:-static>
            $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:-static-libgcc>
            $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:-static-libstdc++>
    )
    message(STATUS "Using GCC/MinGW compiler")
else()
    message(FATAL_ERROR "Compiler '${CMAKE_CXX_COMPILER_ID}' is not supported by this build.")
endif()
set_target_properties(liblink PROPERTIES OUTPUT_NAME "liblink")

#Debug executable
add_executable(kvdaDebug ${SOURCE_FILES})
target_compile_options(kvdaDebug PRIVATE
        ${CMAKE_CXX_FLAGS_DEBUG}   # typical debug flags (-O0 -g for gcc/clang)
)
target_link_options(kvdaDebug PRIVATE
        ${CMAKE_EXE_LINKER_FLAGS}         # global linker flags
        ${CMAKE_EXE_LINKER_FLAGS_DEBUG}   # typical debug linker flags
)

#Release executable
add_executable(kvdaRelease ${SOURCE_FILES})
target_compile_options(kvdaRelease PRIVATE
        -O3
        -DNDEBUG
)
target_link_options(kvdaRelease PRIVATE
        ${CMAKE_EXE_LINKER_FLAGS}
        ${CMAKE_EXE_LINKER_FLAGS_RELEASE}
)
