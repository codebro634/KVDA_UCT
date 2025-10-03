
#include <random>
#include <ranges>

#include "../../../include/Agents/Oga/OgaAgent.h"
#include "../../../include/Agents/Oga/OgaTree.h"
#include "../../../include/Agents/Oga/OgaUtils.h"
#include "../../../include/Agents/Oga/OgaAbstractNodes.h"
#include "../../../include/Agents/Oga/OgaGroundNodes.h"

#include "../../../include/Games/MDPs/GraphTraversal.h"
#include "../../../include/Games/MDPs/Navigation.h"
#include "../../../include/Utils/UnitTest.h"
#include "../../../include/Games/Wrapper/FiniteHorizon.h"

namespace OGA
{
    /*
     * Test that uses a simple 2x2 navigation map to check if the agent selects the correct action.
     */
    void simpleTest()
    {
        auto agent = OGA::OgaAgent(
            OGA::OgaArgs{
                .budget = {1000, "iterations"},
                .recency_count_limit = 3,
                .exploration_parameter = 1.0,
                .discount = 1.0,
                .behavior_flags = {.group_terminal_states = false}
            });
        auto model = FINITEH::Model(new Navigation::Model("../resources/NavigationMaps/Test_Simple2x2.txt",false),50,true);
        auto rng = std::mt19937(42);

        const auto state = model.getInitialState(rng);
        OGA::OgaTree* tree;
        const auto action = agent.getAction(&model, state, rng, &tree);
        ASSERT_EQUALS(1,action);
        delete tree;
        delete state;

        std::cout << "- Simple test done" << std::endl;
    }

    void simpleGraphTest()
    {
        auto agent = OGA::OgaAgent(
            OGA::OgaArgs{
                .budget = {1000, "iterations"},
                .recency_count_limit = 3,
                .exploration_parameter = 1.0,
                .discount = 1.0,
                .behavior_flags = {.group_terminal_states = false}
            });
        auto model = FINITEH::Model(new GTR::Model("../resources/GraphTraversalGraphs/snowflake.txt"),50,true);
        auto rng = std::mt19937(42);

        const auto state = model.getInitialState(rng);
        OGA::OgaTree* tree;
        agent.getAction(&model, state, rng, &tree);

        const auto* root = tree->getRoot();
        ASSERT_EQUALS(0,root->getDepth());
        ASSERT_EQUALS(1000,root->getVisits());

        OGA::Set<OGA::OgaAbstractQStateNode> children;
        for (const auto* child : root->getChildren())
        {
            children.insert(child->getAbstractNode());
        }

        ASSERT_EQUALS(4, children.size());

        delete tree;
        delete state;

        std::cout << "- Simple graph test done" << std::endl;
    }

    void simpleGroupTerminalTest()
    {
        auto agent = OGA::OgaAgent(
            OGA::OgaArgs{
                .budget = {1000, "iterations"},
                .recency_count_limit = 3,
                .exploration_parameter = 1.0,
                .discount = 1.0,
                .behavior_flags = {.group_terminal_states = true}
            });
        auto model = FINITEH::Model(new GTR::Model("../resources/GraphTraversalGraphs/snowflake.txt"),50,true);
        auto rng = std::mt19937(42);

        const auto state = model.getInitialState(rng);
        OGA::OgaTree* tree;
        agent.getAction(&model, state, rng, &tree);

        const auto* root = tree->getRoot();
        ASSERT_EQUALS(0, root->getDepth());
        ASSERT_EQUALS(1000, root->getVisits());

        OGA::Set<OGA::OgaAbstractQStateNode> children;
        for (const auto* child : root->getChildren())
        {
            children.insert(child->getAbstractNode());
        }
        ASSERT_EQUALS(2, children.size());
        delete tree;
        delete state;

        std::cout << "- Simple group terminal test done" << std::endl;
    }

    void simpleGroupHorizonTest(){
        auto agent = OGA::OgaAgent(
            OGA::OgaArgs{
                .budget = {1000, "iterations"},
                .recency_count_limit = 3,
                .exploration_parameter = 1.0,
                .discount = 1.0,
                .behavior_flags = {.group_terminal_states = true}
            });
        std::cout << "Starting simple group horizon test" << std::endl;
        auto model = FINITEH::Model(new GTR::Model("../resources/GraphTraversalGraphs/horizon.txt"),2,true); //short horizon
        auto rng = std::mt19937(42);

        auto state = model.getInitialState(rng);

        OGA::OgaTree* tree;
        agent.getAction(&model, state, rng, &tree);


        const auto* root = tree->getRoot();
        ASSERT_EQUALS(0, root->getDepth());
        ASSERT_EQUALS(1000, root->getVisits());

        OGA::Set<OGA::OgaAbstractQStateNode> children;
        for (const auto* child : root->getChildren()){
            children.insert(child->getAbstractNode());
        }
        ASSERT_EQUALS(1, children.size());

        delete tree;
        delete state;

        auto model2 = FINITEH::Model(new GTR::Model("../resources/GraphTraversalGraphs/horizon.txt"),10,true); //longer horizon
        state = model2.getInitialState(rng);

        agent.getAction(&model2, state, rng, &tree);


        root = tree->getRoot();
        ASSERT_EQUALS(0, root->getDepth());
        ASSERT_EQUALS(1000, root->getVisits());

        children.clear();
        for (const auto* child : root->getChildren())
        {
            children.insert(child->getAbstractNode());
        }
        ASSERT_EQUALS(2, children.size());

        delete tree;
        delete state;


        std::cout << "- Simple group horizon test done" << std::endl;
    }


}




void OGA::OgaAgent::runTests() {
    std::cout << "Running tests for OgaAgent" << std::endl;

    simpleTest();
    simpleGraphTest();
    simpleGroupTerminalTest();
    simpleGroupHorizonTest();

    std::cout << "Finished tests for OgaAgent" << std::endl;

}
