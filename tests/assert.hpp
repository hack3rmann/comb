#pragma once

#include <fmt/format.h>

#define comb_assert(expr)                                                    \
    ({                                                                        \
        if (!(expr)) {                                                        \
            throw std::runtime_error(fmt::format(                             \
                "test '{}' failed: assetion '{}' failed", __FUNCTION__, #expr \
            ));                                                               \
        }                                                                     \
    })

#define comb_assert_eq(left, right)                                   \
    ({                                                                 \
        auto const left_result = (left);                               \
        auto const right_result = (right);                             \
                                                                       \
        if (left_result != right_result) {                             \
            throw std::runtime_error(fmt::format(                      \
                "test '{}' failed: assertion '{} == {}' "              \
                "failed with left = '{}' and right = '{}'",            \
                __FUNCTION__, #left, #right, left_result, right_result \
            ));                                                        \
        }                                                              \
    })

#define comb_assert_ne(left, right)                                   \
    ({                                                                 \
        auto const left_result = (left);                               \
        auto const right_result = (right);                             \
                                                                       \
        if (left_result == right_result) {                             \
            throw std::runtime_error(fmt::format(                      \
                "test '{}' failed: assertion '{} != {}' "              \
                "failed with left = '{}' and right = '{}'",            \
                __FUNCTION__, #left, #right, left_result, right_result \
            ));                                                        \
        }                                                              \
    })
