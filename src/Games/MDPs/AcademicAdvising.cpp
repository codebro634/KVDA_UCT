#include <bits/atomic_base.h>

#include "../../../include/Games/MDPs/AcademicAdvising.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace std;

using namespace AA;


std::vector<int> Model::obsShape() const {
    return {static_cast<int>(prereqs.size())};
}

void Model::getObs(ABS::Gamestate* uncasted_state, int* obs) {
    auto* AAState = dynamic_cast<AA::Gamestate*>(uncasted_state);
    for (size_t i = 0; i < prereqs.size(); ++i)
        obs[i] = AAState->isIthCoursePassed(i) ? 1 : 0;
}

[[nodiscard]] std::vector<int> Model::actionShape() const {
    int idle = idle_action? 1 : 0;
    if (simultaneous_actions)
        return {static_cast<int>(prereqs.size() + idle), static_cast<int>(prereqs.size() + idle)};
    else
        return {static_cast<int>(prereqs.size() + idle)};
}

int Model::encodeAction(int* decoded_action) {
    if (simultaneous_actions) {
        int c1 = decoded_action[0]- (idle_action? 1:0);
        int c2 = decoded_action[1]- (idle_action? 1:0);
        if (c1 > c2)
            std::swap(c1, c2);

        if (c1 == -1 && c2 == -1)
            return -1;
        else if (c1 == -1)
            return c2;
        else if (c2 == -1)
            return c1;
        else
            return (c1 << 10) | c2;
    }else
        return decoded_action[0] - (idle_action? 1:0);
}

[[nodiscard]] std::string Gamestate::toString() const {
    return "((" + std::to_string(passed_courses) + "," + std::to_string(taken_courses) + ")" + "," + ABS::Gamestate::toString() + ")";
}


ABS::Gamestate* Model::deserialize(std::string &ostring) const {
    auto* state = new Gamestate();
    int passed, taken, turn, terminal;
    sscanf(ostring.c_str(), "((%d,%d),(%d,%d))", &passed, &taken, &turn, &terminal);
    state->passed_courses = passed;
    state->taken_courses = taken;
    state->num_taken = 0;
    state->num_passed = 0;

    auto req_set = std::set<int>(req_courses.begin(), req_courses.end());

    for(size_t i = 0; i < prereqs.size(); i++){
        if(!state->isIthCoursePassed(i) && req_set.contains(i))
                state->missing_reqs.insert(i);
        if(state->isIthCoursePassed(i))
            state->num_passed++;
        if(state->isIthCourseTaken(i))
            state->num_taken++;
    }

    state->turn = turn;
    state->terminal = terminal;
    return state;
}

Model::Model(const std::string& fileName, bool dense_rewards, bool idle_action)
{
    this->dense_rewards = dense_rewards;
    //std::cout << "Reading file " << fileName << std::endl;
    
    std::ifstream file(fileName); // Open the file
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << fileName << std::endl;
        return;
    }

    std::string line;
    int line_num = 0;
    [[maybe_unused]] int num_courses;

    // Read the file line by line
    while (std::getline(file, line)) {
        std::istringstream iss(line); // Create a string stream for each line

        if (line == "empty")
            prereqs.emplace_back();
        else if (line_num > 0){
            std::vector<int> courses;
            int course;

            // Parse integers from the line
            while (iss >> course)
                courses.push_back(course);

            if (line_num == 1)
                req_courses = courses; // Transfer parsed data to req_courses
            else
                prereqs.push_back(courses); // Add parsed data to prereqs
        }else{
            iss >> num_courses;
            iss >> simultaneous_actions;
        }

        line_num++;
    }

    assert (num_courses == static_cast<int>(prereqs.size())); // Check that the number of courses is correct

    for(size_t i = 0; i < prereqs.size(); i++)
        actions.push_back(i);
    if (simultaneous_actions){
        for(size_t i = 0; i < prereqs.size(); i++){
            for(size_t j = i+1; j < prereqs.size(); j++){
                actions.push_back((i << 10) | j);
            }
        }
    }
    this->idle_action = idle_action;
    if(idle_action)
        actions.push_back(-1);
    file.close(); // Close the file
    assert (prereqs.size() <= 32); //We are using a 32 bit integer to store the passed courses. If one wants to increase this limit, change == operator
}

inline bool Gamestate::isIthCoursePassed(int i)
{
    return passed_courses & (1 << i);
}

inline bool Gamestate::isIthCourseTaken(int i)
{
    return taken_courses & (1 << i);
}

inline void Gamestate::setIthCoursePassed(int i)
{
    passed_courses |= (1 << i);
}

inline void Gamestate::setIthCourseTaken(int i)
{
    taken_courses |= (1 << i);
}

double Model::getDistance(const ABS::Gamestate* a, const ABS::Gamestate* b) const {
    const Gamestate* state_a = (Gamestate*) a;
    const Gamestate* state_b = (Gamestate*) b;
    return __builtin_popcount( state_a->passed_courses ^ state_b->passed_courses) + __builtin_popcount( state_a->taken_courses ^ state_b->taken_courses);
}

