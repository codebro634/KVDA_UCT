#ifndef OGAGROUNDNODES_H
#define OGAGROUNDNODES_H

#include <map>

#include "OgaUtils.h"

namespace OGA {
    struct OgaSearchStats;

    //Forward declarations
    class OgaQStateNode;
    class OgaAbstractStateNode;

    class OgaStateNode
    {
    private:
        // Information that identifies the node
        unsigned id;
        ABS::Gamestate* state;
        unsigned depth;

        // Bookkeeping for MCTS
        unsigned visits = 0;
        std::vector<int> tried_actions{};
        std::vector<int> untried_actions{};
        std::vector<OgaQStateNode*> children{};

        // OGA bookkeeping
        OgaAbstractStateNode* abstract_node = nullptr;
        Set<OgaQStateNode> parents{}; // Needed for updating state abstractions
        NextAbstractQStates* last_next_distr = nullptr;
        NextAbstractQStates* last_next_filtered_distr = nullptr;

        //Optional: Policy NN scores for each action
        std::map<int,double> policy_probs{};

        // If smart reward handling is enabled
        double vstar_dist_to_repr = 0;

        // Temporarily store the trajectory path and rewards to be able to do backpropagation
        OgaQStateNode* trajectory_parent = nullptr;
        unsigned recency_count = 0;
        bool has_received_abs_update = false;

    public:

        OgaStateNode(
            ABS::Gamestate* state,
            unsigned depth,
            OgaSearchStats& search_stats
        );

        ~OgaStateNode();

        [[nodiscard]] ABS::Gamestate* getState() const;
        [[nodiscard]] ABS::Gamestate* getStateCopy(ABS::Model* model) const;
        [[nodiscard]] unsigned getDepth() const;
        [[nodiscard]] bool isTerminal() const;
        [[nodiscard]] unsigned getId() const { return id; }
        [[nodiscard]] NextAbstractQStates* getLastNextDistr() const { return last_next_distr; }
        [[nodiscard]] NextAbstractQStates* getLastFilteredNextDistr() const { return last_next_filtered_distr; }
        [[nodiscard]] std::vector<int> getTriedActions() const { return tried_actions; }
        [[nodiscard]] bool policyProbsAvailable(ABS::Model* model);
        [[nodiscard]] double getPolicyProb(int action) const{ return policy_probs.at(action); }

        // MCTS bookkeeping functions
        void addVisit();
        [[nodiscard]] unsigned getVisits() const;
        void initUntriedActions(ABS::Model* model, const std::vector<int>& actions, std::mt19937& rng);
        [[nodiscard]] int popUntriedAction();
        [[nodiscard]] bool isFullyExpanded() const;
        [[nodiscard]] bool isPartiallyExpanded() const;
        [[nodiscard]] unsigned numTriedActions() const;
        [[nodiscard]] unsigned numUntriedActions() const;
        void addChild(OgaQStateNode* child);
        void setLastNextDistr(NextAbstractQStates* next_distr, NextAbstractQStates* next_filtered_distr) { last_next_distr = next_distr; last_next_filtered_distr = next_filtered_distr; }
        [[nodiscard]] const std::vector<OgaQStateNode*>& getChildren() const;
        //For smart reward handling
        static double consistentVstarDist(OgaStateNode* s1, OgaStateNode* s2, bool *consistent);
        void setVstarDistToRepr(double dv);
        [[nodiscard]] double getVstarDistToRepr() const { return vstar_dist_to_repr; }

        // OGA bookkeeping functions
        void setAbstractNode(OgaAbstractStateNode* abstract_node);
        [[nodiscard]] OgaAbstractStateNode* getAbstractNode() const;
        [[nodiscard]] const Set<OgaQStateNode>& getParents() const;
        [[nodiscard]] unsigned getRecencyCount() const { return recency_count; }
        void addRecencyCount() { recency_count++; }
        void resetRecencyCount() { recency_count = 0; }
        void setReceivedAbsUpdate(bool received) { has_received_abs_update = received; }
        [[nodiscard]] bool hasReceivedAbsUpdate() const { return has_received_abs_update; }

