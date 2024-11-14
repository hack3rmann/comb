#pragma once

#include <string_view>
#include <optional>
#include <vector>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <type_traits>

namespace comb {

template <class T, class Char>
struct BasicParseResult {
    std::optional<T> value;
    std::basic_string_view<Char> tail;

    inline auto constexpr ok(this BasicParseResult const& self) -> bool {
        return self.value.has_value();
    }

    template <class Self>
    inline auto constexpr get_value(this Self&& self) {
        return std::forward<Self>(self).value.value();
    }

    friend inline auto constexpr operator|(
        BasicParseResult&& lhs, BasicParseResult&& rhs
    ) -> BasicParseResult {
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

template <class T>
concept NotVoid = !std::same_as<T, void>;

template <class T, class Input, class Char>
concept BasicTransformMap = requires(T transform, Input input) {
    {
        transform(input)
    } -> NotVoid;
};

template <class T, class Input>
concept TransformMap = BasicTransformMap<T, Input, char>;

template <class T, class Input, class Char>
concept BasicFilterPredicate = requires(T predicate, Input input) {
    {
        predicate(input)
    } -> std::same_as<bool>;
};

template <class T, class Input>
concept FilterPredicate = BasicFilterPredicate<T, Input, char>;

template <class T, class Char>
    requires BasicParseFunction<T, Char>
struct BasicParser {
    T parse;

    using ParseValue =
        decltype(parse(std::basic_string_view<Char>{}).get_value());

    template <class S>
    using ParserChar = BasicParser<S, Char>;

    template <class Self>
    inline auto constexpr operator()(
        this Self&& self, std::basic_string_view<Char> src
    ) -> BasicParseResult<ParseValue, Char> {
        return std::forward<Self>(self).parse(src);
    }

    template <BasicParseFunction<Char> S>
    friend inline auto constexpr operator|(
        BasicParser<T, Char> lhs, BasicParser<S, Char> rhs
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[lhs = std::move(lhs), rhs = std::move(rhs)](
                              std::basic_string_view<Char> src
                          ) {
            auto left_result = lhs.parse(src);

            if (left_result.ok()) {
                return std::move(left_result);
            } else {
                auto right_result = rhs.parse(src);
                return std::move(right_result);
            }
        }};
    }

    template <BasicParseFunction<Char> S>
    friend inline auto constexpr operator&(
        BasicParser<T, Char> lhs, BasicParser<S, Char> rhs
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[lhs = std::move(lhs), rhs = std::move(rhs)](
                              std::basic_string_view<Char> src
                          ) {
            using PairValue = std::pair<
                typename decltype(lhs)::ParseValue,
                typename decltype(rhs)::ParseValue>;

            auto left_result = lhs.parse(src);

            if (!left_result.ok()) {
                return BasicParseResult<PairValue, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };
            }

            auto right_result = rhs.parse(left_result.tail);

            if (!right_result.ok()) {
                return BasicParseResult<PairValue, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };
            }

            return BasicParseResult<PairValue, Char>{
                .value = std::make_optional<PairValue>(std::make_pair(
                    std::move(left_result).get_value(),
                    std::move(right_result).get_value()
                )),
                .tail = right_result.tail,
            };
        }};
    }

    template <BasicParseFunction<Char> S>
    friend inline auto constexpr operator>>(
        BasicParser<T, Char> lhs, BasicParser<S, Char> rhs
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[lhs = std::move(lhs), rhs = std::move(rhs)](
                              std::basic_string_view<Char> src
                          ) {
            using RightValue = typename decltype(rhs)::ParseValue;

            auto left_result = lhs.parse(src);

            if (!left_result.ok()) {
                return BasicParseResult<RightValue, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };
            } else {
                auto right_result = rhs.parse(left_result.tail);
                return std::move(right_result);
            }
        }};
    }

    template <BasicParseFunction<Char> S>
    friend inline auto constexpr operator<<(
        BasicParser<T, Char> lhs, BasicParser<S, Char> rhs
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[lhs = std::move(lhs),
                           rhs = std::move(rhs)](std::string_view src) {
            using LeftValue = typename decltype(lhs)::ParseValue;

            auto left_result = lhs.parse(src);

            if (!left_result.ok()) {
                return left_result;
            }

            auto right_result = rhs.parse(left_result.tail);

            if (right_result.ok()) {
                return BasicParseResult<LeftValue, Char>{
                    .value = std::move(left_result.value),
                    .tail = right_result.tail,
                };
            } else {
                return BasicParseResult<LeftValue, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };
            }
        }};
    }

    inline auto constexpr map(
        this BasicParser self,
        BasicTransformMap<ParseValue, Char> auto transform
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[self = std::move(self),
                           transform = std::move(transform
                           )](std::basic_string_view<Char> src) {
            auto result = self.parse(src);

            using NewType = decltype(transform(result.get_value()));

            if (result.ok()) {
                return BasicParseResult<NewType, Char>{
                    .value = std::make_optional<NewType>(
                        transform(std::move(result).get_value())
                    ),
                    .tail = result.tail,
                };
            } else {
                return BasicParseResult<NewType, Char>{
                    .value = std::nullopt,
                    .tail = result.tail,
                };
            }
        }};
    }

    inline auto constexpr map_result(
        this BasicParser self,
        BasicTransformMap<BasicParseResult<ParseValue, Char>, Char> auto
            transform
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[self = std::move(self),
                           transform = std::move(transform)](
                              std::basic_string_view<Char> src
                          ) { return transform(self.parse(src)); }};
    }

    inline auto constexpr repeat(this BasicParser self, size_t min_count = 0)
        -> BasicParserLike<Char> auto {
        return ParserChar{[self = std::move(self),
                           min_count](std::basic_string_view<Char> src) {
            using Sequence = std::vector<ParseValue>;

            auto result_sequence = Sequence{};
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
                    .value =
                        std::make_optional<Sequence>(std::move(result_sequence)
                        ),
                    .tail = tail,
                };
            }
        }};
    }

    inline auto constexpr opt(this BasicParser self)
        -> BasicParserLike<Char> auto {
        return ParserChar{[self = std::move(self
                           )](std::basic_string_view<Char> src) {
            using Value = std::optional<decltype(self.parse(src).get_value())>;

            auto result = self.parse(src);

            return BasicParseResult<Value, Char>{
                .value = std::make_optional<Value>(std::move(result).value),
                .tail = result.tail,
            };
        }};
    }

    inline auto constexpr opt_default(this BasicParser self)
        -> BasicParserLike<Char> auto
        requires std::is_default_constructible_v<ParseValue>
    {
        return ParserChar{[self = std::move(self
                           )](std::basic_string_view<Char> src) {
            auto result = self.parse(src);

            if (result.ok()) {
                return std::move(result);
            } else {
                return BasicParseResult<ParseValue, Char>{
                    .value = std::make_optional<ParseValue>(),
                    .tail = src,
                };
            }
        }};
    }

    inline auto constexpr opt_value(this BasicParser self, ParseValue value)
        -> BasicParserLike<Char> auto {
        return ParserChar{[self = std::move(self), value = std::move(value)](
                              std::basic_string_view<Char> src
                          ) {
            auto result = self.parse(src);

            if (result.ok()) {
                return std::move(result);
            } else {
                return BasicParseResult<ParseValue, Char>{
                    .value = std::make_optional<ParseValue>(std::move(value)),
                    .tail = src,
                };
            }
        }};
    }

    inline auto constexpr take_if(
        this BasicParser self,
        BasicFilterPredicate<ParseValue const&, Char> auto predicate
    ) -> BasicParserLike<Char> auto {
        return ParserChar{[self = std::move(self),
                           predicate = std::move(predicate
                           )](std::basic_string_view<Char> src) {
            auto result = self.parse(src);

            if (result.ok() && predicate(result.get_value())) {
                return std::move(result);
            } else {
                return BasicParseResult<ParseValue, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };
            }
        }};
    }
};

