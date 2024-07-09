#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <algorithm>
#include <map>
#include <string>
#include <random>
#include <chrono>
#include "inverse_instrument.h"
#include "csv_reader.h"
#include "position.h"
#include "exchange.h"
#include "strategy.h"
#include "orderbook.h"
#include "signal_builder.h"
#include "circ_buffer.h"

using namespace Simulator;
using namespace doctest;

TEST_CASE("Testing TemporalTable") {
    constexpr u_int rows = 3;
    constexpr u_int cols = 4;
    TemporalTable table(rows, cols);

    SUBCASE("Initial state") {
        for (u_int i = 0; i < rows; ++i) {
            const auto& row = table.get(i);
            CHECK(row.size() == cols);
            CHECK(std::all_of(row.begin(), row.end(), [](double val) { return val == 0.0; }));
        }
    }

    SUBCASE("Adding and retrieving rows") {
        std::vector<double> row1 = {1.1, 1.2, 1.3, 1.4};
        std::vector<double> row2 = {2.1, 2.2, 2.3, 2.4};
        std::vector<double> row3 = {3.1, 3.2, 3.3, 3.4};

        table.addRow(row1);
		auto result = table.get(0);
        CHECK(std::equal(table.get(0).begin(), table.get(0).end(), row1.begin()));

        table.addRow(row2);
        CHECK(std::equal(table.get(0).begin(), table.get(0).end(), row2.begin()));
        CHECK(std::equal(table.get(1).begin(), table.get(1).end(), row1.begin()));

        table.addRow(row3);
        CHECK(std::equal(table.get(0).begin(), table.get(0).end(), row3.begin()));
        CHECK(std::equal(table.get(1).begin(), table.get(1).end(), row2.begin()));
        CHECK(std::equal(table.get(2).begin(), table.get(2).end(), row1.begin()));
    }

    SUBCASE("Overwriting old rows") {
        std::vector<double> row1 = {1.1, 1.2, 1.3, 1.4};
        std::vector<double> row2 = {2.1, 2.2, 2.3, 2.4};
        std::vector<double> row3 = {3.1, 3.2, 3.3, 3.4};
        std::vector<double> row4 = {4.1, 4.2, 4.3, 4.4};

        table.addRow(row1);
        table.addRow(row2);
        table.addRow(row3);
        table.addRow(row4);

        CHECK(std::equal(table.get(0).begin(), table.get(0).end(), row4.begin()));
        CHECK(std::equal(table.get(1).begin(), table.get(1).end(), row3.begin()));
        CHECK(std::equal(table.get(2).begin(), table.get(2).end(), row2.begin()));
    }

    SUBCASE("Adding multiple rows in a loop") {
        for (u_int i = 0; i < 10; ++i) {
            std::vector<double> row(cols, static_cast<double>(i));
            table.addRow(row);
            CHECK(std::equal(table.get(0).begin(), table.get(0).end(), row.begin()));
            if (i > 0) {
                std::vector<double> prev_row(cols, static_cast<double>(i - 1));
                CHECK(std::equal(table.get(1).begin(), table.get(1).end(), prev_row.begin()));
            }
            if (i > 1) {
                std::vector<double> prev_prev_row(cols, static_cast<double>(i - 2));
                CHECK(std::equal(table.get(2).begin(), table.get(2).end(), prev_prev_row.begin()));
            }
        }
    }
}

struct TestData {
    int value;
    bool operator==(const TestData& other) const { return value == other.value; }
};

