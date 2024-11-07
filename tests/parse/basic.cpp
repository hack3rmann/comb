#include "../assert.hpp"
#include "../parse.hpp"

namespace comb_test {

using namespace comb;

auto test_parse_sequence() -> void {
    auto const result1 = prefix("hello").parse("hello, world!");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "hello");
    comb_assert_eq(result1.tail, ", world!");

    auto const result2 = prefix("Minecraft").parse("Hello, World!");

    comb_assert(!result2.ok());
}

auto test_parse_char() -> void {
    auto const result1 = character('T').parse("Terramine");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), 'T');
    comb_assert_eq(result1.tail, "erramine");

    auto const result2 = character('A').parse("Minecraft");

    comb_assert(!result2.ok());
}

auto test_parse_combine() -> void {
    auto const src = "Minecraft is a good game";

    auto const result1 =
        (prefix("Terraria") | prefix("Minecraft")).parse(src);

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "Minecraft");
    comb_assert_eq(result1.tail, " is a good game");

    auto const result2 =
        (prefix("Minecraft") | prefix("Terraria")).parse(src);

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "Minecraft");
    comb_assert_eq(result2.tail, " is a good game");

    auto const result3 =
        (prefix("Terraria") | prefix("VintageStory")).parse(src);

    comb_assert(!result3.ok());
}

auto test_parse_integer() -> void {
    auto const result1 = integer().parse("42");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), 42);
    comb_assert_eq(result1.tail, "");

    auto const result2 = integer().parse("1234567 is a number");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), 1234567);
    comb_assert_eq(result2.tail, " is a number");

    auto const result3 = integer().parse("Hello, Wolrd!");

    comb_assert(!result3.ok());

    auto const result4 = integer().parse("-666");

    comb_assert(result4.ok());
    comb_assert_eq(result4.get_value(), -666);
    comb_assert_eq(result4.tail, "");
}

auto test_parse_whitespaces() -> void {
    auto const result1 = whitespace().parse("  \t\nName");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "  \t\n");
    comb_assert_eq(result1.tail, "Name");

    auto const result2 = whitespace().parse("Name");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "");
    comb_assert_eq(result2.tail, "Name");

    auto const result3 = whitespace(1).parse("Name");

    comb_assert(!result3.ok());

    auto const result4 = whitespace(2).parse(" Number");

    comb_assert(!result4.ok());

    auto const result5 = whitespace(2).parse(" \n Number");

    comb_assert(result5.ok());
    comb_assert_eq(result5.get_value(), " \n ");
    comb_assert_eq(result5.tail, "Number");
}

auto test_parse_newline() -> void {
    auto const result1 = newline().parse("\nNew line");

    comb_assert(result1.ok());
    comb_assert_eq(result1.get_value(), "\n");
    comb_assert_eq(result1.tail, "New line");

    auto const result2 = newline().parse("\r\nNew line");

    comb_assert(result2.ok());
    comb_assert_eq(result2.get_value(), "\r\n");
    comb_assert_eq(result2.tail, "New line");
}

}  // namespace comb_test
