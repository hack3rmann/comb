# `comb` parser combinators (C++)

## Brief

`comb` is a single header file library with implementation of parser combinators.

## How to install

### With CMake

Add to your CMakeLists.txt:

```cmake
# Include `FetchContent` if it's not present
include(FetchContent)

set(COMB_VERSION 0.1.0)
# FetchContent_Declare(
#     comb
#     DOWNLOAD_EXTRACT_TIMESTAMP OFF
#     URL https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.tar.gz)
FetchContent_Declare(
    comb
    GIT_REPOSITORY https://github.com/hack3rmann/comb.git
    GIT_TAG 43d3d8d7c5f39b4a126e2efc554ce272079821ad)

FetchContent_MakeAvailable(comb)
```

It will fetch `comb` at build time.

### Simple copying

Because `comb` is *single header* you can just copy [comb/parse.hpp](/comb/parse.hpp) to your project.
No dependencies needed.

## Examples

Parse key-value arguments.

```cpp
#include <cassert>
#include <comb/parse.hpp>

auto main() -> int {
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
```

Parse enumeration values and implement your own parser.

```cpp
#include <cassert>
#include <comb/parse.hpp>

// parses '$(name)' with any given $(name)
auto single_quoted_name(std::string_view name) {
    return character('\'') >> prefix(name) << character('\'');
}

auto main() -> int {
    auto constexpr SOURCE = "'first' 'second'  'third'\t 'second'";

    enum class Variant {
        First, Second, Third,
    };

    // .map transforms parsed string (if there is any) to returned value
    // | means "parse left or parse right". Note that it is short-circuited
    auto variant_parser
        = single_quoted_name("first").map([](auto parsed) { return Variant::First; })
        | single_quoted_name("second").map([](auto parsed) { return Variant::Second; })
        | single_quoted_name("third").map([](auto parsed) { return Variant::Third; });

    // repeat this parser separated by any number of whitespaces
    auto parser = (std::move(variant_parser) << whitespace()).sequence();

    auto result = parser.parse(SOURCE);
    assert(result.ok());

    auto variants = std::move(result).get_value();
    assert(variants == (std::vector<Variant>{
        Variant::First, Variant::Second, Variant::Third, Variant::Second
    }));
}
```
