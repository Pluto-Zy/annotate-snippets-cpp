#ifndef ANNOTATE_SNIPPETS_STYLED_STRING_HPP
#define ANNOTATE_SNIPPETS_STYLED_STRING_HPP

#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "annotate_snippets/style.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ants {
class StyledString : public detail::StyledStringImpl {
    using Base = detail::StyledStringImpl;

public:
    /// Constructs an empty `StyledString`.
    StyledString() = default;

    /// Constructs a `StyledString` whose content is `content` and the style of the whole string
    /// will be inferred from the context in which the string is used (i.e. the `Style::Auto`
    /// style).
    StyledString(std::string content) :
        // Here we must explicitly cast `Style::Auto` to the `Style` type, otherwise overload
        // resolution would select `StyledString(Args&&...)` instead of `StyledString(std::string,
        // Style)`. This is because `Style::Auto` can be implicitly converted to integer while
        // `Style` cannot.
        StyledString(std::move(content), static_cast<Style>(Style::Auto)) { }

    /// Constructs a `StyledString` where its content is built from `args...`, as if constructed
    /// with `std::string(std::forward<Args>(args)...)`, and the style of the whole string will be
    /// inferred from the context in which the string is used (i.e. the `Style::Auto` style).
    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<std::string, Args...>, int> = 0>
    StyledString(Args&&... args) : StyledString(std::string(std::forward<Args>(args)...)) { }

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

    auto operator[](std::size_t index) const -> char {
        return content_[index];
    }

    auto operator[](std::size_t index) -> char& {
        return content_[index];
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
            { /*start_index=*/ 0, /*style=*/ style },
            { /*start_index=*/ content_.size(), /*style=*/ {} },
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
            append_styled_part_impl(Style::Default);
        }
    }

    /// Uses the string `content` to overwrite characters starting from `position`, and sets the
    /// style of the newly replaced characters to `style`.
    ///
    /// Note that this function does not move other characters but replaces the substring in the
    /// range `[position, position + content.size())` with `content`, leaving characters in other
    /// positions unchanged.
    ///
    /// If the range of characters to be overwritten extends beyond the existing range of the
    /// string, the string will be expanded to accommodate `content`. If the target position
    /// `position` exceeds the current range of the string, the string will be extended by adding
    /// unstyled spaces to make it sufficiently long.
    void set_styled_content(std::size_t position, std::string_view content, Style style) {
        // Ensures there is enough space to insert `content`.
        if (content_.size() < position + content.size()) {
            append_spaces(position + content.size() - content_.size());
        }

        // We need to replace a substring starting at `position` with a length equal to
        // `content.size()`. We could use `replace()` for this:
        //
        //      content_.replace(position, content.size(), content);
        //
        // However, due to a GCC bug (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100366), g++
        // incorrectly assumes that this code causes overlapping `__builtin_memcpy` calls.
        // Therefore, we use `std::copy()` instead.
        std::copy(
            content.begin(),
            content.end(),
            std::next(content_.begin(), static_cast<int>(position))
        );
        set_style(style, position, position + content.size());
    }

    /// Uses the string `content` to overwrite characters starting from `position`, and sets the
    /// style of the newly replaced characters to `style`. If `style` is `Style::Auto`, it is
    /// replaced with `auto_replacement`.
    ///
    /// Note that this function does not move other characters but replaces the substring in the
    /// range `[position, position + content.size())` with `content`, leaving characters in other
    /// positions unchanged.
    ///
    /// If the range of characters to be overwritten extends beyond the existing range of the
    /// string, the string will be expanded to accommodate `content`. If the target position
    /// `position` exceeds the current range of the string, the string will be extended by adding
    /// unstyled spaces to make it sufficiently long.
    void set_styled_content(
        std::size_t position,
        std::string_view content,
        Style style,
        Style auto_replacement
    ) {
        set_styled_content(position, content, style.is_auto_style() ? auto_replacement : style);
    }

    /// Sequentially overwrites the string starting from `position` with all parts of
    /// `styled_content`. The style of the newly written string will be consistent with that of
    /// `styled_content`. This function is typically used to write a `StyledStringView` or
    /// `StyledString` into a specific position of the current string line by line.
    ///
    /// If the range of characters to be overwritten extends beyond the existing range of the
    /// string, the string will be expanded to accommodate `content`. If the target position
    /// `position` exceeds the current range of the string, the string will be extended by adding
    /// unstyled spaces to make it sufficiently long.
    void set_styled_content(
        std::size_t position,
        std::vector<StyledStringViewPart> const& styled_content
    ) {
        for (auto const& [content, style] : styled_content) {
            set_styled_content(position, content, style);
            position += content.size();
        }
    }

    /// Sequentially overwrites the string starting from `position` with all parts of
    /// `styled_content`. The style of the newly written string will be consistent with that of
    /// `styled_content`, except that any parts with `Style::Auto` will be replaced with
    /// `auto_replacement`. This function is typically used to write a `StyledStringView` or
    /// `StyledString` into a specific position of the current string line by line.
    ///
    /// If the range of characters to be overwritten extends beyond the existing range of the
    /// string, the string will be expanded to accommodate `content`. If the target position
    /// `position` exceeds the current range of the string, the string will be extended by adding
    /// unstyled spaces to make it sufficiently long.
    void set_styled_content(
        std::size_t position,
        std::vector<StyledStringViewPart> const& styled_content,
        Style auto_replacement
    ) {
        for (auto const& [content, style] : styled_content) {
            set_styled_content(position, content, style, auto_replacement);
            position += content.size();
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
        styled_parts_.push_back(StyledPart { /*start_index=*/content_.size(), /*style=*/ {} });
    }
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_STYLED_STRING_HPP
