#pragma once

#include <comb/parse.hpp>

namespace comb_test {

auto test_code_from_example() -> void;
auto test_second_example() -> void;
auto test_parse_sequence() -> void;
auto test_parse_char() -> void;
auto test_parse_combine() -> void;
auto test_parse_integer() -> void;
auto test_parse_whitespaces() -> void;
auto test_parse_newline() -> void;
auto test_parse_quoted_string() -> void;
auto test_parse_parser_sequence() -> void;
auto test_parse_parser_right() -> void;
auto test_parse_parser_left_right() -> void;
auto test_parse_parser_map() -> void;
auto test_parse_parser_vector_sequence() -> void;
auto test_parse_opt() -> void;
auto test_parse_opt_default() -> void;
auto test_parse_list() -> void;
auto test_parse_list_disallowed_trailing_sep() -> void;
auto test_parse_list_required_trailing_sep() -> void;
auto test_parse_list_min_n_elems() -> void;
auto test_parse_pair() -> void;
auto test_parse_json() -> void;
auto test_parse_json_object() -> void;
auto test_parse_float() -> void;

}  // namespace tmine_test