TEST_CASE("Testing TemporalBuffer with custom class TestData") {
    Simulator::TemporalBuffer<TestData> buffer(2); // Buffer for 2 lags

    SUBCASE("Initial state") {
        CHECK_NOTHROW(buffer.get(0));
        CHECK_NOTHROW(buffer.get(1));
        CHECK_NOTHROW(buffer.get(2));
    }

    SUBCASE("Adding and retrieving custom objects") {
        buffer.add(TestData{1});
        CHECK(buffer.get(0) == TestData{1});

        buffer.add(TestData{2});
        CHECK(buffer.get(0) == TestData{2});
        CHECK(buffer.get(1) == TestData{1});

        buffer.add(TestData{3});
        CHECK(buffer.get(0) == TestData{3});
        CHECK(buffer.get(1) == TestData{2});
        CHECK(buffer.get(2) == TestData{1});
    }

    SUBCASE("Overwriting old values with custom objects") {
        buffer.add(TestData{4});
        buffer.add(TestData{5});
        buffer.add(TestData{6});
        CHECK(buffer.get(0) == TestData{6});
        CHECK(buffer.get(1) == TestData{5});
        CHECK(buffer.get(2) == TestData{4});
    }

    SUBCASE("Out of range access with custom objects") {
        CHECK_THROWS_AS(buffer.get(3), std::out_of_range);
        CHECK_THROWS_AS(buffer.get(-1), std::out_of_range);
    }

    SUBCASE("Adding multiple custom objects in a loop") {
        for (int i = 1; i <= 1000; ++i) {
            buffer.add(TestData{i});
            CHECK(buffer.get(0) == TestData{i});
            if (i > 1) {
                CHECK(buffer.get(1) == TestData{i - 1});
            }
            if (i > 2) {
                CHECK(buffer.get(2) == TestData{i - 2});
            }
        }
    }
}

TEST_CASE("test of orderbook") {
	double bid_price = 1000;
	double ask_price = 1000;
	std::map<std::string, double> lob;
	std::mt19937 rng;
	std::random_device rd;
	rng.seed(rd());
	std::uniform_int_distribution<int> dist(1000, 50000);

	for(int ii=0; ii < 20; ++ii) {
		std::string bid_lbl = "bids[";
		bid_lbl += std::to_string(ii) + "]";
		std::string ask_lbl = "asks[";
		ask_lbl += std::to_string(ii) + "]";
		bid_price -= 0.5;
		ask_price += 0.5;
		lob[bid_lbl + ".price"] = bid_price;
		lob[ask_lbl + ".price"] = ask_price;
		lob[bid_lbl + ".amount"] = dist(rng);
		lob[ask_lbl + ".amount"] = dist(rng);
	}
	Orderbook book(lob);
	CHECK(book.ask_prices.size() == 20);
	CHECK(book.bid_prices.size() == 20);
	CHECK(book.ask_sizes.size() == 20);
	CHECK(book.bid_sizes.size() == 20);
	SignalBuilder builder;
	std::vector<std::chrono::duration<double>> durations;

	int ii = 0;
	for (ii=0; ii < 15000; ++ii) {
		std::map<std::string, double> lob;
		double mid_price = 0.5 * (bid_price + ask_price) + dist(rng) / 2000;
		double bid_price = mid_price;
		double ask_price = mid_price;

		for(int jj=0; jj < 20; ++jj) {
			std::string bid_lbl = "bids[";
			bid_lbl += std::to_string(jj) + "]";
			std::string ask_lbl = "asks[";
			ask_lbl += std::to_string(jj) + "]";
			bid_price -= 0.5;
			ask_price += 0.5;
			lob[bid_lbl + ".price"] = bid_price;
			lob[ask_lbl + ".price"] = ask_price;
			lob[bid_lbl + ".amount"] = dist(rng);
			lob[ask_lbl + ".amount"] = dist(rng);
		}

		Orderbook book(lob);
		auto signals = builder.add_book(book);
		if (builder.is_data_ready()) {
			CHECK(std::all_of(signals.begin(), signals.end(), [](double val) {return std::isfinite(val);}));
			CHECK(std::all_of(signals.begin(), signals.end(), [](double val) { return std::abs(val) > 0;}));
			CHECK(std::all_of(signals.begin(), signals.end(), [](double val) { return std::abs(val) < 10;}));
		}
	}

	CHECK(ii == 15000);
}

TEST_CASE("testing the inverse_instrument") {
	InverseInstrument instr("BTC", 0.5, 10.0, 0.0, 0.0005);
	CHECK(instr.getTickSize() == Approx(0.5));
	CHECK(instr.getName() == "BTC");
	CHECK(instr.getMinAmount() == Approx(10.0));
	CHECK(instr.getQtyFromNotional(10000.0, 100.0) == Approx(0.01));
	CHECK(instr.pnl(1000, 10000.0, 20000.0) == Approx(0.05));
	CHECK(instr.fees(1000, 10000.0, true) == Approx(0.0));
	CHECK(instr.fees(1000, 10000.0, false) == Approx(0.00005));
}

