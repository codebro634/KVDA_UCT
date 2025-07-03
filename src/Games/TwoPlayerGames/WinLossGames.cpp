#include "../../../include/Games/TwoPlayerGames/WinLossGames.h"
#include <stdexcept>

namespace WLG
{

    Model::Model(bool zero_sum) : zero_sum(zero_sum) {}

    std::vector<double> Model::getRewards(GameResult result) const{

        if (zero_sum){
            switch (result){
                case GameResult::NOT_OVER:
                    return {0.0, 0.0};
                case GameResult::DRAW:
                    return {0.0, 0.0};
                case GameResult::P0_WIN:
                    return {1.0, -1.0};
                case GameResult::P1_WIN:
                    return {-1.0, 1.0};
            }
        }
        else{
            switch (result){
                case GameResult::NOT_OVER:
                    return {0.0, 0.0};
                case GameResult::DRAW:
                    return {0.0, 0.0};
                case GameResult::P0_WIN:
                    return {1.0, 0.0};
                case GameResult::P1_WIN:
                    return {0.0, 1.0};
            }
        }

        throw std::runtime_error("Invalid game result");
    }

}