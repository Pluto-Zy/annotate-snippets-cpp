#ifndef ANNOTATE_SNIPPETS_DETAIL_DIAG_LEVEL_HPP
#define ANNOTATE_SNIPPETS_DETAIL_DIAG_LEVEL_HPP

#include <concepts>
#include <string_view>

namespace ants::detail {
/// Checks if LevelTy has a member display_string() function to convert it to a corresponding
/// displayable string.
template <class LevelTy>
concept has_member_display_string = requires(LevelTy&& level) {
    { level.display_string() } -> std::convertible_to<std::string_view>;
};

/// Checks whether the display_string() function can be found via ADL with an argument of type
/// LevelTy, which is used to convert the level to its corresponding displayable string.
template <class LevelTy>
concept has_adl_display_string = !has_member_display_string<LevelTy> && requires(LevelTy&& level) {
    { display_string(level) } -> std::convertible_to<std::string_view>;
};

struct DisplayStringImpl {
    template <has_member_display_string LevelTy>
    constexpr auto operator()(LevelTy&& level) const -> std::string_view {
        return static_cast<std::string_view>(level.display_string());
    }

    template <has_adl_display_string LevelTy>
    constexpr auto operator()(LevelTy&& level) const -> std::string_view {
        return static_cast<std::string_view>(display_string(level));
    }

    void operator()(auto&&) const = delete;
};

/// A customization point object used to convert the level object to a string displayable on the
/// screen.
constexpr inline DisplayStringImpl level_display_string {};

/// Checks whether type Ty can be used as the level type for diagnostic information.
template <class Ty>
concept diagnostic_level =
    std::regular<Ty> && requires(Ty&& level) { level_display_string(level); };
}  // namespace ants::detail

#endif  // ANNOTATE_SNIPPETS_DETAIL_DIAG_LEVEL_HPP
