#include "position_signal_builder.h"
#include "norm_macro.h"

using namespace Simulator;

PositionSignalBuilder::PositionSignalBuilder()
                      :raw_signals(10)
                      ,mean_raw_signals(std::make_unique<position_signal_repository>())
                      ,ssr_raw_signals(std::make_unique<position_signal_repository>())
                      ,norm_raw_signals(std::make_unique<position_signal_repository>())
                      ,velocity_10_signals(std::make_unique<position_signal_repository>())
                      ,mean_velocity_10_signals(std::make_unique<position_signal_repository>())
                      ,ssr_velocity_10_signals(std::make_unique<position_signal_repository>())
                      ,volatility_10_signals(std::make_unique<position_signal_repository>())
                      ,mean_volatility_10_signals(std::make_unique<position_signal_repository>())
                      ,ssr_volatility_10_signals(std::make_unique<position_signal_repository>())
                      ,norm_volatility_10_signals(std::make_unique<position_signal_repository>()) {
}

std::vector<double> PositionSignalBuilder::add_info(PositionInfo& info) {

}

position_signal_repository& PositionSignalBuilder::get_position_signals(signal_type sigtype) {

}

position_signal_repository& PositionSignalBuilder::get_velocity_signals(signal_type sigtype) {

}

position_signal_repository& PositionSignalBuilder::get_volatility_signals(signal_type sigtype) {

}