TEST_CASE("testing the csv reader") {
	CsvReader reader("test.csv");
	CHECK(reader.getTimeStamp() == 1704067200170770);
	CHECK(reader.getDouble("bids[0].price") == Approx(42301.5));
	CHECK(reader.getDouble("bids[1].price") == Approx(42301.0));
	auto& obs = reader.current();
	CHECK(obs.data.at("bids[0].price") == Approx(42301.5));
	CHECK(obs.data.at("bids[1].price") == Approx(42301.0));
	CHECK(reader.hasNext());
	auto& next = reader.next();
	CHECK(next.id == 1704067200170778);
	CHECK(reader.getTimeStamp() == 1704067200170778);
	CHECK(next.data.at("bids[0].price") == Approx(42301.5));
	CHECK(next.data.at("bids[1].price") == Approx(42301.0));
	CHECK(reader.getDouble("bids[0].price") == Approx(42301.5));
	CHECK(reader.getDouble("bids[1].price") == Approx(42301.0));
	auto& curr = reader.current();
	CHECK(curr.id == 1704067200170778);
	CHECK(curr.data.at("bids[0].price") == Approx(42301.5));
	CHECK(curr.data.at("bids[1].price") == Approx(42301.0));
	reader.reset(0);
	CHECK(reader.getTimeStamp() == 1704067200170770);
}

