#pragma once

#ifndef WINLOSSGAMES_H
#define WINLOSSGAMES_H
#include <vector>

namespace WLG
{

    enum class GameResult {
        NOT_OVER,
        DRAW,
        P0_WIN,
        P1_WIN
    };

    class Model
    {
    public:
        virtual ~Model() = default;
        explicit Model(bool zero_sum);
        [[nodiscard]] std::vector<double> getRewards(GameResult result) const;

    private:
        bool zero_sum;

    };

}


#endif //WINLOSSGAMES_H