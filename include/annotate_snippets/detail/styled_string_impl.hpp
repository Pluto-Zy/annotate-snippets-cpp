#ifndef ANNOTATE_SNIPPETS_DETAIL_STYLED_STRING_IMPL_HPP
#define ANNOTATE_SNIPPETS_DETAIL_STYLED_STRING_IMPL_HPP

#include "annotate_snippets/style.hpp"

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
/// Represents a part of a `StyledStringView` or `StyledString`. The part has content `content`, and
/// the style of the part is `style`.
///
/// Objects of this class can be obtained through the `styled_line_parts()` method of
/// `StyledStringView` and `StyledString`.
struct StyledStringViewPart {
    std::string_view content;
    Style style;

    auto operator==(StyledStringViewPart const& other) const -> bool = default;
};

namespace detail {
class StyledStringImpl {
protected:
    /// Internal storage for the styles of the different parts of a string.
    ///
    /// A single `StyledPart` object cannot be used to represent the style of a string; at least two
    /// `StyledPart` objects are required. For two adjacent `StyledPart` objects `p1` and `p2`, the
    /// style of the substrings in the range `[p1.start_index, p2.start_index)` of the string
    /// `content_` is `p1.style`.
    struct StyledPart {
        std::size_t start_index;
        Style style;
    };

public:
    /// Sets the style of the substring in range `[start_index, end_index)`. Any existing styles for
    /// the characters in this substring will be overwritten.
    void set_style(Style style, std::size_t start_index, std::size_t end_index);

protected:
    std::vector<StyledPart> styled_parts_;

    // clang-format off
    StyledStringImpl() : styled_parts_ {
        { .start_index = 0, .style {} }, { .start_index = 0, .style {} }
    } { }
    // clang-format on

    explicit StyledStringImpl(std::vector<StyledPart> parts) : styled_parts_(std::move(parts)) { }

    // clang-format off
    explicit StyledStringImpl(std::size_t content_size, Style content_style) : styled_parts_ { {
        // We need at least two `StyledPart` elements to specify the style of the whole string.
        { .start_index = 0, .style = content_style },
        { .start_index = content_size, .style {} },
    } } { }
    // clang-format on

    /// Splits `content` into several `StyledStringViewPart`s by line and style, and puts substrings
    /// consisting of consecutive characters of the same style into one `StyledStringViewPart`. If
    /// there are multiple lines in a substring, splits each line into a separate
    /// `StyledStringViewPart`.
    ///
    /// @return The first level of the return value array represents the lines in `content`, and the
    /// second level saves the `StyledStringViewPart`s consisting of consecutive characters of the
    /// same style contained in the same line.
    auto styled_line_parts(  //
        std::string_view content
    ) const -> std::vector<std::vector<StyledStringViewPart>>;
};
}  // namespace detail
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_DETAIL_STYLED_STRING_IMPL_HPP