template <class T>
using Parser = BasicParser<T, char>;

namespace basic {
    template <class Char>
    inline auto constexpr character(Char value) -> BasicParserLike<Char> auto {
        return Parser{
            [value](std::basic_string_view<Char> src
            ) -> BasicParseResult<Char, Char> {
                if (src.empty() || src[0] != value) {
                    return BasicParseResult<Char, Char>{
                        .value = std::nullopt, .tail = src
                    };
                } else {
                    src.remove_prefix(1);
                    return BasicParseResult<Char, Char>{
                        .value = value, .tail = src
                    };
                }
            }
        };
    }
}  // namespace basic

inline auto constexpr character(char value) -> ParserLike auto {
    return basic::character<char>(value);
}

// FIXME(hack3rmann): remove this definition
inline auto constexpr is_whitespace(char value) -> bool {
    return (9 <= value && value <= 13) || 32 == value;
}

namespace basic {
    template <class Char>
    struct Prefix {
        template <class T>
        using ParserChar = BasicParser<T, Char>;

        inline static auto constexpr prefix(std::basic_string_view<Char> match)
            -> BasicParserLike<Char> auto {
            return ParserChar{
                [match](std::basic_string_view<Char> src
                ) -> BasicParseResult<std::basic_string_view<Char>, Char> {
                    if (src.size() < match.size() || !src.starts_with(match)) {
                        return BasicParseResult<
                            std::basic_string_view<Char>, Char>{
                            .value = std::nullopt, .tail = src
                        };
                    } else {
                        auto head = src;
                        head.remove_suffix(src.size() - match.size());
                        src.remove_prefix(match.size());

                        return BasicParseResult<
                            std::basic_string_view<Char>, Char>{
                            .value = head, .tail = src
                        };
                    }
                }
            };
        }
    };

