#include "../json.hpp"

namespace json {

using namespace comb;

auto parse(std::string_view src) -> ParseResult<JsonValue> {
    auto parse_bool = prefix("false").map([](auto) {
        return JsonValue{false};
    }) | prefix("true").map([](auto) { return JsonValue{true}; });

    auto parse_integer =
        integer().map([](auto value) { return JsonValue{value}; });

    auto parse_float =
        floating().map([](auto value) { return JsonValue{value}; });

    auto parse_string =
        quoted_string().map([](auto string) { return JsonValue{string}; });

    auto parse_list =
        character('[') >> whitespace() >>
        list(json(), whitespace() >> character(',') << whitespace(), TrailingSeparator::Disallowed)
            << whitespace() << character(']');

    auto parse_object = 0;

    exit(1);
}

}  // namespace json
