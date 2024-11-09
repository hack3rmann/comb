#pragma once

#include <string_view>
#include <optional>
#include <vector>
#include <cstdint>
#include <cerrno>
#include <cstdlib>

namespace comb {

template <class T, class Char>
struct BasicParseResult {
    std::optional<T> value;
    std::basic_string_view<Char> tail;

    auto ok(this BasicParseResult const& self) -> bool {
        return self.value.has_value();
    }

    template <class Self>
    auto get_value(this Self&& self) {
        return std::forward<Self>(self).value.value();
    }

    friend auto operator|(BasicParseResult&& lhs, BasicParseResult&& rhs)
        -> BasicParseResult {
        if (lhs.ok()) {
            return std::move(lhs);
        } else {
            return std::move(rhs);
        }
    }
};

template <class T>
using ParseResult = BasicParseResult<T, char>;

template <class T, class Char>
concept BasicParseFunction =
    requires(T parse, std::basic_string_view<Char> src) {
        {
            parse(src).ok()
        } -> std::same_as<bool>;
        parse(src).get_value();
        {
            parse(src).tail
        } -> std::same_as<decltype(src)&&>;
    };

template <class T>
concept ParseFunction = BasicParseFunction<T, char>;

template <class T, class Char>
concept BasicParserLike = requires(T parser, std::basic_string_view<Char> src) {
    {
        parser(src)
    } -> std::same_as<decltype(parser.parse(src))>;
    {
        parser.parse
    } -> BasicParseFunction<Char>;
};

template <class T>
concept ParserLike = BasicParserLike<T, char>;

template <class T, class Char>
    requires BasicParseFunction<T, Char>
struct BasicParser {
    T parse;

    using ParseValue =
        decltype(parse(std::basic_string_view<Char>{}).get_value());

    template <class Self>
    auto operator()(this Self&& self, std::basic_string_view<Char> src)
        -> BasicParseResult<ParseValue, Char> {
        return std::forward<Self>(self).parse(src);
    }

