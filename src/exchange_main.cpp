#include <iostream>
#include <vector>
#include <span>

#include "core/clock.h"
#include "core/types.h"
#include "matching/matching_engine.h"
#include "matching/events.h"

using namespace exchange;

void print_events(std::span<const matching::Event> events) {
    for (const auto& event : events) {
        if (event.type == matching::EventType::ORDER_ACCEPTED) {
            std::cout << "[Event] Order Accepted: ID " << event.payload.order_accepted.order_id << "\n";
        } else if (event.type == matching::EventType::ORDER_RESTED) {
            std::cout << "[Event] Order Rested in Book: ID " << event.payload.order_rested.order_id << "\n";
        } else if (event.type == matching::EventType::TRADE) {
            std::cout << "[Event] *** TRADE EXECUTED ***\n"
                      << "        Price: " << event.payload.trade.price / core::PRICE_SCALE << "\n"
                      << "        Qty:   " << event.payload.trade.qty << "\n"
                      << "        Buyer: " << event.payload.trade.buy_order_id << "\n"
                      << "        Seller:" << event.payload.trade.sell_order_id << "\n";
        }
    }
}

int main() {
    std::cout << "Starting LOB (Limit Order Book) Matching Engine...\n\n";

    matching::MatchingEngine<> engine;
    
    // Initialize the book for BTCUSD
    core::Symbol btc = core::make_symbol("BTCUSD");
    std::vector<core::Symbol> symbols = { btc };
    engine.prepare_symbols(symbols);

    // 1. Submit a Buy Order (10 BTC @ $50,000)
    matching::InboundOrder buy_order{
        .action = matching::RequestAction::ADD,
        .symbol = btc,
        .order_id = 1,
        .side = core::Side::BUY,
        .price = 50000 * core::PRICE_SCALE,
        .qty = 10,
        .order_type = core::OrderType::LIMIT,
        .timestamp = core::now_ns(),
        .participant_id = 101,
        .display_qty = 10
    };
    
    std::cout << "-> Submitting BUY 10 BTC @ $50,000\n";
    auto buy_events = engine.process_order(buy_order);
    print_events(buy_events);
    std::cout << "\n";

    // 2. Submit a Sell Order that crosses the spread (5 BTC @ $49,000)
    matching::InboundOrder sell_order{
        .action = matching::RequestAction::ADD,
        .symbol = btc,
        .order_id = 2,
        .side = core::Side::SELL,
        .price = 49000 * core::PRICE_SCALE,
        .qty = 5,
        .order_type = core::OrderType::LIMIT,
        .timestamp = core::now_ns(),
        .participant_id = 102,
        .display_qty = 5
    };

    std::cout << "-> Submitting SELL 5 BTC @ $49,000 (Marketable)\n";
    auto sell_events = engine.process_order(sell_order);
    print_events(sell_events);
    std::cout << "\n";

    std::cout << "Engine test complete.\n";
    return 0;
}