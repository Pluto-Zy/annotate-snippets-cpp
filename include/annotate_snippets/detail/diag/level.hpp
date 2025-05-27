#ifndef ANNOTATE_SNIPPETS_DETAIL_DIAG_LEVEL_HPP
#define ANNOTATE_SNIPPETS_DETAIL_DIAG_LEVEL_HPP

#include <string_view>
#include <type_traits>

namespace ants::detail {
/// Checks if `LevelTy` has a member `title()` function to convert it to a corresponding displayable
/// string.
template <class LevelTy, class = void>
struct has_member_title : std::false_type { };  // NOLINT(readability-identifier-naming)

template <class LevelTy>
struct has_member_title<LevelTy, std::void_t<decltype(std::declval<LevelTy const&>().title())>> :
    std::is_convertible<decltype(std::declval<LevelTy const&>().title()), std::string_view> { };

/// Checks whether the `title()` function can be found via ADL with an argument of type `LevelTy`,
/// which is used to convert the level to its corresponding displayable string.
template <class LevelTy, class = void>
struct has_adl_title : std::false_type { };  // NOLINT(readability-identifier-naming)

template <class LevelTy>
struct has_adl_title<LevelTy, std::void_t<decltype(title(std::declval<LevelTy const&>()))>> :
    std::is_convertible<decltype(title(std::declval<LevelTy const&>())), std::string_view> { };

struct LevelTitleImpl {
    template <class LevelTy, std::enable_if_t<has_member_title<LevelTy>::value, int> = 0>
    constexpr auto operator()(LevelTy const& level) const -> std::string_view {
        return static_cast<std::string_view>(level.title());
    }

    template <
        class LevelTy,
        std::enable_if_t<
            std::conjunction_v<has_adl_title<LevelTy>, std::negation<has_member_title<LevelTy>>>,
            int> = 0>
    constexpr auto operator()(LevelTy const& level) const -> std::string_view {
        return static_cast<std::string_view>(title(level));
    }
};

/// A customization point object used to convert the level object to a string displayable on the
/// screen.
constexpr inline LevelTitleImpl level_title {};

/// Checks whether type `Ty` can be used as the diagnostic level type for diagnostic information.
template <class Ty>
constexpr bool is_diagnostic_level = std::disjunction_v<has_member_title<Ty>, has_adl_title<Ty>>;
}  // namespace ants::detail

#endif  // ANNOTATE_SNIPPETS_DETAIL_DIAG_LEVEL_HPP
