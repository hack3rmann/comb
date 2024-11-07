#include "../assert.hpp"
#include "../parse.hpp"

namespace comb_test {

using namespace comb;

auto test_parse_sequence() -> void {
    auto const result1 = parse_sequence("hello, world!", "hello");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "hello");
    comb_assert_eq(result1.tail, ", world!");

    auto const result2 = parse_sequence("Hello, World!", "Minecraft");

    comb_assert(!result2.ok());
}

auto test_parse_char() -> void {
    auto const result1 = parse_char("Terramine", 'T');

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), 'T');
    comb_assert_eq(result1.tail, "erramine");

    auto const result2 = parse_char("Minecraft", 'A');

    comb_assert(!result2.ok());
}

auto test_parse_combine() -> void {
    auto const src = "Minecraft is a good game";

    auto const result1 = parse_sequence(src, "Terraria") |
                         parse_sequence(src, "Minecraft");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "Minecraft");
    comb_assert_eq(result1.tail, " is a good game");

    auto const result2 = parse_sequence(src, "Minecraft") |
                         parse_sequence(src, "Terraria");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "Minecraft");
    comb_assert_eq(result2.tail, " is a good game");

    auto const result3 = parse_sequence(src, "Terraria") |
                         parse_sequence(src, "VintageStory");

    comb_assert(!result3.ok());
}

auto test_parse_integer() -> void {
    auto const result1 = parse_integer("42");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), 42);
    comb_assert_eq(result1.tail, "");

    auto const result2 = parse_integer("1234567 is a number");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), 1234567);
    comb_assert_eq(result2.tail, " is a number");

    auto const result3 = parse_integer("Hello, Wolrd!");

    comb_assert(!result3.ok());

    auto const result4 = parse_integer("-666");

    comb_assert(result4.ok());
    comb_assert_eq(result4.get_value(), -666);
    comb_assert_eq(result4.tail, "");
}

auto test_parse_whitespaces() -> void {
    auto const result1 = parse_whitespace("  \t\nName");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "  \t\n");
    comb_assert_eq(result1.tail, "Name");

    auto const result2 = parse_whitespace("Name");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "");
    comb_assert_eq(result2.tail, "Name");

    auto const result3 = parse_whitespace("Name", 1);

    comb_assert(!result3.ok());

    auto const result4 = parse_whitespace(" Number", 2);

    comb_assert(!result4.ok());

    auto const result5 = parse_whitespace(" \n Number", 2);

    comb_assert(result5.ok());
    comb_assert_eq(result5.get_value(), " \n ");
    comb_assert_eq(result5.tail, "Number");
}

auto test_parse_newline() -> void {
    auto const result1 = parse_newline("\nNew line");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "\n");
    comb_assert_eq(result1.tail, "New line");

    auto const result2 = parse_newline("\r\nNew line");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "\r\n");
    comb_assert_eq(result2.tail, "New line");
}

}  // namespace comb_test
