cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 20)

project(KVDA LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wno-unused-parameter")

set(SOURCE_FILES
        src/main.cpp
        src/Arena.cpp
        src/Agents/RandomAgent.cpp
        include/Games/Gamestate.h
        include/Agents/Agent.h
        include/Agents/RandomAgent.h
        include/Arena.h
        include/Agents/HumanAgent.h
        src/Agents/HumanAgent.cpp
        src/Games/TwoPlayerGames/Connect4.cpp
        include/Games/TwoPlayerGames/Connect4.h
        include/Games/TwoPlayerGames/Othello.h
        src/Games/TwoPlayerGames/Othello.cpp
        include/Games/TwoPlayerGames/Constrictor.h
        src/Games/TwoPlayerGames/Constrictor.cpp
        src/Games/TwoPlayerGames/Pusher.cpp
        include/Games/TwoPlayerGames/Pusher.h
        include/Games/MDPs/GameOfLife.h
        src/Games/MDPs/GameOfLife.cpp
        include/Games/MDPs/SailingWind.h
        src/Games/MDPs/SailingWind.cpp
        include/Agents/OneStepLookahead.h
        src/Agents/OneStepLookaheadAgent.cpp
        include/Agents/Oga/OgaAgent.h
        src/Agents/Oga/OgaAgent.cpp
        include/Utils/UnitTest.h
        include/Games/MDPs/SysAdmin.h
        src/Games/MDPs/SysAdmin.cpp
        include/Games/MDPs/AcademicAdvising.h
        src/Games/MDPs/AcademicAdvising.cpp
        include/Games/MDPs/ToySoccer.h
        src/Games/MDPs/ToySoccer.cpp
        include/Utils/Argparse.h
        include/Games/MDPs/Elevators.h
        src/Games/MDPs/Elevators.cpp
        include/Games/MDPs/Tamarisk.h
        src/Games/MDPs/Tamarisk.cpp
        include/Games/MDPs/Saving.h
        src/Games/MDPs/Saving.cpp
        include/Games/MDPs/FixedActionSpaceEnv.h
        src/Games/MDPs/FixedActionSpaceEnv.cpp
        include/Games/MDPs/GraphTraversal.h
        src/Games/MDPs/GraphTraversal.cpp
        include/Utils/Distributions.h
        src/Utils/Distributions.cpp
        include/Games/MDPs/GraphTraversal.h
        src/Games/MDPs/GraphTraversal.cpp
        include/Games/MDPs/Wildfire.h
        src/Games/MDPs/Wildfire.cpp
        include/Games/MDPs/CooperativeRecon.h
        src/Games/MDPs/CooperativeRecon.cpp
        include/Games/MDPs/Traffic.h
        src/Games/MDPs/Traffic.cpp
        include/Games/MDPs/SkillsTeaching.h
        src/Games/MDPs/SkillsTeaching.cpp
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
        include/Utils/MiscAnalysis.h
        src/Utils/MiscAnalysis.cpp
        include/Games/TwoPlayerGames/WinLossGames.h
        src/Games/TwoPlayerGames/WinLossGames.cpp
        include/Utils/MemoryAnalysis.h
        src/Utils/MemoryAnalysis.cpp
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
        include/Utils/ValueIteration.h
        src/Utils/ValueIteration.cpp
)

#Debug executable
add_executable(KVDADebug ${SOURCE_FILES})
target_compile_options(KVDADebug PRIVATE
        ${CMAKE_CXX_FLAGS_DEBUG}   # typical debug flags (-O0 -g for gcc/clang)
)
target_link_options(KVDADebug PRIVATE
        ${CMAKE_EXE_LINKER_FLAGS}         # global linker flags
        ${CMAKE_EXE_LINKER_FLAGS_DEBUG}   # typical debug linker flags
)

#Release executable
add_executable(KVDARelease ${SOURCE_FILES})
target_compile_options(KVDARelease PRIVATE
        -O3
        -DNDEBUG
)
target_link_options(KVDARelease PRIVATE
        ${CMAKE_EXE_LINKER_FLAGS}
        ${CMAKE_EXE_LINKER_FLAGS_RELEASE}
)