    template <BasicParseFunction<Char> S>
    friend auto operator|(
        BasicParser<T, Char>&& lhs, BasicParser<S, Char>&& rhs
    ) -> BasicParserLike<Char> auto {
        auto parse = [=](std::basic_string_view<Char> src) {
            auto left_result = std::move(lhs.parse)(src);

            if (left_result.ok()) {
                return std::move(left_result);
            } else {
                auto right_result = std::move(rhs.parse)(src);
                return std::move(right_result);
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }

    template <BasicParseFunction<Char> S>
    friend auto operator>>(
        BasicParser<T, Char>&& lhs, BasicParser<S, Char>&& rhs
    ) -> BasicParserLike<Char> auto {
        auto parse = [=](std::basic_string_view<Char> src) {
            auto left_result = std::move(lhs.parse)(src);

            if (!left_result.ok()) {
                return decltype(std::move(rhs.parse)(src)
                ){.value = ::std::nullopt, .tail = src};
            } else {
                auto right_result = std::move(rhs.parse)(left_result.tail);
                return std::move(right_result);
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }

    template <BasicParseFunction<Char> S>
    friend auto operator<<(
        BasicParser<T, Char>&& lhs, BasicParser<S, Char>&& rhs
    ) -> BasicParserLike<Char> auto {
        auto parse = [=](std::string_view src) {
            auto left_result = std::move(lhs.parse)(src);

            if (left_result.ok()) {
                auto right_result = std::move(rhs.parse)(left_result.tail);

                if (right_result.ok()) {
                    return decltype(left_result
                    ){.value = std::move(left_result).value,
                      .tail = right_result.tail};
                } else {
                    return decltype(left_result
                    ){.value = ::std::nullopt, .tail = src};
                }
            } else {
                return left_result;
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }

    auto map(this BasicParser&& self, auto transform)
        -> BasicParserLike<Char> auto
        requires requires {
            transform(self.parse(std::basic_string_view<Char>{}).get_value());
        }
    {
        auto parse = [=](std::basic_string_view<Char> src) {
            auto result = (self) (src);

            using NewType = decltype(transform(result.get_value()));

            if (result.ok()) {
                return BasicParseResult<NewType, Char>{
                    .value = transform(std::move(result.get_value())),
                    .tail = result.tail
                };
            } else {
                return BasicParseResult<NewType, Char>{
                    .value = std::nullopt, .tail = result.tail
                };
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }

    auto sequence(this BasicParser&& self, size_t min_count = 0)
        -> BasicParserLike<Char> auto {
        auto parse = [=](std::basic_string_view<Char> src) {
            using Value = decltype(self.parse(src).get_value());
            using Sequence = std::vector<Value>;

            auto result_sequence = std::vector<Value>{};
            auto tail = src;

            for (auto result = self.parse(tail); result.ok();
                 result = self.parse(tail))
            {
                result_sequence.emplace_back(std::move(result).get_value());
                tail = result.tail;
            }

            if (result_sequence.size() < min_count) {
                return BasicParseResult<Sequence, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };
            } else {
                return BasicParseResult<Sequence, Char>{
                    .value = std::move(result_sequence),
                    .tail = tail,
                };
            }
        };

        return BasicParser<decltype(parse), Char>(std::move(parse));
    }

    auto opt(this BasicParser&& self) -> BasicParserLike<Char> auto {
        auto parse = [=](std::basic_string_view<Char> src) {
            using Value = std::optional<decltype(self.parse(src).get_value())>;

            auto result = self.parse(src);

            return BasicParseResult<Value, Char>{
                .value = std::make_optional<Value>(std::move(result).value),
                .tail = result.tail,
            };
        };

        return BasicParser<decltype(parse), Char>(std::move(parse));
    }
};

template <class T>
using Parser = BasicParser<T, char>;

inline auto character(char value) -> ParserLike auto {
    return Parser{[value](std::string_view src) -> ParseResult<char> {
        if (src.empty() || src[0] != value) {
            return ParseResult<char>{.value = std::nullopt, .tail = src};
        } else {
            src.remove_prefix(1);
            return ParseResult<char>{.value = value, .tail = src};
        }
    }};
}

// FIXME(hack3rmann): remove this definition
inline auto is_whitespace(char value) -> bool {
    return (9 <= value && value <= 13) || 32 == value;
}

namespace basic {
    template <class Char>
    inline auto prefix(std::basic_string_view<Char> match)
        -> BasicParserLike<Char> auto {
        auto parse = [match](std::basic_string_view<Char> src
                     ) -> BasicParseResult<std::basic_string_view<Char>, Char> {
            if (src.size() < match.size() || !src.starts_with(match)) {
                return BasicParseResult<std::basic_string_view<Char>, Char>{
                    .value = std::nullopt, .tail = src
                };
            } else {
                auto head = src;
                head.remove_suffix(src.size() - match.size());
                src.remove_prefix(match.size());

                return BasicParseResult<std::basic_string_view<Char>, Char>{
                    .value = head, .tail = src
                };
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }
}  // namespace basic

inline auto prefix(std::string_view match) -> ParserLike auto {
    return basic::prefix<char>(match);
}

inline auto integer(uint32_t radix = 10) -> ParserLike auto {
    return Parser{[radix](std::string_view src) -> ParseResult<int64_t> {
        errno = 0;
        char* parse_end = nullptr;
        auto const value = std::strtoll(src.data(), &parse_end, radix);
        auto const parse_size = (size_t) (parse_end - src.data());

        if (0 != errno || parse_size > src.size() || parse_size == 0) {
            return ParseResult<int64_t>{.value = std::nullopt, .tail = src};
        } else {
            src.remove_prefix(parse_size);
            return ParseResult<int64_t>{.value = (int64_t) value, .tail = src};
        }
    }};
}

inline auto floating() -> ParserLike auto {
    return Parser{[](std::string_view src) -> ParseResult<double> {
        errno = 0;
        char* parse_end = nullptr;
        auto const value = std::strtod(src.data(), &parse_end);
        auto const parse_size = (size_t) (parse_end - src.data());

        if (0 != errno || parse_size > src.size() || parse_size == 0) {
            return ParseResult<double>{.value = std::nullopt, .tail = src};
        } else {
            src.remove_prefix(parse_size);
            return ParseResult<double>{.value = value, .tail = src};
        }
    }};
}

// TODO(hack3rmann): move to `charonly` module
inline auto whitespace(uint32_t min_count = 0) -> ParserLike auto {
    return Parser{
        [min_count](std::string_view src) -> ParseResult<std::string_view> {
            auto n_spaces = size_t{0};

            for (auto symbol : src) {
                if (!is_whitespace(symbol)) {
                    break;
                }

                n_spaces += 1;
            }

            if (n_spaces < min_count) {
                return ParseResult<std::string_view>{
                    .value = std::nullopt, .tail = src
                };
            } else {
                auto match = src;
                match.remove_suffix(src.size() - n_spaces);
                src.remove_prefix(n_spaces);

                return ParseResult<std::string_view>{
                    .value = match, .tail = src
                };
            }
        }
    };
}

// TODO(hack3rmann): move to `charonly` module
inline auto newline() -> ParserLike auto {
    return prefix("\r\n") | prefix("\n") | prefix("\r");
}

// TODO(hack3rmann): move to `charonly` module
inline auto quoted_string(char quote_symbol = '"') -> ParserLike auto {
    return Parser{
        [quote_symbol](std::string_view src) -> ParseResult<std::string_view> {
            auto const open_quote = character(quote_symbol).parse(src);
            auto tail = open_quote.tail;

            if (!open_quote.ok()) {
                return ParseResult<std::string_view>{
                    .value = std::nullopt, .tail = src
                };
            }

            auto n_string_symbols = size_t{0};

            for (auto symbol : tail) {
                if (quote_symbol == symbol) {
                    break;
                }

                n_string_symbols += 1;
            }

            if (n_string_symbols == tail.size() ||
                quote_symbol != tail[n_string_symbols])
            {
                return ParseResult<std::string_view>{
                    .value = std::nullopt, .tail = src
                };
            }

            auto match = tail;
            match.remove_suffix(src.size() - 1 - n_string_symbols);
            tail.remove_prefix(n_string_symbols + 1);

            return ParseResult<std::string_view>{.value = match, .tail = tail};
        }
    };
}

enum class TrailingSeparator {
    Disallowed,
    Allowed,
    Required,
};

namespace basic {
    template <class Char>
    auto list(
        BasicParserLike<Char> auto&& elem_parser,
        BasicParserLike<Char> auto&& separator_parser,
        TrailingSeparator trailing_sep = TrailingSeparator::Allowed,
        size_t min_elem_count = 0
    ) -> BasicParserLike<Char> auto {
        auto parse = [elem_parser = std::move(elem_parser),
                      separator_parser = std::move(separator_parser),
                      trailing_sep,
                      min_elem_count](std::basic_string_view<Char> src) {
            using Elem = decltype(elem_parser.parse(src).get_value());
            using Value = std::vector<Elem>;

            auto values = Value{};
            auto prev_tail = std::basic_string_view<Char>{};
            auto tail = src;
            auto empty_result = BasicParseResult<Value, Char>{
                .value = std::nullopt,
                .tail = src,
            };

            if (auto first_result = elem_parser.parse(src); first_result.ok()) {
                prev_tail = tail;
                tail = first_result.tail;
                values.emplace_back(std::move(first_result).get_value());

                while (true) {
                    auto sep_result = separator_parser.parse(tail);

                    if (!sep_result.ok()) {
                        if (TrailingSeparator::Required == trailing_sep) {
                            values.pop_back();
                            tail = prev_tail;
                        }

                        break;
                    }

                    prev_tail = tail;
                    tail = sep_result.tail;

                    auto elem_result = elem_parser.parse(tail);

                    if (!elem_result.ok()) {
                        if (TrailingSeparator::Disallowed == trailing_sep) {
                            tail = prev_tail;
                        }

                        break;
                    }

                    prev_tail = tail;
                    tail = elem_result.tail;

                    values.emplace_back(std::move(elem_result).get_value());
                }
            }

            if (values.size() < min_elem_count) {
                return empty_result;
            } else {
                return BasicParseResult<Value, Char>{
                    .value = std::move(values),
                    .tail = tail,
                };
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }
}  // namespace basic

auto list(
    ParserLike auto&& elem_parser, ParserLike auto&& separator_parser,
    TrailingSeparator trailing_sep = TrailingSeparator::Allowed,
    size_t min_elem_count = 0
) -> ParserLike auto {
    return basic::list<char>(
        std::move(elem_parser), std::move(separator_parser), trailing_sep,
        min_elem_count
    );
}

}  // namespace comb
