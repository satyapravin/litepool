#include <iostream>
#include <fstream>
#include <string>
#include <csv_reader.h>
#include <orderbook.h>
#include <market_signal_builder.h>

using namespace RLTrader;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "please provide input and output file names" << std::endl;
    }

    std::string filename = argv[1];
    std::string output = argv[2];
    std::ofstream ofile(output);
    RLTrader::CsvReader reader(filename, 1, 36000);
    MarketSignalBuilder builder(5);
    bool header = false;

    while(reader.hasNext()) {
        auto row = reader.next();
        Orderbook book(row.data);
        auto signals = builder.add_book(book);
        if (!header) {
            header= true;
            ofile << "timestamp,bid_price,ask_price,";
            for(auto ii=0; ii < signals.size(); ++ii) {
                ofile << "signal_" << ii << ",";
            }

            ofile << std::endl;
        }

        ofile << row.id << "," << row.getBestBidPrice() << "," << row.getBestAskPrice() << ",";
        for(double signal : signals) {
            ofile << signal << ",";
        }

        ofile << std::endl;
    }

    return 0;
}
