#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "inverse_instrument.h"
#include "csv_reader.h"
#include "position.h"
#include "exchange.h"
#include "strategy.h"

using namespace Simulator;
using namespace doctest;

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
	const auto& bids = exch.getBids();
	CHECK(bids.size() == 2);
	const auto& asks = exch.getAsks();
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
	const auto& bidsTwo = exch.getBids();
	CHECK(bidsTwo.size() == 2);
	const auto& asksTwo = exch.getAsks();
	CHECK(asksTwo.size() == 1);
	exch.cancelBuys();
	exch.next();
	exch.cancelSells();
	const auto& bidsThree = exch.getBids();
	const auto& asksThree = exch.getAsks();
	CHECK(bidsThree.size() == 0);
	CHECK(asksThree.size() == 1);
	fills = exch.getFills();
	CHECK(fills.size() == 0);
	exch.next();
	const auto& asksFour = exch.getAsks();
	CHECK(asksFour.size() == 0);
	CHECK(exch.getUnackedOrders().size() == 0);
}

TEST_CASE("test of strategy") {
	CsvReader reader("test.csv");
	Exchange exch(reader, 5);
	InverseInstrument instr("BTC", 0.5, 10.0, 0, 0.0005);
	Strategy strategy(instr, exch, 1, 0, 0, 30.0, 5);
	strategy.quote(0, 0, 30, 85);
	const auto& bids = exch.getBids();
	const auto& asks = exch.getAsks();
	CHECK(bids.size() == 7);
	CHECK(asks.size() == 1);
}
