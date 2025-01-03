#pragma once

#include <variant>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <comb/parse.hpp>

namespace json {

enum class JsonVariant {
    Bool,
    Integer,
    Float,
    String,
    List,
    Object,
};

using JsonBool = bool;
using JsonInteger = int64_t;
using JsonFloat = double;
using JsonString = std::string_view;
using JsonList = std::vector<struct JsonValue>;
using JsonObject = std::unordered_map<std::string_view, struct JsonValue>;

struct JsonValue {
    inline explicit JsonValue(JsonBool value)
    : value{value} {}

    inline explicit JsonValue(JsonInteger value)
    : value{value} {}

    inline explicit JsonValue(JsonFloat value)
    : value{value} {}

    inline explicit JsonValue(JsonString value)
    : value{value} {}

    inline explicit JsonValue(JsonList value)
    : value{std::move(value)} {}

    inline explicit JsonValue(JsonObject value)
    : value{std::move(value)} {}

    std::variant<
        JsonBool, JsonInteger, JsonFloat, JsonString, JsonList, JsonObject>
        value;
};

auto parse(std::string_view src) -> comb::ParseResult<JsonValue>;

inline auto json() -> comb::ParserLike auto {
    auto parse = [](std::string_view src) -> comb::ParseResult<JsonValue> {
        return ::json::parse(src);
    };

    return comb::Parser<decltype(parse)>{std::move(parse)};
}

}  // namespace json
