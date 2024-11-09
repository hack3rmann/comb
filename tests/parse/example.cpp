#include <cassert>

#include "../parse.hpp"

namespace comb_test {

using namespace comb;

auto test_code_from_example() -> void {
    auto constexpr SOURCE = "name = 'George'\n"
                            "name  = 'John'\r\n"
                            "name ='Amy'\r";

    // prefix("name") means parse if source prefix matches "name"
    // whitespaces(n) parses any ASCII whitespace symbol at leas n times (n is 0 by default)
    // quoted_string('\'') parses all text inside the given quote symbol
    // >> means "parse and drop current value, take right"
    // << means "parse and take current value, drop right"
    // newline() parses newline symbol (one of LF, CR of CRLF)
    // .sequence(1) means "repeat current parser at least 1 times"
    auto parser = (prefix("name")
                >> whitespace()
                >> character('=')
                >> whitespace()
                >> quoted_string('\'')
                << newline()).sequence(1);

    auto result = parser.parse(SOURCE);
    assert(result.ok());

    auto names = std::move(result).get_value();
    assert(names == (std::vector<std::string_view>{"George", "John", "Amy"}));
}

// parses '$(name)' with any given $(name)
static auto single_quoted_name(std::string_view name) {
    return character('\'') >> prefix(name) << character('\'');
}

auto test_second_example() -> void {
    auto constexpr SOURCE = "'first' 'second'  'third'\t 'second'";

    enum class Variant {
        First, Second, Third,
    };

    // .map transforms parsed string (if there is any) to returned value
    // | means "parse left or parse right". Note that it is short-circuited
    auto variant_parser
        = single_quoted_name("first").map([](auto) { return Variant::First; })
        | single_quoted_name("second").map([](auto) { return Variant::Second; })
        | single_quoted_name("third").map([](auto) { return Variant::Third; });

    // repeat this parser separated by any number of whitespaces
    auto parser = (variant_parser << whitespace()).sequence();

    auto result = parser.parse(SOURCE);
    assert(result.ok());

    auto variants = std::move(result).get_value();
    assert(variants == (std::vector<Variant>{
        Variant::First, Variant::Second, Variant::Third, Variant::Second
    }));
}

}