TEST_CASE("testing the position") {
	InverseInstrument instr("BTC", 0.5, 10.0, 0.0, 0.0005);
	Position pos(instr, 0.1, 0, 0.0);

	SUBCASE("initial position") {
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1000, 1001);
		CHECK(info.averagePrice == Approx(0.0));
		CHECK(info.balance == Approx(0.1));
		CHECK(info.inventoryPnL == Approx(0.0));
		CHECK(info.leverage == Approx(0.0));
		CHECK(info.tradeCount == 0);
		CHECK(info.tradingPnL == Approx(0.0));
	}

	SUBCASE("first buy order") {
		Order order;
		order.amount = 10.0;
		order.microSecond = 1;
		order.orderId = 1;
		order.price = 1000.0;
		order.side = OrderSide::BUY;
		order.state = OrderState::FILLED;
		pos.onFill(order, true);
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1010, 1020);
		CHECK(info.averagePrice == Approx(1000.0));
		CHECK(info.balance == Approx(0.1));
		CHECK(info.inventoryPnL == Approx(0.000147783));
		CHECK(info.leverage == Approx(0.09900990099));
		CHECK(info.tradeCount == 1);
		CHECK(info.tradingPnL == Approx(0.0));
	}

	SUBCASE("Three buys and a smaller sell order") {
		for (int ii = 1; ii <= 3; ++ii) {
			Order order;
			order.amount = 10.0;
			order.microSecond = 1;
			order.orderId = 1;
			order.price = 1000.0;
			order.side = OrderSide::BUY;
			order.state = OrderState::FILLED;
			pos.onFill(order, true);
			CHECK(pos.getInitialBalance() == Approx(0.1));
			PositionInfo info;
			pos.fetchInfo(info, 1010, 1020);
			CHECK(info.averagePrice == Approx(1000.0));
			CHECK(info.balance == Approx(0.1));
			CHECK(info.inventoryPnL == Approx(0.000147783 * ii));
			CHECK(info.tradeCount == ii);
			CHECK(info.tradingPnL == Approx(0.0));
		}

		Order order;
		order.amount = 15.0;
		order.microSecond = 1;
		order.orderId = 1;
		order.price = 1015.0;
		order.side = OrderSide::SELL;
		order.state = OrderState::FILLED;
		pos.onFill(order, true);
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1010, 1020);
		CHECK(info.averagePrice == Approx(1000.0));
		CHECK(info.balance == Approx(0.1 + 0.00022167487684729079));
		CHECK(info.inventoryPnL == Approx(0.000147783 * 1.5));
		CHECK(info.leverage == Approx(0.14818635955510023));
		CHECK(info.tradeCount == 4);
		CHECK(info.tradingPnL == Approx(0.00022167487684729079));
	}

	SUBCASE("Three sells and a smaller buy order") {
		for (int ii = 1; ii <= 3; ++ii) {
			Order order;
			order.amount = 10.0;
			order.microSecond = 1;
			order.orderId = 1;
			order.price = 1000.0;
			order.side = OrderSide::SELL;
			order.state = OrderState::FILLED;
			pos.onFill(order, true);
			CHECK(pos.getInitialBalance() == Approx(0.1));
			PositionInfo info;
			pos.fetchInfo(info, 1010, 1020);
			CHECK(info.averagePrice == Approx(1000.0));
			CHECK(info.balance == Approx(0.1));
			CHECK(info.inventoryPnL == Approx(-0.000147783 * ii));
			CHECK(info.tradeCount == ii);
			CHECK(info.tradingPnL == Approx(0.0));
		}

		Order order;
		order.amount = 15.0;
		order.microSecond = 1;
		order.orderId = 1;
		order.price = 1015.0;
		order.side = OrderSide::BUY;
		order.state = OrderState::FILLED;
		pos.onFill(order, true);
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1010, 1020);
		CHECK(info.averagePrice == Approx(1000.0));
		CHECK(info.balance == Approx(0.1 - 0.000221675));
		CHECK(info.inventoryPnL == Approx(-0.000147783 * 1.5));
		CHECK(info.leverage == Approx(0.14738554024423889));
		CHECK(info.tradeCount == 4);
		CHECK(info.tradingPnL == Approx(-0.000221675));
	}

	SUBCASE("Equal buy and sell order") {
		{
			Order order;
			order.amount = 10.0;
			order.microSecond = 1;
			order.orderId = 1;
			order.price = 1000.0;
			order.side = OrderSide::BUY;
			order.state = OrderState::FILLED;
			pos.onFill(order, true);
			CHECK(pos.getInitialBalance() == Approx(0.1));
			PositionInfo info;
			pos.fetchInfo(info, 1010, 1020);
			CHECK(info.averagePrice == Approx(1000.0));
			CHECK(info.balance == Approx(0.1));
			CHECK(info.inventoryPnL == Approx(0.000147783));
			CHECK(info.leverage == Approx(0.099009900990099));
			CHECK(info.tradeCount == 1);
			CHECK(info.tradingPnL == Approx(0.0));
		}

		Order order;
		order.amount = 10.0;
		order.microSecond = 1;
		order.orderId = 1;
		order.price = 1015.0;
		order.side = OrderSide::SELL;
		order.state = OrderState::FILLED;
		pos.onFill(order, true);
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1010, 1020);
		CHECK(info.averagePrice == Approx(0.0));
		CHECK(info.balance == Approx(0.1 + 0.000147783));
		CHECK(info.inventoryPnL == Approx(0));
		CHECK(info.leverage == Approx(0));
		CHECK(info.tradeCount == 2);
		CHECK(info.tradingPnL == Approx(0.000147783));
	}

	SUBCASE("Equal sell and buy order") {
		{
			Order order;
			order.amount = 10.0;
			order.microSecond = 1;
			order.orderId = 1;
			order.price = 1015.0;
			order.side = OrderSide::SELL;
			order.state = OrderState::FILLED;
			pos.onFill(order, true);
			CHECK(pos.getInitialBalance() == Approx(0.1));
			PositionInfo info;
			pos.fetchInfo(info, 1010, 1020);
			CHECK(info.averagePrice == Approx(1015.0));
			CHECK(info.balance == Approx(0.1));
			CHECK(info.inventoryPnL == Approx(0));
			CHECK(info.leverage == Approx(0.098039215));
			CHECK(info.tradeCount == 1);
			CHECK(info.tradingPnL == Approx(0.0));
		}

		Order order;
		order.amount = 10.0;
		order.microSecond = 1;
		order.orderId = 1;
		order.price = 1000.0;
		order.side = OrderSide::BUY;
		order.state = OrderState::FILLED;
		pos.onFill(order, true);
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1010, 1020);
		CHECK(info.averagePrice == Approx(0.0));
		CHECK(info.balance == Approx(0.1 + 0.000147783));
		CHECK(info.inventoryPnL == Approx(0));
		CHECK(info.leverage == Approx(0));
		CHECK(info.tradeCount == 2);
		CHECK(info.tradingPnL == Approx(0.000147783));
	}

	SUBCASE("Buy more than initial sell order") {
		{
			Order order;
			order.amount = 10.0;
			order.microSecond = 1;
			order.orderId = 1;
			order.price = 1015.0;
			order.side = OrderSide::SELL;
			order.state = OrderState::FILLED;
			pos.onFill(order, true);
			CHECK(pos.getInitialBalance() == Approx(0.1));
			PositionInfo info;
			pos.fetchInfo(info, 1010, 1020);
			CHECK(info.averagePrice == Approx(1015.0));
			CHECK(info.balance == Approx(0.1));
			CHECK(info.inventoryPnL == Approx(0));
			CHECK(info.leverage == Approx(0.098039215));
			CHECK(info.tradeCount == 1);
			CHECK(info.tradingPnL == Approx(0.0));
		}

		Order order;
		order.amount = 20.0;
		order.microSecond = 1;
		order.orderId = 1;
		order.price = 1000.0;
		order.side = OrderSide::BUY;
		order.state = OrderState::FILLED;
		pos.onFill(order, true);
		CHECK(pos.getInitialBalance() == Approx(0.1));
		PositionInfo info;
		pos.fetchInfo(info, 1010, 1020);
		CHECK(info.averagePrice == Approx(1000.0));
		CHECK(info.balance == Approx(0.1 + 0.000147783));
		CHECK(info.inventoryPnL == Approx(0.0001477832));
		CHECK(info.leverage == Approx(0.0988637968));
		CHECK(info.tradeCount == 2);
		CHECK(info.tradingPnL == Approx(0.000147783));
	}
}

