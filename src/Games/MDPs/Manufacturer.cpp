#include "../../../include/Games/MDPs/Manufacturer.h"
#include <fstream>
#include <sstream>
#include <cmath>

using namespace MAN;

bool Gamestate::operator==(const ABS::Gamestate& o) const {
    const auto* other = dynamic_cast<const Gamestate*>(&o);
    return
    has_lobbyist == other->has_lobbyist && has_marketing_manager == other->has_marketing_manager &&
            prod_manager_applies == other->prod_manager_applies && lobbyist_applies == other->lobbyist_applies &&
            marketing_manager_applies == other->marketing_manager_applies &&
            construction_progress_factories == other->construction_progress_factories &&
            goods_in_stock == other->goods_in_stock && factories == other->factories &&
            good_in_stock_for_sale == other->good_in_stock_for_sale &&
            current_price_levels == other->current_price_levels &&
            current_price_trends == other->current_price_trends && prod_managers == other->prod_managers;
}

size_t Gamestate::hash() const {
    int price_hash = 0;
    for(size_t i = 0; i < current_price_levels.size(); i++) {
       price_hash = (price_hash << 3) | current_price_levels[i];
        price_hash = (price_hash << 2) | current_price_trends[i];
    }
    int goods_in_stock_hash = 0;
    for(size_t i = 0; i < goods_in_stock.size(); i++) {
        for(size_t j = 0; j < goods_in_stock[i].size(); j++) {
            goods_in_stock_hash = (goods_in_stock_hash << 3) | goods_in_stock[i][j];
        }
    }
    int application_hash = marketing_manager_applies + (lobbyist_applies << 10) + (prod_manager_applies << 20);
    int staff_hash = 0;
    for(size_t i = 0; i < prod_managers.size(); i++) {
        staff_hash = (staff_hash << 1) | prod_managers[i];
    }
    staff_hash = (staff_hash << 20) | (has_lobbyist << 11) | (has_marketing_manager << 3);
    int factories_hash = 0;
    for(size_t i = 0; i < factories.size(); i++) {
        factories_hash = (factories_hash << 1) | factories[i];
    }
    int construction_hash = 0;
    for(size_t i = 0; i < construction_progress_factories.size(); i++) {
        construction_hash = (construction_hash << 7) | construction_progress_factories[i];
    }

    return price_hash ^ goods_in_stock_hash ^ application_hash ^ staff_hash ^ factories_hash ^ construction_hash;
}