    template <class Char>
    auto constexpr prefix = Prefix<Char>::prefix;
}  // namespace basic

auto constexpr prefix = basic::prefix<char>;

inline auto constexpr integer(uint32_t radix = 10) -> ParserLike auto {
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

inline auto constexpr floating() -> ParserLike auto {
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

inline auto constexpr whitespace(uint32_t min_count = 0) -> ParserLike auto {
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

inline auto constexpr newline() -> ParserLike auto {
    return prefix("\r\n") | prefix("\n") | prefix("\r");
}

inline auto constexpr quoted_string(char quote_symbol = '"') -> ParserLike
    auto {
    return Parser{
        [quote_symbol](std::string_view src) -> ParseResult<std::string_view> {
            auto open_quote = character(quote_symbol).parse(src);
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
    struct List {
        template <class T>
        using ParserChar = BasicParser<T, Char>;

        static auto constexpr list(
            BasicParserLike<Char> auto elem_parser,
            BasicParserLike<Char> auto separator_parser,
            TrailingSeparator trailing_sep = TrailingSeparator::Allowed,
            size_t min_elem_count = 0
        ) -> BasicParserLike<Char> auto {
            return ParserChar{[elem_parser = std::move(elem_parser),
                               separator_parser = std::move(separator_parser),
                               trailing_sep,
                               min_elem_count](std::basic_string_view<Char> src
                              ) {
                using Elem = decltype(elem_parser.parse(src).get_value());
                using Value = std::vector<Elem>;

                auto values = Value{};
                auto prev_tail = std::basic_string_view<Char>{};
                auto tail = src;
                auto empty_result = BasicParseResult<Value, Char>{
                    .value = std::nullopt,
                    .tail = src,
                };

                if (auto first_result = elem_parser.parse(src);
                    first_result.ok())
                {
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
            }};
        }
    };

    template <class Char>
    auto constexpr list(
        BasicParserLike<Char> auto elem_parser,
        BasicParserLike<Char> auto separator_parser,
        TrailingSeparator trailing_sep = TrailingSeparator::Allowed,
        size_t min_elem_count = 0
    ) -> BasicParserLike<Char> auto {
        return List<Char>::list(
            std::move(elem_parser), std::move(separator_parser), trailing_sep,
            min_elem_count
        );
    }
}  // namespace basic

auto constexpr list(
    ParserLike auto elem_parser, ParserLike auto separator_parser,
    TrailingSeparator trailing_sep = TrailingSeparator::Allowed,
    size_t min_elem_count = 0
) -> ParserLike auto {
    return basic::List<char>::list(
        std::move(elem_parser), std::move(separator_parser), trailing_sep,
        min_elem_count
    );
}

}  // namespace comb
