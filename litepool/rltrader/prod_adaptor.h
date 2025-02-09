#pragma once
#include "base_adaptor.h"

namespace Simulator {
    class ProdAdaptor final : public BaseAdaptor {
    public:
        void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent);
        void reset(const double& positionAmount, const double& averagePrice);
        bool next();
        std::unordered_map<std::string, double> getInfo();
        std::vector<double> getState();
        [[nodiscard]] long long getTime() const override;
    };
}