TEST_CASE("testing exchange") {
	CsvReader reader("test.csv");
	Exchange exch(reader, 5); // 10 microsecond delay is not practical in reality
	const auto& row = exch.getObs();
	CHECK(row.id == 1704067200170770);
	CHECK(row.data.at("bids[0].price") == Approx(42301.5));
	CHECK(row.data.at("bids[1].price") == Approx(42301.0));

	for (int ii = 0; ii < 8; ++ii)
		CHECK(exch.next());

	CHECK(!exch.next());
	exch.reset(1);
	auto next = exch.getObs();
	CHECK(next.id == 1704067200170778);
	CHECK(next.data.at("bids[0].price") == Approx(42301.5));
	CHECK(next.data.at("bids[1].price") == Approx(42301.0));
	exch.reset(0);
	exch.quote(1, OrderSide::SELL, 42302, 100);
	exch.quote(2, OrderSide::SELL, 42305, 500);
	exch.quote(3, OrderSide::BUY, 40000, 300);
	exch.quote(4, OrderSide::BUY, 39000, 200);
	std::vector<Order> unacks = exch.getUnackedOrders();
	CHECK(unacks.size() == 4);
	exch.next();
	const auto& bids = exch.getBidOrders();
	CHECK(bids.size() == 2);
	const auto& asks = exch.getAskOrders();
	CHECK(asks.size() == 2);
	unacks = exch.getUnackedOrders();
	CHECK(unacks.size() == 0);
	exch.next();
	exch.next();
	auto fills = exch.getFills();
	CHECK(fills.size() == 1);
	auto filled = fills[0];
	CHECK(filled.orderId == 1);
	CHECK(filled.amount == Approx(100.0));
	CHECK(filled.price == Approx(42302));
	const auto& bidsTwo = exch.getBidOrders();
	CHECK(bidsTwo.size() == 2);
	const auto& asksTwo = exch.getAskOrders();
	CHECK(asksTwo.size() == 1);
	exch.cancelBuys();
	exch.next();
	exch.cancelSells();
	const auto& bidsThree = exch.getBidOrders();
	const auto& asksThree = exch.getAskOrders();
	CHECK(bidsThree.size() == 0);
	CHECK(asksThree.size() == 1);
	fills = exch.getFills();
	CHECK(fills.size() == 0);
	exch.next();
	const auto& asksFour = exch.getAskOrders();
	CHECK(asksFour.size() == 0);
	CHECK(exch.getUnackedOrders().size() == 0);
}

TEST_CASE("test of strategy") {
	CsvReader reader("test.csv");
	Exchange exch(reader, 5);
	InverseInstrument instr("BTC", 0.5, 10.0, 0, 0.0005);
	Strategy strategy(instr, exch, 1, 0, 0, 30.0, 5);
	strategy.quote(0, 0, 30, 85);
	const auto& bids = exch.getBidOrders();
	const auto& asks = exch.getAskOrders();
	CHECK(bids.size() == 7);
	CHECK(asks.size() == 1);
}
