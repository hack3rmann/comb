#include "../json.hpp"
#include "../parse.hpp"
#include "../assert.hpp"

namespace comb_test {

using namespace comb;

auto test_parse_json() -> void {
    auto parse = json::json();

    auto result = parse("\"string\"");
    auto tail = result.tail;

    comb_assert(result.ok());

    auto value = std::move(result).get_value();
    auto json_string = std::get<json::JsonString>(std::move(value).value);

    comb_assert_eq(json_string, "string");
    comb_assert_eq(tail, "");
}

auto test_parse_json_object() -> void {
    auto parse = json::json();

    auto result = parse("{ \"name\": \"Bob\", \"Money\": 666.42 }");

    comb_assert(result.ok());

    auto value = std::move(result).get_value();
    auto json_object = std::get<json::JsonObject>(std::move(value).value);

    comb_assert(json_object.contains("name"));
    comb_assert(json_object.contains("Money"));
    comb_assert_eq(json_object.size(), 2);
    
    auto name = std::get<json::JsonString>(json_object.at("name").value);
    auto money = std::get<json::JsonFloat>(json_object.at("Money").value);

    comb_assert_eq(name, "Bob");
    comb_assert_eq(money, 666.42);
}

}
