#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace RLTrader {
class BaseAdaptor {
public:
    virtual ~BaseAdaptor() = default;
    virtual void quote(int buy_spread, int sell_spread, int buy_percent, int sell_percent) = 0;
    virtual void reset() = 0;
    virtual bool next() = 0;
    virtual std::unordered_map<std::string, double> getInfo() = 0;
    virtual std::vector<double> getState() = 0;
};
}