Model::Model(const std::string& filePath){
    std::ifstream in(filePath);
    //test if file was opened correctly
    if(!in.is_open()){
        std::cerr << "Could not open file: " << filePath << std::endl;
        exit(1);
    }

    in >> num_goods;

    int num_process_dependencies;
    in >> num_process_dependencies;
    processed_to = std::vector<std::vector<bool>>(num_goods,std::vector<bool>(num_goods,false));
    for(int i = 0; i < num_process_dependencies; i++) {
        int from, to;
        in >> from >> to;
        processed_to[from][to] = true;
    }

    prices = std::vector<double>(num_goods,0);
    for(int i = 0; i < num_goods; i++)
        in >> prices[i];

    in >> max_factories;
    build_factory_cost = std::vector<double>(num_goods,0);
    delay_probs = std::vector<double>(num_goods,0);
    for(int i = 0; i < num_goods; i++) {
        in >> build_factory_cost[i] >> delay_probs[i];
    }

    in >> salary_marketing_manager >> salary_lobbyist >> salary_prod_manager;

    in >> marketing_manager_apply_prob >> lobbyist_apply_prob >> prod_manager_apply_prob;

    in >> marketing_manager_bonus >> lobbyist_bonus;

    int init_factory;
    in >> init_factory;
    init_factories = std::vector<bool>(num_goods,false);
    init_factories[init_factory] = true;

    init_trends = std::vector<int>(num_goods,1);
    for(int i = 0; i < num_goods; i++) {
        in >> init_trends[i];
    }

    price_trend_change_probs = std::vector<double>(num_goods,0.05);
    for(int i = 0; i < num_goods; i++) {
        in >> price_trend_change_probs[i];
    }

    price_level_transition_prob = std::vector<std::vector<std::vector<double>>>(3,std::vector<std::vector<double>>(7,std::vector<double>(7,0.0)));

    //down probs
    price_level_transition_prob[0][0][0] = 1.0;

    price_level_transition_prob[0][1][0] = 0.2;
    price_level_transition_prob[0][1][1] = 0.8;

    price_level_transition_prob[0][2][1] = 0.2;
    price_level_transition_prob[0][2][2] = 0.8;

    price_level_transition_prob[0][3][2] = 0.2;
    price_level_transition_prob[0][3][3] = 0.8;

    price_level_transition_prob[0][4][3] = 0.2;
    price_level_transition_prob[0][4][4] = 0.8;

    price_level_transition_prob[0][5][4] = 0.2;
    price_level_transition_prob[0][5][5] = 0.8;

    price_level_transition_prob[0][6][5] = 0.2;
    price_level_transition_prob[0][6][6] = 0.8;


    //stable probs
    price_level_transition_prob[1][0][0] = 0.9;
    price_level_transition_prob[1][0][1] = 0.1;

    price_level_transition_prob[1][1][0] = 0.1;
    price_level_transition_prob[1][1][1] = 0.8;
    price_level_transition_prob[1][1][2] = 0.1;

    price_level_transition_prob[1][2][1] = 0.1;
    price_level_transition_prob[1][2][2] = 0.8;
    price_level_transition_prob[1][2][3] = 0.1;

    price_level_transition_prob[1][3][2] = 0.1;
    price_level_transition_prob[1][3][3] = 0.8;
    price_level_transition_prob[1][3][4] = 0.1;

    price_level_transition_prob[1][4][3] = 0.1;
    price_level_transition_prob[1][4][4] = 0.8;
    price_level_transition_prob[1][4][5] = 0.1;

    price_level_transition_prob[1][5][4] = 0.1;
    price_level_transition_prob[1][5][5] = 0.8;
    price_level_transition_prob[1][5][6] = 0.1;

    price_level_transition_prob[1][6][5] = 0.1;
    price_level_transition_prob[1][6][6] = 0.9;

    //up probs
    price_level_transition_prob[2][0][1] = 0.2;
    price_level_transition_prob[2][0][0] = 0.8;


    price_level_transition_prob[2][1][2] = 0.2;
    price_level_transition_prob[2][1][1] = 0.8;

    price_level_transition_prob[2][2][3] = 0.2;
    price_level_transition_prob[2][2][2] = 0.8;

    price_level_transition_prob[2][3][4] = 0.2;
    price_level_transition_prob[2][3][3] = 0.8;

    price_level_transition_prob[2][4][5] = 0.2;
    price_level_transition_prob[2][4][4] = 0.8;

    price_level_transition_prob[2][5][6] = 0.2;
    price_level_transition_prob[2][5][5] = 0.8;

    price_level_transition_prob[2][6][6] = 1.0;

    //price level factors
    price_level_factors = std::vector<double>(7,0.0);
    price_level_factors[0] = 0.5;
    price_level_factors[1] = 0.6667;
    price_level_factors[2] = 0.8333;
    price_level_factors[3] = 1.0;
    price_level_factors[4] = 1.1667;
    price_level_factors[5] = 1.3333;
    price_level_factors[6] = 1.5;

    assert (num_goods <= 8 && max_factories <= 3); //otherwise the 32-bit action representation does no longer work
}

