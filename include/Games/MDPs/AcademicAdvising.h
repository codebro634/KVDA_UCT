#pragma once

#ifndef AA_H
#define AA_H
#include <set>
#include <vector>

#include "../Gamestate.h"
#endif

namespace AA
{

    const static float PRIOR_PROB_PASS_NO_PREREQ = 0.8;
    const static float PRIOR_PROB_PASS = 0.2;
    const static int COURSE_COST = 1;
    const static int REDO_COST = 2;
    const static int INCOMPLETE_COST = 5;
    const static int PASS_REWARD = 5; //only needed for dense reward setting

    const static std::vector<std::vector<int>> DEFAULT_PREREQS = {
        {},
        {0},
        {0,1},
        {0, 1, 2},
        {0, 1, 2, 3}
    };

    const static std::vector<int> DEFAULT_REQ_COURSES = {2,4};

    struct Gamestate: public ABS::Gamestate{
        unsigned int passed_courses; //i-th bit indicates if i-th course has been passed
        unsigned int taken_courses; //i-th bit indicates if i-th course has been taken

        //only for computational efficiency, these could be derived from the above two
        int num_passed=0,num_taken=0;
        std::set<int> missing_reqs;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
        [[nodiscard]] std::string toString() const override;

        void setIthCoursePassed(int i);
        void setIthCourseTaken(int i);
        bool isIthCoursePassed(int i);
        bool isIthCourseTaken(int i);
    };

    class Model: public ABS::Model
    {
    public:
        ~Model() override = default;
        explicit Model(const std::string& fileName, bool dense_rewards, bool idle_action);
        void printState(ABS::Gamestate* state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        ABS::Gamestate* getInitialState(int num) override;
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}

        [[nodiscard]] double getMinV(int steps) const override {return -(INCOMPLETE_COST + std::max(REDO_COST,COURSE_COST))*steps;}
        [[nodiscard]] double getMaxV(int steps) const override {return -INCOMPLETE_COST + std::max(REDO_COST,COURSE_COST);}

        [[nodiscard]] ABS::Gamestate* deserialize(std::string &ostring) const override;

        [[nodiscard]] std::vector<int> obsShape() const override;
        void getObs(ABS::Gamestate* uncasted_state, int* obs);
        [[nodiscard]] std::vector<int> actionShape() const override;
        [[nodiscard]] int encodeAction(int* decoded_action) override;

    private:
        bool dense_rewards;
        bool simultaneous_actions;
        bool idle_action;

        std::vector<int> actions;
        std::vector<std::vector<int>> prereqs;
        std::vector<int> req_courses;
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;
        [[nodiscard]] double getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const override;

    };

}

