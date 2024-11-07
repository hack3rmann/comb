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

    friend auto operator|(BasicParseResult&& lhs, BasicParseResult&& rhs) -> BasicParseResult {
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
struct BasicParser {
    T parse;

    template <class Self>
    auto operator()(this Self&& self, std::basic_string_view<Char> src) {
        return std::forward<Self>(self).parse(src);
    }

    template <class S>
    friend auto operator|(BasicParser<T, Char>&& lhs, BasicParser<S, Char>&& rhs) {
        auto parse = [=](std::string_view src) {
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

    template <class S>
    friend auto operator>>(BasicParser<T, Char>&& lhs, BasicParser<S, Char>&& rhs) {
        auto parse = [=](std::string_view src) {
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

    template <class S>
    friend auto operator<<(BasicParser<T, Char>&& lhs, BasicParser<S, Char>&& rhs) {
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

    template <class F>
    auto map(this BasicParser&& self, F transform) {
        auto parse = [=](std::string_view src) {
            auto result = (self) (src);

            using NewType = decltype(transform(result.get_value()));

            if (result.ok()) {
                return ParseResult<NewType>{
                    .value = transform(std::move(result.get_value())),
                    .tail = result.tail
                };
            } else {
                return ParseResult<NewType>{
                    .value = std::nullopt, .tail = result.tail
                };
            }
        };

        return BasicParser<decltype(parse), Char>{std::move(parse)};
    }

    auto sequence(this BasicParser&& self, size_t min_count = 0) {
        auto parse = [=](std::string_view src) {
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
                return ParseResult<Sequence>{
                    .value = std::nullopt,
                    .tail = src,
                };
            } else {
                return ParseResult<Sequence>{
                    .value = std::move(result_sequence),
                    .tail = tail,
                };
            }
        };

        return BasicParser<decltype(parse), Char>(std::move(parse));
    }
};

template <class T>
using Parser = BasicParser<T, char>;

inline auto character(char value) {
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

inline auto prefix(std::string_view match) {
    return Parser{
        [match](std::string_view src) -> ParseResult<std::string_view> {
            if (src.size() < match.size() || !src.starts_with(match)) {
                return ParseResult<std::string_view>{
                    .value = std::nullopt, .tail = src
                };
            } else {
                auto head = src;
                head.remove_suffix(src.size() - match.size());
                src.remove_prefix(match.size());

                return ParseResult<std::string_view>{
                    .value = head, .tail = src
                };
            }
        }
    };
}

inline auto integer(uint32_t radix = 10) {
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

inline auto whitespace(uint32_t min_count = 0) {
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

inline auto newline() {
    return prefix("\r\n") | prefix("\n") | prefix("\r");
}

inline auto quoted_string() {
    return Parser{[](std::string_view src) -> ParseResult<std::string_view> {
        auto const open_quote = character('"').parse(src);
        auto tail = open_quote.tail;

        if (!open_quote.ok()) {
            return ParseResult<std::string_view>{
                .value = std::nullopt, .tail = src
            };
        }

        auto n_string_symbols = size_t{0};

        for (auto symbol : tail) {
            if ('"' == symbol) {
                break;
            }

            n_string_symbols += 1;
        }

        if (n_string_symbols == tail.size() || '"' != tail[n_string_symbols]) {
            return ParseResult<std::string_view>{
                .value = std::nullopt, .tail = src
            };
        }

        auto match = tail;
        match.remove_suffix(src.size() - 1 - n_string_symbols);
        tail.remove_prefix(n_string_symbols + 1);

        return ParseResult<std::string_view>{.value = match, .tail = tail};
    }};
}

}  // namespace comb
