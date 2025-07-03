#ifndef MANFACTURER_H
#define MANFACTURER_H

#pragma once

#include "../Gamestate.h"
#include <string>
#include <vector>
#include <random>
#include <set>

namespace MAN {

    struct Gamestate : public ABS::Gamestate {
        std::vector<std::vector<bool>> goods_in_stock;
        std::vector<bool> factories;
        std::vector<int> construction_progress_factories;
        std::vector<bool> good_in_stock_for_sale;
        std::vector<int> current_price_levels;
        std::vector<int> current_price_trends;
        std::vector<bool> prod_managers;
        bool has_lobbyist, has_marketing_manager;

        bool prod_manager_applies, lobbyist_applies, marketing_manager_applies;

        bool operator==(const ABS::Gamestate& other) const override;
        [[nodiscard]] size_t hash() const override;
    };

    class Model : public ABS::Model {

    private:
        std::vector<std::vector<std::vector<double>>> price_level_transition_prob; //trend, from_lvl, to_lvl
        std::vector<double> price_trend_change_probs;
        std::vector<double> delay_probs;
        std::vector<double> prices;
        std::vector<double> price_level_factors;
        double marketing_manager_bonus{}, lobbyist_bonus{};
        std::vector<std::vector<bool>> processed_to;
        std::vector<double> build_factory_cost;
        int max_factories{};
        int num_goods{};
        double salary_prod_manager{}, salary_lobbyist{}, salary_marketing_manager{};
        double prod_manager_apply_prob{}, lobbyist_apply_prob{}, marketing_manager_apply_prob{};

        std::vector<bool> init_factories;
        std::vector<int> init_trends;

    protected:
        std::pair<std::vector<double>,double> applyAction_(ABS::Gamestate* uncasted_state, int action, std::mt19937& rng, std::vector<std::pair<int,int>>* decision_outcomes) override;
        std::vector<int> getActions_(ABS::Gamestate* uncasted_state) override;

    public:
        ~Model() override = default;
        explicit Model(const std::string& filePath);
        void printState(ABS::Gamestate* uncasted_state) override;
        ABS::Gamestate* getInitialState(std::mt19937& rng) override;
        int getNumPlayers() override;
        bool hasTransitionProbs() override {return true;}
        ABS::Gamestate* copyState(ABS::Gamestate* uncasted_state) override;
    };

} // namespace ABS

#endif //MANFACTURER_H