bool Gamestate::operator==(const ABS::Gamestate& other) const
{
    return passed_courses == dynamic_cast<const Gamestate&>(other).passed_courses && taken_courses == dynamic_cast<const Gamestate&>(other).taken_courses && terminal == dynamic_cast<const Gamestate&>(other).terminal;
}

size_t Gamestate::hash() const
{
    return passed_courses | (taken_courses << 16);
}

void Model::printState(ABS::Gamestate* state) {
    auto* AAState = dynamic_cast<AA::Gamestate*>(state);
    if (!AAState) return;

    for(size_t i = 0; i < actions.size(); i++)
        std::cout << "Passed course " << i <<": " << (AAState->isIthCoursePassed(i)? "Yes":"No") << std::endl;

    for(size_t i = 0; i < actions.size(); i++)
        std::cout << "Taken course " << i <<": " << (AAState->isIthCourseTaken(i)? "Yes":"No") << std::endl;

    //print missing courses
    std::cout << "Missing courses: ";
    for(auto req: AAState->missing_reqs)
        std::cout << req << " ";
    std::cout << std::endl;

    //print rerequites
    for (size_t i = 0; i < prereqs.size(); ++i) {
        std::cout << i << "<- ";
        for (size_t j = 0; j < prereqs[i].size(); ++j) {
            std::cout << prereqs[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

ABS::Gamestate* Model::getInitialState(int num) {
    auto* state = new AA::Gamestate();
    state->passed_courses = 0;
    state->taken_courses = 0;
    state->num_passed = 0;
    state->num_taken = 0;
    for(auto req: req_courses)
        state->missing_reqs.insert(req);
    return state;
}

ABS::Gamestate* Model::getInitialState(std::mt19937& rng)
{
    return getInitialState(0);
}

int Model::getNumPlayers() {
    return 1;
}


ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state) {
    auto state = dynamic_cast<Gamestate*>(uncasted_state);
    auto new_state = new Gamestate();
    *new_state = *state; //default copy constructor should work
    return new_state;
}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state)  {
    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) {
    auto* state = dynamic_cast<AA::Gamestate*>(uncasted_state);

    float p = 1;
    double reward = 0;
    size_t decision_point = 0;

    if(dense_rewards) {
        reward += state->num_passed / (double) actions.size() * PASS_REWARD;
        reward -= (state->missing_reqs.size() / (double) req_courses.size()) * INCOMPLETE_COST;
    }else
        reward -= state->missing_reqs.empty()? 0 : INCOMPLETE_COST;

    if (action != -1){

        auto courses = std::vector<int>();
        courses.push_back(action & ((1 << 10) - 1));
        if (simultaneous_actions && (action >> 10) != 0)
            courses.push_back(action >> 10);
        for (int course : courses){

            //Reward handling and update taken course
            if(!state->isIthCourseTaken(course)) {
                state->setIthCourseTaken(course);
                state->num_taken++;
                reward -= COURSE_COST;
            }else{
                reward -= REDO_COST;
            }

            if(!state->isIthCoursePassed(course)) {
                int n_prereqs = prereqs[course].size();
                double sample = std::uniform_real_distribution<double>(0, 1)(rng);
                //No prerequisites
                if(n_prereqs == 0) {
                    if( (decision_outcomes == nullptr && sample < PRIOR_PROB_PASS_NO_PREREQ) || (decision_outcomes != nullptr && (PRIOR_PROB_PASS_NO_PREREQ == 1 || (PRIOR_PROB_PASS_NO_PREREQ != 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))) {
                        state->setIthCoursePassed(course);
                        state->num_passed++;
                        state->missing_reqs.erase(course);
                        p *= PRIOR_PROB_PASS_NO_PREREQ;
                    }else {
                        p *= 1 - PRIOR_PROB_PASS_NO_PREREQ;
                    }
                    //Atleast one reprequisite
                }else {
                    int n_passed_prereqs = 0;
                    for(auto prereq: prereqs[course]) {
                        if(state->isIthCoursePassed(prereq))
                            n_passed_prereqs++;
                    }
                    double p_pass = PRIOR_PROB_PASS + (1 - PRIOR_PROB_PASS) * (double) n_passed_prereqs / (1.0 + n_prereqs);
                    if( (decision_outcomes == nullptr && sample < p_pass) || (decision_outcomes != nullptr && (p_pass == 1 || (p_pass != 0 && 0 == getDecisionPoint(decision_point, 0, 1, decision_outcomes))))) {
                        state->setIthCoursePassed(course);
                        state->num_passed++;
                        state->missing_reqs.erase(course);
                        p *= p_pass;
                    }else {
                        p *= 1 - p_pass;
                    }
                }
            }
        }
    }

    //Terminal handling
    state->terminal = state->passed_courses == static_cast<unsigned int>(1 << actions.size()) - 1;

    return {{(double)reward}, p};
}