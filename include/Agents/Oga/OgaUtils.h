#ifndef OGAUTILS_H
#define OGAUTILS_H

#include <unordered_map>
#include <unordered_set>

#include "../../Games/Gamestate.h"

namespace OGA {

    //Forward declarations
    class OgaAbstractQStateNode;
    class OgaAbstractStateNode;
    class NextDistribution;

    struct OgaBehaviorFlags{

        bool group_terminal_states : 1 = true;
        bool group_partially_expanded_states : 1 = false;
        unsigned int partial_expansion_group_threshold = std::numeric_limits<int>::max();

        //For non-exact abstractions
        std::string q_abs_alg = "eps"; //vals: 'eps', 'random'
        double eps_a = 0;
        double eps_t = 0;
        bool consider_missing_outcomes = false; //If true, missing outcomes are considered in the distribution distance
        double alpha = 0; //for pruned-oga

        double equiv_chance = 0.1; //For random abstraction algorithm, applies to state_abs_alg="random" and q_abs_alg="random"
        std::string state_abs_alg = "asap"; //vals: 'asap', 'random'

        bool smart_reward_handling = false; // standard oga + smart_reward_handling => kvda-uct
    };

    class OgaStateNode;
    class OgaQStateNode;
    class OgaTree;


    template <class T>
    struct PointedHash
    {
        size_t operator()(const T* p) const
        {
            return p->hash();
        }
    };

    template <class T>
    struct PointedCompare
    {
        bool operator()(const T* lhs, const T* rhs) const
        {
            return lhs == rhs || *lhs == *rhs;
        }
    };

    template <class T>
    using Set = std::unordered_set<T*, PointedHash<T>, PointedCompare<T>>;

    template <class T, class U>
    using Map = std::unordered_map<T*, U, PointedHash<T>, PointedCompare<T>>;

    /*
     * Saves the probability of changing from one q state (depth d) to an abstract state with depth d + 1
     * with reward r.
     */
    class NextDistribution
    {
    private:
        Map<OgaAbstractStateNode, double> distribution{};
        double rewards;
        bool consider_missing_outcomes, ignore_reward_diffs;

    public:

        explicit NextDistribution(const double rewards, bool consider_missing_outcomes, bool ignore_reward_diffs);
        void addProbability(OgaAbstractStateNode* next_abstract_state_node, double probability);

        double getRewards() const;
        const Map<OgaAbstractStateNode, double>& getDistribution() const;

        [[nodiscard]] double rewardDist(const NextDistribution* other) const;
        [[nodiscard]] double transDist(const NextDistribution* other) const;

        [[nodiscard]] double dist(const NextDistribution* other) const;
        bool approxEqual(const NextDistribution* other, double exp_a, double exp_t) const;
    };

    class NextAbstractQStates
    {
    private:
        Set<OgaAbstractQStateNode> states{};

    public:
        NextAbstractQStates() = default;

        void addAbstractQState(OgaAbstractQStateNode* next_abstract_q_state_node);

        const Set<OgaAbstractQStateNode>& getStates() const;

        bool operator==(const NextAbstractQStates& other) const;
        [[nodiscard]] size_t hash() const;
    };

}

#endif //OGAUTILS_H