        // Trajectory functions
        void setTrajectoryParent(OgaQStateNode* parent);
        [[nodiscard]] OgaQStateNode* popTrajectoryParent();

        // Functions for hashing and comparison needed for unordered_map and unordered_set
        bool operator==(const OgaStateNode& other) const;
        [[nodiscard]] size_t hash() const;

    };

    class OgaQStateNode
    {
    private:
        unsigned id;

        // Information that identifies the node
        ABS::Gamestate* state;
        unsigned depth;
        int action;

        // Bookkeeping for MCTS
        unsigned visits = 0;
        double values = 0;
        double squared_values = 0;
        OgaStateNode* parent = nullptr; // Cached for better performance in backpropagation and state abstraction updates

        // Bookkeeping for OGA
        Map<ABS::Gamestate, std::pair<double, OgaStateNode*>> children{}; // Cached for better performance
        double prob_sum = 0; //probability of sum of all sampled successors. Maximum is 1.
        OgaAbstractQStateNode* abstract_node = nullptr;
        std::vector<double> rewards{}; // For q state abstractions, a q state needs a deterministic reward (cost)
        unsigned recency_count = 0;

        bool use_ground_stats = false;
        bool has_received_abs_update = false;

        // If smart reward handling is enabled
        double qstart_dist_to_repr = 0;

    public:

        OgaQStateNode(
            ABS::Gamestate* state,
            unsigned depth,
            int action,
            OgaSearchStats& search_stats
        );

        ~OgaQStateNode();

        [[nodiscard]] ABS::Gamestate* getState() const;
        [[nodiscard]] unsigned getDepth() const;
        [[nodiscard]] int getAction() const;
        [[nodiscard]] unsigned getId() const { return id; }

        // MCTS bookkeeping functions
        [[nodiscard]] unsigned getVisits() const;
        [[nodiscard]] double getValues() const;
        [[nodiscard]] double getSquaredValues() const;
        void addExperience(double values);
        [[nodiscard]] double getAbsVisits() const;
        [[nodiscard]] double getAbsValues() const;
        [[nodiscard]] bool hasReceivedAbsUpdate() const { return has_received_abs_update; }
        void setReceivedAbsUpdate(bool received) { has_received_abs_update = received; }
        double getProbSum() const { return prob_sum; }

        //For smart reward handling
        void recalcQstarDistToRepr();
        void setQstarDistToRepr(double qstart_dist) { qstart_dist_to_repr = qstart_dist; }
        [[nodiscard]] double getQstarDistToRepr() const { return qstart_dist_to_repr; }
        [[nodiscard]] double calcQstarDistToNode(const OgaQStateNode* other) const;

        // OGA bookkeeping functions
        bool addChild(ABS::Gamestate* state, double probability, OgaStateNode* child);
        [[nodiscard]] const Map<ABS::Gamestate, std::pair<double, OgaStateNode*>>* getChildren() const;
        void setAbstractNode(OgaAbstractQStateNode* abstract_node);
        [[nodiscard]] OgaAbstractQStateNode* getAbstractNode() const;
        void setRewards(std::vector<double>& rewards);
        [[nodiscard]] double getRewards(int player) const;
        void addRecencyCount();
        void resetRecencyCount();
        [[nodiscard]] unsigned getRecencyCount() const;

        // Trajectory functions
        void setParent(OgaStateNode* parent);
        [[nodiscard]] OgaStateNode* getParent() const;

        // Functions for hashing and comparison needed for unordered_map and unordered_set
        bool operator==(const OgaQStateNode& other) const;
        [[nodiscard]] size_t hash() const;
    };


}

#endif //OGAGROUNDNODES_H
