#include <fmt/printf.h>
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
        (character('[') >> whitespace() >>
         list(
             json(), whitespace() >> character(',') << whitespace(),
             TrailingSeparator::Disallowed
         ) << whitespace()
           << character(']'))
            .map([](auto list) { return JsonValue{std::move(list)}; });

    auto key_value = (quoted_string() << whitespace() << character(':')) &
                     (whitespace() >> json());

    auto parse_object =
        (character('{') >> whitespace() >>
         list(
             std::move(key_value),
             whitespace() >> character(',') << whitespace(),
             TrailingSeparator::Disallowed
         ) << whitespace()
           << character('}'))
            .map([](auto pair_list) {
                auto result = std::unordered_map<std::string_view, JsonValue>{};

                for (auto pair : std::move(pair_list)) {
                    result.insert(std::move(pair));
                }

                return JsonValue{std::move(result)};
            });

    auto parse_value =
        whitespace() >> (std::move(parse_bool) | std::move(parse_integer) |
                         std::move(parse_float) | std::move(parse_string) |
                         std::move(parse_list) | std::move(parse_object)
                        ) << whitespace();

    return parse_value(src);
}

}  // namespace json
