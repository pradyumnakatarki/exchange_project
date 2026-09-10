#include <benchmark/benchmark.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "core/clock.h"
#include "core/types.h"
#include "matching/matching_engine.h"
#include "matching/naive_matching_engine.h"

using namespace exchange;

// Number of operations per repetition (sized safely within 1,000,000 capacity)
constexpr std::size_t BENCHMARK_OPERATIONS = 200'000;
constexpr int BENCHMARK_REPETITIONS = 10;

// ============================================================================
// Scenario 1: Passive Order Add (No Crossing)
// ============================================================================

static void BM_PassiveOrderAdd_Optimized(benchmark::State& state) {
    matching::MatchingEngine<> engine;
    core::Symbol btc = core::make_symbol("BTCUSD");
    std::vector<core::Symbol> symbols = {btc};
    engine.prepare_symbols(symbols);

    core::OrderId id = 1;
    for (auto _ : state) {
        matching::InboundOrder order{
            .action = matching::RequestAction::ADD,
            .symbol = btc,
            .order_id = id++,
            .side = core::Side::BUY,
            .price = 50000 * core::PRICE_SCALE,
            .qty = 10,
            .order_type = core::OrderType::LIMIT,
            .timestamp = core::now_ns(),
            .participant_id = 101,
            .display_qty = 10
        };
        benchmark::DoNotOptimize(engine.process_order(order));
    }
}
BENCHMARK(BM_PassiveOrderAdd_Optimized)
    ->Iterations(BENCHMARK_OPERATIONS)
    ->Repetitions(BENCHMARK_REPETITIONS);

static void BM_PassiveOrderAdd_Naive(benchmark::State& state) {
    naive::MatchingEngine<> engine;
    core::Symbol btc = core::make_symbol("BTCUSD");
    std::vector<core::Symbol> symbols = {btc};
    engine.prepare_symbols(symbols);

    core::OrderId id = 1;
    for (auto _ : state) {
        matching::InboundOrder order{
            .action = matching::RequestAction::ADD,
            .symbol = btc,
            .order_id = id++,
            .side = core::Side::BUY,
            .price = 50000 * core::PRICE_SCALE,
            .qty = 10,
            .order_type = core::OrderType::LIMIT,
            .timestamp = core::now_ns(),
            .participant_id = 101,
            .display_qty = 10
        };
        benchmark::DoNotOptimize(engine.process_order(order));
    }
}
BENCHMARK(BM_PassiveOrderAdd_Naive)
    ->Iterations(BENCHMARK_OPERATIONS)
    ->Repetitions(BENCHMARK_REPETITIONS);

// ============================================================================
// Scenario 2: Aggressive Market Order Match (Immediate Execution)
// ============================================================================

static void BM_AggressiveMarketOrderMatch_Optimized(benchmark::State& state) {
    matching::MatchingEngine<> engine;
    core::Symbol btc = core::make_symbol("BTCUSD");
    std::vector<core::Symbol> symbols = {btc};
    engine.prepare_symbols(symbols);

    // Pre-fill the book with a massive sell wall so liquidity is never exhausted
    matching::InboundOrder sell_wall{
        .action = matching::RequestAction::ADD,
        .symbol = btc,
        .order_id = 9999999,
        .side = core::Side::SELL,
        .price = 50000 * core::PRICE_SCALE,
        .qty = 100000000,
        .order_type = core::OrderType::LIMIT,
        .timestamp = core::now_ns(),
        .participant_id = 101,
        .display_qty = 100000000
    };
    engine.process_order(sell_wall);

    core::OrderId buy_id = 1;
    for (auto _ : state) {
        matching::InboundOrder aggressive_buy{
            .action = matching::RequestAction::ADD,
            .symbol = btc,
            .order_id = buy_id++,
            .side = core::Side::BUY,
            .price = 50000 * core::PRICE_SCALE,
            .qty = 1,
            .order_type = core::OrderType::MARKET,
            .timestamp = core::now_ns(),
            .participant_id = 102,
            .display_qty = 1
        };
        benchmark::DoNotOptimize(engine.process_order(aggressive_buy));
    }
}
BENCHMARK(BM_AggressiveMarketOrderMatch_Optimized)
    ->Iterations(BENCHMARK_OPERATIONS)
    ->Repetitions(BENCHMARK_REPETITIONS);

