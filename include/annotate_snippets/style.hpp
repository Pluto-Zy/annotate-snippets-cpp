#ifndef ANNOTATE_SNIPPETS_STYLE_HPP
#define ANNOTATE_SNIPPETS_STYLE_HPP

#include <cstdint>

namespace ants {
/// Used to specify the rendering style of each part of the text in the diagnostic information.
class Style {
public:
    /// Some predefined styles that are used to render specific parts of the diagnostic information.
    /// Note that all predefined styles are stored as non-positive numbers, because positive numbers
    /// are reserved for user-defined styles.
    ///
    /// TODO: Add an example to illustrate these enumerators.
    enum PredefinedStyle : std::int8_t {
        /// Represents a style that is determined based on the context in which it is used. For
        /// example, the text in the primary label will be rendered with the style `PrimaryLabel`.
        /// The user cannot specify the actual rendering style of `Auto`, as it is always converted
        /// to the actual context-determined style first.
        Auto = 0,
        /// Represents a style that renders text using the output environment's default style. The
        /// user cannot specify the actual rendering style of `Default`, as it is typically
        /// implemented to clear the existing rendering style.
        Default,
        /// Represents the rendering style of the title of the primary diagnostic message, such as
        /// "error:", "warning:" and "help:".
        PrimaryTitle,
        /// Represents the rendering style of the titles of all secondary diagnostic messages, such
        /// as "error:", "warning:" and "help:".
        SecondaryTitle,
        /// Represents the rendering style of the title message of the primary diagnostic message,
        /// that is, the text following the title describing the error message.
        PrimaryMessage,
        /// Represents the rendering style of the title messages of all secondary diagnostic
        /// messages, that is, the text following the title describing the error message.
        SecondaryMessage,
        /// Represents the rendering style of the combination of origin and location in the
        /// diagnostic message. For example, "--> main.cpp:3:5".
        OriginAndLocation,
        /// Represents the rendering style of line numbers and line number separators. For example,
        /// "23 |".
        LineNumber,
        /// Represents the rendering style of the source code.
        SourceCode,
        /// Represents the rendering style of the underline of the primary annotation in the source
        /// code. For example:
        ///
        ///     vec.push_back(content);
        ///     ^^^           -------
        ///     |
        ///     PrimaryUnderline
        PrimaryUnderline,
        /// Represents the rendering style of the underlines of all secondary annotations in the
        /// source code. For example:
        ///
        ///     vec.push_back(content);
        ///     ^^^           -------
        ///                   |
        ///                   SecondaryUnderline
        SecondaryUnderline,
        /// Represents the rendering style of the label following the underline of the primary
        /// annotation in the source code.
        PrimaryLabel,
        /// Represents the rendering style of the labels following the underlines of all secondary
        /// annotation in the source code.
        SecondaryLabel,
        /// Represents the style of the text to be highlighted. This style is not inferred from
        /// `Auto` but is typically specified by the user.
        Highlight,
        /// Represents the style of texts to be added when rendering fix suggestions.
        Addition,
        /// Represents the style of texts to be removed when rendering fix suggestions.
        Removal,
    };

    constexpr Style() : style_(0) { }
    // We can implicitly convert `PredefinedStyle` to `Style`.
    constexpr Style(PredefinedStyle style) :
        style_(
            // Predefined styles are always stored as non-positive numbers,
            static_cast<std::int8_t>(-style)
        ) { }
    constexpr explicit Style(std::int8_t style) : style_(style) { }

    constexpr auto value() const -> std::int8_t {
        return style_;
    }

    constexpr auto is_predefined_style() const -> bool {
        return style_ <= 0;
    }

    constexpr auto is_user_defined_style() const -> bool {
        return !is_predefined_style();
    }

    /// Converts a `Style` to a `Style::PredefinedStyle`. Note that this method assumes that the
    /// current object stores a `PredefinedStyle` (i.e. `is_predefined_style()` returns `true`).
    constexpr auto as_predefined_style() const -> PredefinedStyle {
        // Since predefined styles are always stored as non-positive numbers, we should convert them
        // to positive number.
        return static_cast<PredefinedStyle>(-style_);
    }

    constexpr auto is(PredefinedStyle other) const -> bool {
        return *this == Style(other);
    }

    constexpr auto is_auto_style() const -> bool {
        return is(PredefinedStyle::Auto);
    }

    auto operator==(Style const& other) const -> bool = default;

    /// Create a custom style based on a user-specified style number. The user can also achieve the
    /// same effect through the constructor, but the member function can indicate the user's
    /// intention more clearly.
    constexpr static auto custom(std::int8_t style) -> Style {
        return Style(style);
    }

private:
    std::int8_t style_;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_STYLE_HPP