void Model::printState(ABS::Gamestate* uncasted_state){
    auto* s = dynamic_cast<Gamestate*>(uncasted_state);
    //print all attributes
    std::cout << "Goods in stock:" << std::endl;
    for(size_t i = 0; i < s->goods_in_stock.size(); i++) {
        for(size_t j = 0; j < s->goods_in_stock.size(); j++) {
            std::cout << s->goods_in_stock[i][j] << " ";
        }
        std::cout << std::endl;
    }
    std::cout << "Factories:" << std::endl;
    for(size_t i = 0; i < s->factories.size(); i++) {
        std::cout << s->factories[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Construction progress factories:" << std::endl;
    for(size_t i = 0; i < s->construction_progress_factories.size(); i++) {
        std::cout << s->construction_progress_factories[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Goods in stock for sale:" << std::endl;
    for(size_t i = 0; i < s->good_in_stock_for_sale.size(); i++) {
        std::cout << s->good_in_stock_for_sale[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Current price levels:" << std::endl;
    for(size_t i = 0; i < s->current_price_levels.size(); i++) {
        std::cout << s->current_price_levels[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Current price trends:" << std::endl;
    for(size_t i = 0; i < s->current_price_trends.size(); i++) {
        std::cout << s->current_price_trends[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Prod managers:" << std::endl;
    for(size_t i = 0; i < s->prod_managers.size(); i++) {
        std::cout << s->prod_managers[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Has lobbyist:" << s->has_lobbyist << std::endl;
    std::cout << "Has marketing manager:" << s->has_marketing_manager << std::endl;
    std::cout << "Prod manager applies:" << s->prod_manager_applies << std::endl;
    std::cout << "Lobbyist applies:" << s->lobbyist_applies << std::endl;
    std::cout << "Marketing manager applies:" << s->marketing_manager_applies << std::endl;
}

ABS::Gamestate* Model::getInitialState(std::mt19937&){
    auto* s = new Gamestate();

    s->goods_in_stock = std::vector<std::vector<bool>>(num_goods,std::vector<bool>(num_goods,false));
    s->good_in_stock_for_sale = std::vector<bool>(num_goods,false);
    s->current_price_levels = std::vector<int>(num_goods,3);
    s->current_price_trends = init_trends;
    s->factories = init_factories;
    s->construction_progress_factories = std::vector<int>(num_goods,0);
    s->prod_managers = std::vector<bool>(num_goods,false);
    s->has_lobbyist = false;
    s->has_marketing_manager = false;
    s->prod_manager_applies = false;
    s->lobbyist_applies = false;
    s->marketing_manager_applies = false;

    return s;
}

int Model::getNumPlayers(){
    return 1;
}

ABS::Gamestate* Model::copyState(ABS::Gamestate* uncasted_state){
    auto* sOld = dynamic_cast<Gamestate*>(uncasted_state);
    auto* sNew = new Gamestate();
    *sNew = *sOld;
    return sNew;
}

void unpackAction(int unsigned_action, int num_goods,
    std::vector<std::vector<int>>& purchases,
    std::vector<std::vector<int>>& produces,
    std::vector<std::vector<int>>& purchase_produce,
    std::vector<bool>& purchase_produce_sell_bool,
    std::vector<int>& produce_for_sale,
    std::vector<bool>& produce_for_sale_bool,
    std::vector<int>& purchase_produce_sell,
    std::vector<bool>& sell,
    int& building,
    int& hired_prod_managers,
    bool& hire_lobbyist,
    bool& hire_marketing_manager){

    unsigned int action = unsigned_action;

    hired_prod_managers = -1;
    building = -1;
    sell = std::vector<bool>(num_goods,false);
    produces = std::vector<std::vector<int>>(num_goods,std::vector<int>());
    purchase_produce = std::vector<std::vector<int>>(num_goods,std::vector<int>());
    purchases = std::vector<std::vector<int>>(num_goods,std::vector<int>());
    purchase_produce_sell_bool = std::vector<bool>(num_goods,false);
    produce_for_sale = std::vector<int>();
    purchase_produce_sell = std::vector<int>();
    produce_for_sale_bool = std::vector<bool>(num_goods,false);

    hire_lobbyist = (action & 3) == 1;
    hire_marketing_manager = (action & 7) == 4;
    if( (action & 3) == 2) { //hire prod manager
        int fac_num = (action >> 2) & 7;
        assert (fac_num >= 0);
        hired_prod_managers = fac_num;
    }
    else if( (action & 3) == 3) { //build factory
        building = (action >> 2) & 7;
        assert (building >= 0);
    }

    action = action >> 5;
    for(int i = 0; i < 3; i++) { //at most 3 factories, if max factories 2 for example the third entry is just empty anyway
        int atype = action & 7;
        action = action >> 3;
        int g1 = action & 7;
        action = action >> 3;
        int g2 = action & 7;
        action = action >> 3;
        assert (g1 >= 0 && g2 >= 0 && atype >= 0);

        switch(atype) {
            case 1:
                produce_for_sale.push_back(g1);
                produce_for_sale_bool[g1] = true;
                break;
            case 2:
                purchase_produce_sell.push_back(g1);
                purchase_produce_sell_bool[g1] = true;
                break;
            case 3:
                sell[g1] = true;
                break;
            case 4:
                produces[g1].push_back(g2);
                break;
            case 5:
                purchase_produce[g1].push_back(g2);
                break;
            case 6:
                purchases[g1].push_back(g2);
                break;
            default:
                break;
        }
    }

}

std::vector<int> Model::getActions_(ABS::Gamestate* uncasted_state){
    auto* state = dynamic_cast<Gamestate*>(uncasted_state);

    //which goods can be produced?
    std::vector<bool> can_produce = std::vector<bool>(state->goods_in_stock.size(), true);
    std::vector<bool> prime_goods = std::vector<bool>(state->goods_in_stock.size(), true);
    for(size_t g1 = 0; g1 < state->goods_in_stock.size(); g1++) {
        for(size_t g2 = 0; g2 < state->goods_in_stock.size(); g2++) {
            if(processed_to[g1][g2] && !state->goods_in_stock[g1][g2]) {
                can_produce[g2] = false;
                prime_goods[g2] = false;
            }
        }
    }

    std::vector<int> factories;
    std::vector<std::vector<int>> good_actions; //for each good all available actions
    for(int i = 0; i < num_goods; i++) {
        std::vector<int> this_good_actions = {};

        if(state->good_in_stock_for_sale[i])
            this_good_actions.push_back(3 | (i << 3)); //sell

        for(size_t g = 0; g < state->goods_in_stock.size(); g++) {
            if(processed_to[i][g])
                this_good_actions.push_back(6 | (i << 3) | (g << 6)); //purchase
        }

        if(state->factories[i]) {
            factories.push_back(i);
            if(can_produce[i])
                this_good_actions.push_back(1 | (i << 3)); //produce for sale
            if(state->prod_managers[i])
                this_good_actions.push_back(2 | (i << 3)); //purchase produce sell

            for(size_t g = 0; g < state->goods_in_stock.size(); g++) {
                if(can_produce[i] && processed_to[i][g])
                    this_good_actions.push_back(4 | (i << 3) | (g << 6)); //produce
                if(state->prod_managers[i] && processed_to[i][g])
                    this_good_actions.push_back(5 | (i << 3) | (g << 6)); //purchase produce
            }

        }

        good_actions.push_back(this_good_actions);
    }

    std::vector<int> build_actions;
    if(static_cast<int>(factories.size()) < max_factories) {
        for(size_t i = 0; i < state->factories.size(); i++) {
            if(!state->factories[i] && state->construction_progress_factories[i] == 0 && !prime_goods[i]) {
                build_actions.push_back(i);
            }
        }
    }

    bool hire_marketing_manager = state->marketing_manager_applies;
    bool hire_lobbyist = state->lobbyist_applies;
    std::vector<int> hire_prod_manager_at = state->prod_manager_applies? factories : std::vector<int>();

    std::vector<int> actions = {}; //0 = idle action
    for(size_t active = 0; active <= factories.size(); active++) {
        std::vector<int> actions_with_active = {0};
        for(size_t i = 0; i < active; i++) {

            std::vector<int> tmp = {};
            for(int action : actions_with_active) {

                if(i == active-1) { //management action as last active action
                    if(hire_marketing_manager)
                        tmp.push_back((action << 5) | 4);
                    if(hire_lobbyist)
                        tmp.push_back((action << 5) | 1);

                    for(int at : hire_prod_manager_at)
                        tmp.push_back((action << 5) | 2 | (at << 2));
                    for(int build : build_actions)
                        tmp.push_back((action << 5) | 3 | (build << 2));
                }


                int last_good = i == 0? -1 : ((action >> 3) & 7); //factory action as last active
                for(int good = last_good+1; good < num_goods; good++) {
                    for(int good_action : good_actions[good]) {
                        tmp.push_back(((action << 9) | good_action) << (i == active-1? 5 : 0));
                    }
                }

            }
            actions_with_active = tmp;

        }

        //extend actions with actions_with_active
        for(int action : actions_with_active) {
            actions.push_back(action);
        }

    }

    //Coding bits
    //1-2 management code code (0 = hire marketing manager/idle, 1 = hire lobbyist, 2 = hire prod manager, 3 build factory) [hire marketng manager 1-2: 0, 3-4:1]
    //3-5 factory/good number
    //6-14: factory1 (1-3 action type, 4-6 good1, 7-9 good2), action type 0 = no action
    //15-23: factory2
    //24-32: factory3


    // for (int action : actions) {
    //     std::vector<std::vector<int>> purchases;
    //     std::vector<std::vector<int>> produces;
    //     std::vector<std::vector<int>> purchase_produce;
    //     std::vector<bool> purchase_produce_sell_bool;
    //     std::vector<int> produce_for_sale;
    //     std::vector<bool> produce_for_sale_bool;
    //     std::vector<int> purchase_produce_sell;
    //     std::vector<bool> sell;
    //     int building;
    //     int hired_prod_managers;
    //     bool hire_lobbyist;
    //     bool hire_marketing_manager;
    //     unpackAction(action, num_goods,purchases, produces, purchase_produce, purchase_produce_sell_bool, produce_for_sale, produce_for_sale_bool, purchase_produce_sell, sell, building, hired_prod_managers, hire_lobbyist, hire_marketing_manager);
    //     //print
    //     std::cout << "----------------" << std::endl;
    //     std::cout << action << std::endl;
    //     std::cout << "Management action: ";
    //     if(hire_marketing_manager)
    //         std::cout << "Hire marketing manager ";
    //     if(hire_lobbyist)
    //         std::cout << "Hire lobbyist ";
    //     if (hired_prod_managers != -1)
    //         std::cout << "Hire prod manager at " << hired_prod_managers << " ";
    //     if(building != -1)
    //         std::cout << "Build factory at " << building << " ";
    //     std::cout << std::endl;
    //     std::cout << "Goods actions:" << std::endl;
    //     for(int i = 0; i < num_goods; i++) {
    //         std::cout << "Good " << i << ": ";
    //         if(produce_for_sale_bool[i])
    //             std::cout << "Produce for sale ";
    //         if(sell[i])
    //             std::cout << "Sell ";
    //         for(int p : produces[i])
    //             std::cout << "Produce " << p << " ";
    //         for(int p : purchase_produce[i])
    //             std::cout << "Purchase produce " << p << " ";
    //         for(int p : purchases[i])
    //             std::cout << "Purchase " << p << " ";
    //         for(int p : purchase_produce_sell)
    //             std::cout << "Purchase produce sell " << p << " ";
    //         std::cout << std::endl;
    //     }
    // }

    return actions;
}

std::pair<std::vector<double>,double> Model::applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes){
    auto newState = dynamic_cast<Gamestate*>(uncasted_state);
    auto oldState = *newState;
    size_t decision_point = 0;

    std::vector<std::vector<int>> purchases;
    std::vector<std::vector<int>> produces;
    std::vector<std::vector<int>> purchase_produce;
    std::vector<bool> purchase_produce_sell_bool;
    std::vector<int> produce_for_sale;
    std::vector<bool> produce_for_sale_bool;
    std::vector<int> purchase_produce_sell;
    std::vector<bool> sell;
    int building;
    int hired_prod_managers;
    bool hire_lobbyist;
    bool hire_marketing_manager;
    unpackAction(action, num_goods,purchases, produces, purchase_produce, purchase_produce_sell_bool, produce_for_sale, produce_for_sale_bool, purchase_produce_sell, sell, building, hired_prod_managers, hire_lobbyist, hire_marketing_manager);

    double outcomeProb = 1.0;

    /*
     * Update Goods in stocks
     */
    for(int i = 0; i <num_goods; i++){
        for(int sellp : produce_for_sale)
            newState->goods_in_stock[i][sellp] = false;
        for(int sellp : purchase_produce_sell)
            newState->goods_in_stock[i][sellp] = false;
        for(size_t j = 0; j < produces.size(); j++) {
            if(!produces[j].empty()){
                newState->goods_in_stock[i][j] = false;
            }
        }

        for(int purchase: purchases[i])
            newState->goods_in_stock[i][purchase] = true;
        for(int produce: produces[i])
            newState->goods_in_stock[i][produce] = true;
        for(int purchase_produce_product: purchase_produce[i])
            newState->goods_in_stock[i][purchase_produce_product] = true;
    }
    /*
     *Update goods in stock for sale and price levels
     */
    for(int i = 0; i < num_goods; i++) {
        newState->good_in_stock_for_sale[i] = produce_for_sale_bool[i] | (oldState.good_in_stock_for_sale[i] & !sell[i]);

        //sample new price level
        auto probs = price_level_transition_prob[oldState.current_price_trends[i]][oldState.current_price_levels[i]];
        std::discrete_distribution<int> dist(probs.begin(),probs.end());
        if (decision_outcomes == nullptr)
         newState->current_price_levels[i] = dist(rng);
        else {
            std::vector<int> non_zero_levels;
            for(size_t j = 0; j < probs.size(); j++) {
                if(probs[j] > 0)
                    non_zero_levels.push_back(j);
            }
            size_t idx =getDecisionPoint(decision_point,0,non_zero_levels.size()-1,decision_outcomes);
            newState->current_price_levels[i] = non_zero_levels[idx];
        }
        outcomeProb *= probs[newState->current_price_levels[i]];

        //sample new price trend
        std::bernoulli_distribution change_dist(price_trend_change_probs[i]);
        if( (decision_outcomes == nullptr && change_dist(rng)) || (decision_outcomes != nullptr && (price_trend_change_probs[i] == 1 || (price_trend_change_probs[i] != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))) {
            if(oldState.current_price_trends[i] == 1) { //stable
                std::bernoulli_distribution change_dist_up(0.5);
                outcomeProb *= price_trend_change_probs[i] * 0.5;
                if((decision_outcomes == nullptr && change_dist_up(rng)) || (decision_outcomes != nullptr && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)) {
                    newState->current_price_trends[i] = 2;
                }else {
                    newState->current_price_trends[i] = 0;
                }
            }else if(oldState.current_price_trends[i] == 0) {
                newState->current_price_trends[i] = 1;
                outcomeProb *= price_trend_change_probs[i];
            }else if(oldState.current_price_trends[i] == 2) {
                newState->current_price_trends[i] = 1;
                outcomeProb *= price_trend_change_probs[i];
            }

        }else {
            outcomeProb *= 1 - price_trend_change_probs[i];
        }

    }


    /*
     * Update factories
     */
    for(int i = 0; i < num_goods; i++) {
        if(oldState.construction_progress_factories[i] == 4)
            newState->factories[i] = true;

        int construction_state = oldState.construction_progress_factories[i];
        if(i == building)
            newState->construction_progress_factories[i] = 1;
        if(construction_state >= 1 && construction_state < 4) {
            std::bernoulli_distribution dist(delay_probs[construction_state]);
            int delay = dist(rng);
            if( (decision_outcomes == nullptr && !delay) || (decision_outcomes != nullptr && (delay_probs[construction_state] == 1 || (delay_probs[construction_state] != 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))) {
                outcomeProb *= 1 - delay_probs[construction_state];
                newState->construction_progress_factories[i] = construction_state + 1;
            }else {
                outcomeProb *= delay_probs[construction_state];
            }
        } else if(construction_state == 4) {
            newState->construction_progress_factories[i] = 0;
        }
    }

    /*
     * Staff
     */
    if(hired_prod_managers != -1)
        newState->prod_managers[hired_prod_managers] = true;
    newState->has_lobbyist = hire_lobbyist | oldState.has_lobbyist;
    newState->has_marketing_manager = hire_marketing_manager | oldState.has_marketing_manager;

    std::bernoulli_distribution dist_prod_manager_apply(prod_manager_apply_prob);
    if( (decision_outcomes == nullptr && dist_prod_manager_apply(rng)) || (decision_outcomes != nullptr && (prod_manager_apply_prob == 1 || (prod_manager_apply_prob > 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))) {
        newState->prod_manager_applies = true;
        outcomeProb *= prod_manager_apply_prob;
    }else {
        newState->prod_manager_applies = false;
        outcomeProb *= 1 - prod_manager_apply_prob;
    }
    std::bernoulli_distribution dist_lobbyist_apply(lobbyist_apply_prob);
    if( (decision_outcomes == nullptr && dist_lobbyist_apply(rng)) || (decision_outcomes != nullptr && (lobbyist_apply_prob == 1 || (lobbyist_apply_prob > 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))) {
        newState->lobbyist_applies = true;
        outcomeProb *= lobbyist_apply_prob;
    }else {
        newState->lobbyist_applies = false;
        outcomeProb *= 1 - lobbyist_apply_prob;
    }
    std::bernoulli_distribution dist_marketing_manager_apply(marketing_manager_apply_prob);
    if( (decision_outcomes == nullptr && dist_marketing_manager_apply(rng)) || (decision_outcomes != nullptr && (marketing_manager_apply_prob == 1 || (marketing_manager_apply_prob > 0 && getDecisionPoint(decision_point,0,1,decision_outcomes) == 0)))) {
        newState->marketing_manager_applies = true;
        outcomeProb *= marketing_manager_apply_prob;
    }else {
        newState->marketing_manager_applies = false;
        outcomeProb *= 1 - marketing_manager_apply_prob;
    }

    /*
     *  Reward
     */
    double reward = 0.0;
    for(int i = 0; i < num_goods; i++) {
        double mm_factor = 1 + (oldState.has_marketing_manager ? marketing_manager_bonus : 0);
        double product_raw_price =  prices[i] * price_level_factors[oldState.current_price_levels[i]];
        reward += sell[i] ? product_raw_price* mm_factor  : 0;
        reward += purchase_produce_sell_bool[i] ? product_raw_price* mm_factor : 0;

        double lobby_factor = 1 - (oldState.has_lobbyist ? lobbyist_bonus : 0);
        reward -= product_raw_price * lobby_factor * (double)purchases[i].size();
        for(int p : purchase_produce_sell) {
            if(processed_to[i][p] && ! oldState.goods_in_stock[i][p])
                reward -= product_raw_price * lobby_factor;
        }

        int needed_goods = 0;
        for(size_t p = 0; p < oldState.factories.size(); p++) {
            if(processed_to[p][i] && !oldState.goods_in_stock[p][i])
                needed_goods++;
        }
        reward -= product_raw_price * lobby_factor * needed_goods;

        if(oldState.construction_progress_factories[i] != 0)
            reward -= build_factory_cost[i];

        reward -= oldState.prod_managers[i] ? salary_prod_manager : 0;
    }
    reward -= oldState.has_lobbyist ? salary_lobbyist : 0;
    reward -= oldState.has_marketing_manager ? salary_marketing_manager : 0;


    std::vector<double> rewards = { reward };

    return std::make_pair(rewards, outcomeProb);
}