static void BM_AggressiveMarketOrderMatch_Naive(benchmark::State& state) {
    naive::MatchingEngine<> engine;
    core::Symbol btc = core::make_symbol("BTCUSD");
    std::vector<core::Symbol> symbols = {btc};
    engine.prepare_symbols(symbols);

    // Pre-fill the book with a massive sell wall so liquidity is never exhausted
    matching::InboundOrder sell_wall{
        .action = matching::RequestAction::ADD,
        .symbol = btc,
        .order_id = 9999999,
        .side = core::Side::SELL,
        .price = 50000 * core::PRICE_SCALE,
        .qty = 100000000,
        .order_type = core::OrderType::LIMIT,
        .timestamp = core::now_ns(),
        .participant_id = 101,
        .display_qty = 100000000
    };
    engine.process_order(sell_wall);

    core::OrderId buy_id = 1;
    for (auto _ : state) {
        matching::InboundOrder aggressive_buy{
            .action = matching::RequestAction::ADD,
            .symbol = btc,
            .order_id = buy_id++,
            .side = core::Side::BUY,
            .price = 50000 * core::PRICE_SCALE,
            .qty = 1,
            .order_type = core::OrderType::MARKET,
            .timestamp = core::now_ns(),
            .participant_id = 102,
            .display_qty = 1
        };
        benchmark::DoNotOptimize(engine.process_order(aggressive_buy));
    }
}
BENCHMARK(BM_AggressiveMarketOrderMatch_Naive)
    ->Iterations(BENCHMARK_OPERATIONS)
    ->Repetitions(BENCHMARK_REPETITIONS);

// ============================================================================
// Custom Benchmark Reporter: Outputs Google Benchmark data + Average Speedup
// ============================================================================

class PerformanceJumpReporter : public benchmark::ConsoleReporter {
public:
    void ReportRuns(const std::vector<Run>& reports) override {
        // Output standard Google Benchmark console reporting for each run
        ConsoleReporter::ReportRuns(reports);

        // Record individual iteration measurements
        for (const auto& run : reports) {
            if (run.run_type == Run::RT_Iteration && !run.skipped) {
                cpu_times_[run.run_name.function_name].push_back(run.GetAdjustedCPUTime());
                real_times_[run.run_name.function_name].push_back(run.GetAdjustedRealTime());
            }
        }
    }

    void Finalize() override {
        ConsoleReporter::Finalize();

        auto calc_mean = [](const std::vector<double>& vals) -> double {
            if (vals.empty()) return 0.0;
            const double sum = std::accumulate(vals.begin(), vals.end(), 0.0);
            return sum / static_cast<double>(vals.size());
        };

        const double passive_opt_cpu = calc_mean(cpu_times_["BM_PassiveOrderAdd_Optimized"]);
        const double passive_naive_cpu = calc_mean(cpu_times_["BM_PassiveOrderAdd_Naive"]);
        const double match_opt_cpu = calc_mean(cpu_times_["BM_AggressiveMarketOrderMatch_Optimized"]);
        const double match_naive_cpu = calc_mean(cpu_times_["BM_AggressiveMarketOrderMatch_Naive"]);

        std::cout << "\n"
                  << "========================================================================================\n"
                  << "               AVERAGE PERFORMANCE JUMP REPORT (ACROSS " << BENCHMARK_REPETITIONS << " RUNS)                 \n"
                  << "========================================================================================\n"
                  << std::fixed << std::setprecision(2);

        if (passive_opt_cpu > 0.0 && passive_naive_cpu > 0.0) {
            const double speedup = passive_naive_cpu / passive_opt_cpu;
            const double latency_reduction = (1.0 - (passive_opt_cpu / passive_naive_cpu)) * 100.0;
            std::cout << "Scenario 1: Passive Order Add (" << BENCHMARK_OPERATIONS << " orders/run)\n"
                      << "  - Naive Engine (Standard new/delete + std::unordered_map): " << passive_naive_cpu << " ns/op\n"
                      << "  - Optimized Engine (core::MemoryPool + PMR Arena):         " << passive_opt_cpu << " ns/op\n"
                      << "  - Average Performance Jump:                               " << speedup << "x faster ("
                      << "-" << latency_reduction << "% latency)\n\n";
        }

        if (match_opt_cpu > 0.0 && match_naive_cpu > 0.0) {
            const double speedup = match_naive_cpu / match_opt_cpu;
            const double latency_reduction = (1.0 - (match_opt_cpu / match_naive_cpu)) * 100.0;
            std::cout << "Scenario 2: Aggressive Market Order Match (" << BENCHMARK_OPERATIONS << " orders/run)\n"
                      << "  - Naive Engine (Standard new/delete + std::unordered_map): " << match_naive_cpu << " ns/op\n"
                      << "  - Optimized Engine (core::MemoryPool + PMR Arena):         " << match_opt_cpu << " ns/op\n"
                      << "  - Average Performance Jump:                               " << speedup << "x faster ("
                      << "-" << latency_reduction << "% latency)\n";
        }

        std::cout << "========================================================================================\n\n";
    }

private:
    std::map<std::string, std::vector<double>> cpu_times_;
    std::map<std::string, std::vector<double>> real_times_;
};

int main(int argc, char** argv) {
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }

    PerformanceJumpReporter reporter;
    ::benchmark::RunSpecifiedBenchmarks(&reporter);
    ::benchmark::Shutdown();
    return 0;
}