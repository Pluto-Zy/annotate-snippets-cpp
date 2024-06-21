#ifndef ANNOTATE_SNIPPETS_STYLED_STRING_HPP
#define ANNOTATE_SNIPPETS_STYLED_STRING_HPP

#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "annotate_snippets/style.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace ants {
class StyledString : public detail::StyledStringImpl {
    using Base = detail::StyledStringImpl;

public:
    /// Constructs an empty `StyledString`.
    StyledString() = default;

    /// Constructs a `StyledString` whose content is `content` and the style of the whole string is
    /// `style`.
    StyledString(std::string content, Style style) :
        Base(content.size(), style), content_(std::move(content)) { }

    /// Constructs a `StyledString` whose content is `content` and the style of the whole string is
    /// `style`. This function has the same effect as the corresponding constructor.
    static auto styled(std::string content, Style style) -> StyledString {
        return { std::move(content), style };
    }

    /// Constructs a `StyledString` whose content is `content` and the style of the whole string
    /// will be inferred from the context in which the string is used (i.e. the `Style::Auto`
    /// style).
    static auto inferred(std::string content) -> StyledString {
        return styled(std::move(content), Style::Auto);
    }

    /// Constructs a `StyledString` whose content is `content` with no style (i.e. the
    /// `Style::Default` style). It will be rendered as the default style of the output environment.
    static auto plain(std::string content) -> StyledString {
        return styled(std::move(content), Style::Default);
    }

    auto content() const -> std::string const& {
        return content_;
    }

    auto content() -> std::string& {
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
    ) & -> StyledString& {
        set_style(style, start_index, end_index);
        return *this;
    }

    auto with_style(
        Style style,
        std::size_t start_index,
        std::size_t end_index
    ) && -> StyledString&& {
        set_style(style, start_index, end_index);
        return std::move(*this);
    }

    auto with_style(Style style, std::size_t start_index) & -> StyledString& {
        set_style(style, start_index);
        return *this;
    }

    auto with_style(Style style, std::size_t start_index) && -> StyledString&& {
        set_style(style, start_index);
        return std::move(*this);
    }

    auto with_style(Style style) & -> StyledString& {
        set_style(style);
        return *this;
    }

    auto with_style(Style style) && -> StyledString&& {
        set_style(style);
        return std::move(*this);
    }

    /// Appends the string `content` to the end of the current `StyledString` with the specified
    /// `style`. Existing parts of the `StyledString` remain unaffected.
    void append(std::string_view content, Style style) {
        content_.append(content);
        append_styled_part_impl(style);
    }

    /// Appends the string `content` to the end of the current `StyledString` with the specified
    /// `style`. If `style` is `Style::Auto`, it is changed to `auto_replacement`. Existing parts of
    /// the `StyledString` remain unaffected.
    void append(std::string_view content, Style style, Style auto_replacement) {
        append(content, style.is_auto_style() ? auto_replacement : style);
    }

    /// Appends the sequence of styled strings specified by `parts` to the end of the current
    /// `StyledString`. This method is typically used to add a `StyledStringView` or `StyledString`
    /// line by line to the current string.
    void append(std::vector<StyledStringViewPart> const& parts) {
        for (StyledStringViewPart const& part : parts) {
            append(part.content, part.style);
        }
    }

    /// Appends the sequence of styled strings specified by `parts` to the end of the current
    /// `StyledString`. If the style of any part is `Style::Auto`, it is replaced with
    /// `auto_replacement`. This method is typically used to add a `StyledStringView` or
    /// `StyledString` line by line to the current string.
    void append(std::vector<StyledStringViewPart> const& parts, Style auto_replacement) {
        for (StyledStringViewPart const& part : parts) {
            append(part.content, part.style, auto_replacement);
        }
    }

    /// Appends a newline character at the end of the string to ensure that subsequent additions
    /// begin on a new line.
    void append_newline() {
        content_.push_back('\n');
        // We simply set the style of the newline character to match the style of the character
        // before it, as the style of the newline character is not observable in the final
        // rendering. In other words, this approach does not affect the results of
        // `styled_line_parts()`.
        styled_parts_.back().start_index = content_.size();
    }

    /// Appends `count` number of spaces at the end of the string, with each space styled as
    /// `Style::Default`.
    void append_spaces(std::size_t count) {
        if (count != 0) {
            content_.append(count, ' ');
            append_styled_part_impl(ants::Style::Default);
        }
    }

    /// Splits `content` into several `StyledStringPart`s by line and style, and puts substrings
    /// consisting of consecutive characters of the same style into one `StyledStringPart`. If there
    /// are multiple lines in a substring, splits each line into a separate `StyledStringPart`.
    ///
    /// Example:
    ///
    ///     StyledString::inferred("Hello\nWorld")
    ///         .with_style(Style::Highlight, 2, 8)
    ///         .styled_line_parts()
    ///
    ///     returns
    ///     {
    ///         { { "He", Style::Auto }, { "llo", Style::Highlight } }  // First line
    ///         { { "Wo", Style::Highlight }, { "rld", Style::Auto } }  // Second line
    ///     }
    ///
    /// Note that the returned `StyledStringViewPart` does not take ownership of the underlying
    /// string. Once the `StyledString` is destroyed, the returned value becomes invalid.
    ///
    /// @return The first level of the return value array represents the lines in `content`, and the
    /// second level saves the `StyledStringPart`s consisting of consecutive characters of the same
    /// style contained in the same line.
    auto styled_line_parts() const -> std::vector<std::vector<StyledStringViewPart>> {
        return Base::styled_line_parts(content_);
    }

private:
    std::string content_;

    /// Adds a new item to the existing `styled_parts_` array, so that the part of `content_` not
    /// covered by `styled_parts_` has the style `style`.
    ///
    /// The usual usage is to first expand `content_`, and then use this method to set the style of
    /// the newly added part to `style`.
    void append_styled_part_impl(Style style) {
        if (styled_parts_.back().start_index == 0) {
            // If the original string is empty, `styled_parts_` contains two elements with
            // `start_index` both set to 0; we need to remove one of them.
            styled_parts_.resize(1);
        }

        styled_parts_.back().style = style;
        // Insert a new style part.
        styled_parts_.push_back(StyledPart { .start_index = content_.size(), .style {} });
    }
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_STYLED_STRING_HPP
