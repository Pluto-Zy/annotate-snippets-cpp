#ifndef ANNOTATE_SNIPPETS_STYLED_STRING_VIEW_HPP
#define ANNOTATE_SNIPPETS_STYLED_STRING_VIEW_HPP

#include "style.hpp"

#include <string_view>
#include <vector>

namespace ants {
/// Represents a part of a `StyledStringView`. The part has content `content`, and the style of the
/// part is `style`.
///
/// Objects of this class can be obtained through `StyledStringView::styled_line_parts()`.
struct StyledStringPart {
    std::string_view content;
    Style style;

    auto operator==(StyledStringPart const& other) const -> bool = default;
};

/// Represents a styled (multi-line) string view. Different parts of the string can have different
/// rendering styles.
///
/// Note that `StyledStringView` does *not* take ownership of the underlying string.
class StyledStringView {
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
    /// Constructs an empty `StyledStringView`.
    StyledStringView() :
        // clang-format off
        content_(),
        styled_parts_ {
            { .start_index = 0, .style = Style::Auto },
            { .start_index = 0, .style = Style::Auto },
        } { }
    // clang-format on

    /// Constructs a `StyledStringView` whose content is `content` and the style of the whole string
    /// is `style`.
    static auto styled(std::string_view content, Style style) -> StyledStringView {
        // clang-format off
        return {
            /*content=*/content,
            /*parts=*/{
                // We need at least two `StyledPart` elements to specify the style of the whole
                // string.
                { .start_index = 0, .style = style },
                { .start_index = content.size(), .style = Style::Auto },
            },
        };
        // clang-format on
    }

    /// Constructs a `StyledStringView` whose content is `content` and the style of the whole string
    /// will be inferred from the context in which the string is used (i.e. the `Style::Auto`
    /// style).
    static auto inferred(std::string_view content) -> StyledStringView {
        return styled(content, Style::Auto);
    }

    /// Constructs a `StyledStringView` whose content is `content` with no style (i.e. the
    /// `Style::Default` style). It will be rendered as the default style of the output environment.
    static auto plain(std::string_view content) -> StyledStringView {
        return styled(content, Style::Default);
    }

    auto content() const -> std::string_view {
        return content_;
    }

    auto empty() const -> bool {
        return content_.empty();
    }

    /// Sets the style of the substring in range `[start_index, end_index)`. Any existing styles for
    /// the characters in this substring will be overwritten.
    void set_style(Style style, std::size_t start_index, std::size_t end_index);

    /// Sets the style of the substring starting at `start_index` and ending at the end of the whole
    /// string. Any existing styles for the characters in this substring will be overwritten.
    void set_style(Style style, std::size_t start_index) {
        set_style(style, start_index, content_.size());
    }

    /// Sets the style of the whole string.
    void set_style(Style style) {
        // clang-format off
        std::vector<StyledPart> {
            { .start_index = 0, .style = style },
            { .start_index = content_.size(), .style = Style::Auto },
        }
            .swap(styled_parts_);
        // clang-format on
    }

    auto with_style(
        Style style,
        std::size_t start_index,
        std::size_t end_index
    ) & -> StyledStringView& {
        set_style(style, start_index, end_index);
        return *this;
    }

    auto with_style(
        Style style,
        std::size_t start_index,
        std::size_t end_index
    ) && -> StyledStringView&& {
        set_style(style, start_index, end_index);
        return std::move(*this);
    }

    auto with_style(Style style, std::size_t start_index) & -> StyledStringView& {
        set_style(style, start_index);
        return *this;
    }

    auto with_style(Style style, std::size_t start_index) && -> StyledStringView&& {
        set_style(style, start_index);
        return std::move(*this);
    }

    auto with_style(Style style) & -> StyledStringView& {
        set_style(style);
        return *this;
    }

    auto with_style(Style style) && -> StyledStringView&& {
        set_style(style);
        return std::move(*this);
    }

    /// Splits `content` into several `StyledStringPart`s by line and style, and puts substrings
    /// consisting of consecutive characters of the same style into one `StyledStringPart`. If there
    /// are multiple lines in a substring, splits each line into a separate `StyledStringPart`.
    ///
    /// Example:
    ///
    ///     StyledStringView::inferred("Hello\nWorld")
    ///         .with_style(Style::Highlight, 2, 8)
    ///         .styled_line_parts()
    ///
    ///     returns
    ///     {
    ///         { { "He", Style::Auto }, { "llo", Style::Highlight } }  // First line
    ///         { { "Wo", Style::Highlight }, { "rld", Style::Auto } }  // Second line
    ///     }
    ///
    /// @return The first level of the return value array represents the lines in `content`, and the
    /// second level saves the `StyledStringPart`s consisting of consecutive characters of the same
    /// style contained in the same line.
    auto styled_line_parts() const -> std::vector<std::vector<StyledStringPart>>;

private:
    std::string_view content_;
    std::vector<StyledPart> styled_parts_;

    StyledStringView(std::string_view content, std::vector<StyledPart> parts) :
        content_(content), styled_parts_(std::move(parts)) { }
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_STYLED_STRING_VIEW_HPP
