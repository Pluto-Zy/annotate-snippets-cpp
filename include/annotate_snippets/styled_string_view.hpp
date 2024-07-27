#ifndef ANNOTATE_SNIPPETS_STYLED_STRING_VIEW_HPP
#define ANNOTATE_SNIPPETS_STYLED_STRING_VIEW_HPP

#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "style.hpp"

#include <concepts>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
/// Represents a styled (multi-line) string view. Different parts of the string can have different
/// rendering styles.
///
/// Note that `StyledStringView` does *not* take ownership of the underlying string.
class StyledStringView : public detail::StyledStringImpl {
    using Base = detail::StyledStringImpl;

public:
    /// Constructs an empty `StyledStringView`.
    StyledStringView() = default;

    /// Constructs a `StyledStringView` whose content is `content` and the style of the whole string
    /// will be inferred from the context in which the string is used (i.e. the `Style::Auto`
    /// style).
    StyledStringView(std::string_view content) : StyledStringView(content, Style::Auto) { }

    /// Constructs a `StyledStringView` where its content is built from `args...`, as if constructed
    /// with `std::string_view(std::forward<Args>(args)...)`, and the style of the whole string will
    /// be inferred from the context in which the string is used (i.e. the `Style::Auto` style).
    template <class... Args>
        requires std::constructible_from<std::string_view, Args...>
    StyledStringView(Args&&... args) :
        StyledStringView(std::string_view(std::forward<Args>(args)...)) { }

    /// Constructs a `StyledStringView` whose content is `content` and the style of the whole string
    /// is `style`.
    StyledStringView(std::string_view content, Style style) :
        Base(content.size(), style), content_(content) { }

    /// Constructs a `StyledStringView` whose content is `content` and the style of the whole string
    /// is `style`. This function has the same effect as the corresponding constructor.
    static auto styled(std::string_view content, Style style) -> StyledStringView {
        return { content, style };
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

    using Base::set_style;

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
            { .start_index = content_.size(), .style {} },
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
    auto styled_line_parts() const -> std::vector<std::vector<StyledStringViewPart>> {
        return Base::styled_line_parts(content_);
    }

private:
    std::string_view content_;

    friend class StyledString;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_STYLED_STRING_VIEW_HPP
