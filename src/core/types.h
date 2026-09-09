#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include <string_view>
#include <type_traits>

namespace exchange::core {
    using Price = std::int64_t;
    using Quantity = std::uint64_t;
    using OrderId = std::uint64_t;
    using SequenceNumber = std::uint64_t;
    using Timestamp = std::uint64_t;
    using ParticipantId = std::uint32_t;
    using MatchId = std::uint64_t;

    inline constexpr Price PRICE_SCALE = 10000;

    enum class Side : std::uint8_t {
        BUY,
        SELL
    };

    enum class OrderType : std::uint8_t {
        LIMIT,
        MARKET,
        IOC,
        FOK,
        GTC,
        STOP,
        STOP_LIMIT,
        ICEBERG,
        POST_ONLY
    };

    enum class OrderStatus : std::uint8_t {
        NEW,
        ACCEPTED,
        PARTIALLY_FILLED,
        FILLED,
        CANCELED
    };

    using Symbol = std::array<char, 16>;

    constexpr Symbol make_symbol(std::string_view str) {
        Symbol symbol{};
        std::copy_n(str.data(), std::min(str.size(), symbol.size()), symbol.data());
        return symbol;
    }

    constexpr std::string_view symbol_view(const Symbol& symbol) {
        return std::string_view(symbol.data(), std::find(symbol.begin(), symbol.end(), '\0') - symbol.begin());
    }

    struct SymbolLess {
        bool operator()(const Symbol& lhs, const Symbol& rhs) const {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }
    };

    static_assert(std::is_trivially_copyable_v<Price>);
    static_assert(std::is_trivially_copyable_v<Quantity>);
    static_assert(std::is_trivially_copyable_v<OrderId>);
    static_assert(std::is_trivially_copyable_v<SequenceNumber>);
    static_assert(std::is_trivially_copyable_v<Timestamp>);
    static_assert(std::is_trivially_copyable_v<ParticipantId>);
    static_assert(std::is_trivially_copyable_v<MatchId>);
    static_assert(std::is_trivially_copyable_v<Side>);
    static_assert(std::is_trivially_copyable_v<OrderType>);
    static_assert(std::is_trivially_copyable_v<OrderStatus>);
    static_assert(std::is_trivially_copyable_v<Symbol>);
}