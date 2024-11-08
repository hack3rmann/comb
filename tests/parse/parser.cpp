#include <fmt/ranges.h>
#include "../assert.hpp"
#include "../parse.hpp"

namespace comb_test {

using namespace comb;

auto test_parse_parser_sequence() -> void {
    auto parser = prefix("Hello") | prefix("Goodbye");

    auto result1 = parser.parse("Hello, World!");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "Hello");
    comb_assert_eq(result1.tail, ", World!");

    auto result2 = parser.parse("Goodbye, World!");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "Goodbye");
    comb_assert_eq(result2.tail, ", World!");
}

auto test_parse_parser_right() -> void {
    auto parser = character('=') >> prefix("value");

    auto result1 = parser.parse("=value tail");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "value");
    comb_assert_eq(result1.tail, " tail");

    auto result2 = parser.parse("value tail");

    comb_assert(!result2.ok());

    auto result3 = parser.parse("=novalue");

    comb_assert(!result3.ok());
}

auto test_parse_quoted_string() -> void {
    auto const result1 = quoted_string('"').parse("\"String\"");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "String");
    comb_assert_eq(result1.tail, "");

    auto const result2 = quoted_string('"').parse("\"NotString");

    comb_assert(!result2.ok());

    auto const result3 = quoted_string('"').parse("AlsoNotAString");

    comb_assert(!result3.ok());

    auto const result4 = quoted_string('"').parse("\"\"String!");

    comb_assert(result4.ok());
    comb_assert_eq(result4.get_value(), "");
    comb_assert_eq(result4.tail, "String!");
}

auto test_parse_parser_left_right() -> void {
    auto parser = character('<') >> prefix("value") << character('>');

    auto result1 = parser.parse("<value>tail");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "value");
    comb_assert_eq(result1.tail, "tail");

    auto result2 = parser.parse("<valuetail");

    comb_assert(!result2.ok());

    auto result3 = parser.parse("value>tail");

    comb_assert(!result3.ok());
}

auto test_parse_parser_map() -> void {
    enum class State {
        First,
        Second,
        Third,
    };

    auto parser = prefix("first").map([](auto) { return State::First; }) |
                  prefix("second").map([](auto) { return State::Second; }) |
                  prefix("third").map([](auto) { return State::Third; });

    auto result1 = parser.parse("first_tail");

    comb_assert(result1.ok());
    comb_assert_eq((uint32_t) result1.get_value(), (uint32_t) State::First);
    comb_assert_eq(result1.tail, "_tail");

    auto result2 = parser.parse("second_tail");

    comb_assert(result2.ok());
    comb_assert_eq((uint32_t) result2.get_value(), (uint32_t) State::Second);
    comb_assert_eq(result2.tail, "_tail");

    auto result3 = parser.parse("third_tail");

    comb_assert(result3.ok());
    comb_assert_eq((uint32_t) result3.get_value(), (uint32_t) State::Third);
    comb_assert_eq(result3.tail, "_tail");
}

auto test_parse_parser_vector_sequence() -> void {
    auto parser = (character('a') | character('b')).sequence();

    auto result = parser.parse("ababbcaba");

    comb_assert(result.ok());
    comb_assert_eq(
        result.get_value(), (std::vector<char>{'a', 'b', 'a', 'b', 'b'})
    );
    comb_assert_eq(result.tail, "caba");
}

auto test_parse_opt() -> void {
    auto parser = prefix("value") >> character('=') >> integer().opt();

    auto result1 = parser.parse("value=42_tail");

    comb_assert(result1.ok());
    comb_assert(result1.get_value().has_value());
    comb_assert_eq(result1.get_value().value(), 42);
    comb_assert_eq(result1.tail, "_tail");

    auto result2 = parser.parse("value=_tail");

    comb_assert(result2.ok());
    comb_assert(!result2.get_value().has_value());
}

}  // namespace comb_test
