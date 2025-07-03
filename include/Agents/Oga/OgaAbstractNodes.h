#ifndef OGAABSTRACTNODES_H
#define OGAABSTRACTNODES_H
#include <set>

#include "OgaAgent.h"
#include "OgaGroundNodes.h"

namespace OGA {

    class OgaAbstractNode
    {
    private:
        void* key = nullptr;
        unsigned count = 0;
        unsigned id;
        unsigned depth;

    protected:
        OgaAbstractNode(const unsigned depth, unsigned id) : id(id), depth(depth) {}
        void increaseCount();
        void decreaseCount();

    public:

        [[nodiscard]] unsigned getId() const;
        [[nodiscard]] unsigned getDepth() const;
        void* popAndSetKey(void* key);
        [[nodiscard]] void* getKey() const { return key; }
        [[nodiscard]] unsigned getCount() const;

        [[nodiscard]] bool operator==(const OgaAbstractNode& other) const;
        [[nodiscard]] size_t hash() const;
    };

    // Forward declaration
    class OgaAbstractStateNode;
    struct AbsStateCompare;
    using AbsStateSet = std::set<OgaAbstractStateNode*, AbsStateCompare>;

    class OgaAbstractStateNode : public OgaAbstractNode
    {
    public:
        using OgaAbstractNode::OgaAbstractNode;

        explicit OgaAbstractStateNode(unsigned depth, OgaSearchStats& search_stats): OgaAbstractNode(depth, search_stats.max_abs_state_id++) {}

        static void transfer(OgaStateNode* state_node, OgaAbstractStateNode* from, OgaAbstractStateNode* to, AbsStateSet& abstract_state_nodes);
        void add(OgaStateNode* state_node);
        void remove(OgaStateNode* state_node);
        [[nodiscard]] OgaStateNode* getRepresentant() const;
        void setRepresentant(OgaStateNode* representant);
        std::set<OgaStateNode*>& getGroundNodes() { return ground_nodes; }

    private:
        OgaStateNode* representant = nullptr; //only needed when eps_a > 0 or eps_t > 0
        std::set<OgaStateNode*> ground_nodes;
    };


    // Forward declaration
    class OgaAbstractQStateNode;
    struct AbsQCompare;
    using AbsQSet = std::set<OgaAbstractQStateNode*, AbsQCompare>;

    class OgaAbstractQStateNode : public OgaAbstractNode
    {
    private:
        OgaQStateNode * representant = nullptr; //only needed when eps_a or eps_t > 0
        std::set<OgaQStateNode*> ground_nodes;

        double values = 0;
        double visits = 0;
        double squared_values = 0; //only needed for std calculation for OGA-CAD (abs dropping)

    public:
        using OgaAbstractNode::OgaAbstractNode;
        explicit OgaAbstractQStateNode(unsigned depth, OgaSearchStats& search_stats): OgaAbstractNode(depth, search_stats.max_abs_q_id++) {}

        [[nodiscard]] double getValues() const;
        [[nodiscard]] double getSquaredValues() const;
        [[nodiscard]] double getVisits() const;
        [[nodiscard]] OgaQStateNode* getRepresentant() const;
        void setRepresentant(OgaQStateNode* representant);
        void addExperience(double values);
        std::set<OgaQStateNode*>& getGroundNodes() { return ground_nodes; }

        static void transfer(OgaQStateNode* q_state_node, OgaAbstractQStateNode* from, OgaAbstractQStateNode* to, AbsQSet& abs_q_set, OgaBehaviorFlags& flags);
        void add(OgaQStateNode* q_state_node, OgaBehaviorFlags& flags);
        void remove(OgaQStateNode* q_state_node, OgaBehaviorFlags& flags);
        void addValues(double amount) { values += amount; }

        //For debugging
        void testValueConsistency() const;
    };

    struct AbsQCompare{
        bool operator()(const OgaAbstractQStateNode* lhs, const OgaAbstractQStateNode* rhs) const{
            if (lhs->getCount() < rhs->getCount())
                return true;
            else if (lhs->getCount() == rhs->getCount()) {
                return lhs->getId() > rhs->getId();
            }else
                return false;
        }
    };

    struct AbsStateCompare{
        bool operator()(const OgaAbstractStateNode* lhs, const OgaAbstractStateNode* rhs) const{
            if (lhs->getCount() < rhs->getCount())
                return true;
            else if (lhs->getCount() == rhs->getCount()) {
                return lhs->getId() > rhs->getId();
            }else
                return false;
        }
    };


}

#endif //OGAABSTRACTNODES_H
