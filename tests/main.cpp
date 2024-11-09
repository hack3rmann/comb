#include "util.hpp"
#include "parse.hpp"

using namespace comb_test;

auto main() -> int {
    perform_test(test_code_from_example);
    perform_test(test_second_example);
    perform_test(test_parse_char);
    perform_test(test_parse_sequence);
    perform_test(test_parse_combine);
    perform_test(test_parse_integer);
    perform_test(test_parse_whitespaces);
    perform_test(test_parse_newline);
    perform_test(test_parse_quoted_string);
    perform_test(test_parse_parser_sequence);
    perform_test(test_parse_parser_right);
    perform_test(test_parse_parser_left_right);
    perform_test(test_parse_parser_map);
    perform_test(test_parse_parser_vector_sequence);
    perform_test(test_parse_opt);
    perform_test(test_parse_list);
    perform_test(test_parse_list_disallowed_trailing_sep);
    perform_test(test_parse_list_required_trailing_sep);
    perform_test(test_parse_list_min_n_elems);
}
