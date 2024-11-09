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
    auto parser = (character('a') | character('b')).repeat();

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

auto test_parse_list() -> void {
    auto parser = list(prefix("elem"), character(','));

    auto result1 = parser.parse("elem,elem,elem,elemtail");

    comb_assert(result1.ok());
    comb_assert_eq(
        result1.get_value(),
        (std::vector<std::string_view>{"elem", "elem", "elem", "elem"})
    );
    comb_assert_eq(result1.tail, "tail");

    auto result2 = parser.parse("elem,");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), std::vector<std::string_view>{"elem"});
    comb_assert_eq(result2.tail, "");

    auto result3 = parser.parse(",");

    comb_assert(result3.ok());
    comb_assert(result3.get_value().empty());
    comb_assert_eq(result3.tail, ",");

    auto result4 = parser.parse("elem");

    comb_assert(result4.ok());
    comb_assert_eq(result4.get_value(), std::vector<std::string_view>{"elem"});
    comb_assert_eq(result4.tail, "");
}

auto test_parse_list_disallowed_trailing_sep() -> void {
    auto parser = list(
        integer(), whitespace() >> character(',') << whitespace(),
        TrailingSeparator::Disallowed
    );

    auto result1 = parser.parse("1, 2, 3  ,  4, 6, ");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), (std::vector<int64_t>{1, 2, 3, 4, 6}));
    comb_assert_eq(result1.tail, ", ");

    auto result2 = parser.parse("1 2 3");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), std::vector<int64_t>{1});
    comb_assert_eq(result2.tail, " 2 3");

    auto result3 = parser.parse("42,  ");

    comb_assert(result3.ok());
    comb_assert_eq(result3.get_value(), std::vector<int64_t>{42});
    comb_assert_eq(result3.tail, ",  ");

    auto result4 = parser.parse(",  12, 1");

    comb_assert(result4.ok());
    comb_assert_eq(result4.get_value(), std::vector<int64_t>{});
    comb_assert_eq(result4.tail, ",  12, 1");

    auto result5 = parser.parse("1, 2   ");

    comb_assert(result5.ok());
    comb_assert_eq(result5.get_value(), (std::vector<int64_t>{1, 2}));
    comb_assert_eq(result5.tail, "   ");
}

auto test_parse_list_required_trailing_sep() -> void {
    auto parser = list(
        quoted_string('\''), whitespace() >> character(',') << whitespace(),
        TrailingSeparator::Required
    );

    auto result1 =
        parser.parse("'value1,',  'value2'  , 'val,ue3'  ,  other stuff");

    comb_assert(result1.ok());
    comb_assert_eq(
        result1.get_value(),
        (std::vector<std::string_view>{"value1,", "value2", "val,ue3"})
    );
    comb_assert_eq(result1.tail, "other stuff");

    auto result2 =
        parser.parse("'value1,',  'value2'  , 'val,ue3'  other stuff");

    comb_assert(result2.ok());
    comb_assert_eq(
        result2.get_value(),
        (std::vector<std::string_view>{"value1,", "value2"})
    );
    comb_assert_eq(result2.tail, "'val,ue3'  other stuff");

    auto result3 = parser.parse("'value1'");

    comb_assert(result3.ok());
    comb_assert(result3.get_value().empty());
    comb_assert_eq(result3.tail, "'value1'");
}

auto test_parse_list_min_n_elems() -> void {
    auto parse = list(
        prefix("true").map([](auto) { return true; }) |
            prefix("false").map([](auto) { return false; }),
        whitespace(1), TrailingSeparator::Disallowed, 1
    );

    auto result1 = parse("true   true  false true  tr");

    comb_assert(result1.ok());
    comb_assert_eq(
        result1.get_value(), (std::vector<bool>{true, true, false, true})
    );
    comb_assert_eq(result1.tail, "  tr");

    auto result2 = parse("true ");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), std::vector<bool>{true});
    comb_assert_eq(result2.tail, " ");

    auto result3 = parse("something else");

    comb_assert(!result3.ok());

    auto result4 = parse("    trailing separators");

    comb_assert(!result4.ok());

    auto result5 = parse("false");

    comb_assert(result5.ok());
    comb_assert_eq(result5.get_value(), std::vector<bool>{false});
    comb_assert_eq(result5.tail, "");
}

auto test_parse_pair() -> void {
    auto parse = (quoted_string('\'') << whitespace() << character(':')) &
                 (whitespace() >> integer());

    auto result = parse("'value' :  42tail");

    comb_assert(result.ok());

    auto tail = result.tail;
    auto [key, value] = std::move(result).get_value();

    comb_assert_eq(key, "value");
    comb_assert_eq(value, 42);
    comb_assert_eq(tail, "tail");
}

auto test_parse_float() -> void {
    auto parse = list(floating(), whitespace());

    auto result1 = parse("1.2 3.1415 2.718281828");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), (std::vector<double>{1.2, 3.1415, 2.718281828}));
    comb_assert_eq(result1.tail, "");
}

}  // namespace comb_test
