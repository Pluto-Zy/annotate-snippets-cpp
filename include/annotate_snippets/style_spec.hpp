#ifndef ANNOTATE_SNIPPETS_STYLE_SPEC_HPP
#define ANNOTATE_SNIPPETS_STYLE_SPEC_HPP

#include <climits>
#include <cstdint>
#include <ostream>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace ants {
class Style;

/// Represents a `Style` rendering specification, including the text color, background color, and
/// text styles (e.g., bold, italic, underline) when printed to the console.
class StyleSpec {
public:
    /// Predefined colors that can be used for both text foreground and background. These color
    /// names are based on the implementation of `rang`.
    enum PredefinedColor : std::uint8_t {
        Default,

        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        Gray,

        BrightBlack,
        BrightRed,
        BrightGreen,
        BrightYellow,
        BrightBlue,
        BrightMagenta,
        BrightCyan,
        BrightGray,
    };

    /// Predefined text styles. Multiple styles can be combined.
    ///
    /// Note: According to the `rang` documentation, `rang::style::blink` and `rang::style::rblink`
    /// are not supported, hence there are no corresponding enumerators.
    enum TextStyle : std::uint8_t {
        Bold = 0x1,
        Dim = 0x2,
        Italic = 0x4,
        Underline = 0x8,
        Reversed = 0x10,
        Conceal = 0x20,
        Crossed = 0x40,
    };

    StyleSpec() = default;

    /// This constructor allows for implicit conversion of a foreground color to `StyleSpec`.
    constexpr StyleSpec(
        PredefinedColor foreground_color,
        PredefinedColor background_color = PredefinedColor::Default,
        TextStyle text_styles = static_cast<TextStyle>(0)
    ) :
        text_styles_(static_cast<std::uint8_t>(text_styles)),
        foreground_(foreground_color),
        background_(background_color) { }

    /// Outputs the string `content` to the output stream associated with `out`, rendered with the
    /// current style.
    void render_string(std::ostream& out, std::string_view content) const;

    constexpr auto foreground_color() const -> PredefinedColor {
        return foreground_;
    }

    constexpr void set_foreground_color(PredefinedColor color) {
        foreground_ = color;
    }

    constexpr void reset_foreground_color() {
        set_foreground_color(PredefinedColor::Default);
    }

    constexpr auto background_color() const -> PredefinedColor {
        return background_;
    }

    constexpr void set_background_color(PredefinedColor color) {
        background_ = color;
    }

    constexpr void reset_background_color() {
        set_background_color(PredefinedColor::Default);
    }

    constexpr auto text_styles() const -> TextStyle {
        return static_cast<TextStyle>(text_styles_);
    }

    constexpr auto has_text_style(TextStyle style) const -> bool {
        return (text_styles_ & static_cast<std::uint8_t>(style)) != 0;
    }

    constexpr void add_text_style(TextStyle style) {
        text_styles_ |= static_cast<std::uint8_t>(style);
    }

    constexpr void remove_text_style(TextStyle style) {
        text_styles_ &= ~static_cast<std::uint8_t>(style);
    }

    constexpr void clear_text_styles() {
        text_styles_ = 0;
    }

    friend constexpr auto operator+(StyleSpec lhs, StyleSpec::TextStyle rhs) -> StyleSpec {
        lhs.add_text_style(rhs);
        return lhs;
    }

    friend constexpr auto operator+(StyleSpec::TextStyle lhs, StyleSpec rhs) -> StyleSpec {
        rhs.add_text_style(lhs);
        return rhs;
    }

    friend constexpr auto operator+(PredefinedColor foreground_color, TextStyle rhs) -> StyleSpec {
        return static_cast<StyleSpec>(foreground_color) + rhs;
    }

    friend constexpr auto operator+(TextStyle lhs, PredefinedColor foreground_color) -> StyleSpec {
        return lhs + static_cast<StyleSpec>(foreground_color);
    }

    constexpr auto operator+=(TextStyle style) -> StyleSpec& {
        add_text_style(style);
        return *this;
    }

    friend constexpr auto operator-(StyleSpec lhs, StyleSpec::TextStyle rhs) -> StyleSpec {
        lhs.remove_text_style(rhs);
        return lhs;
    }

    constexpr auto operator-=(TextStyle style) -> StyleSpec& {
        remove_text_style(style);
        return *this;
    }

    friend constexpr auto operator==(StyleSpec const& lhs, StyleSpec const& rhs) -> bool {
        return std::tie(lhs.text_styles_, lhs.foreground_, lhs.background_)
            == std::tie(rhs.text_styles_, rhs.foreground_, rhs.background_);
    }

    friend constexpr auto operator!=(StyleSpec const& lhs, StyleSpec const& rhs) -> bool {
        return !(lhs == rhs);
    }

private:
    /// Specified text styles (`TextStyle` and its combinations). We avoid using `std::bitset`
    /// because its alignment requirements are too strict, which would make the size of `StyleSpec`
    /// too large.
    std::uint8_t text_styles_ = 0;
    PredefinedColor foreground_ = PredefinedColor::Default;
    PredefinedColor background_ = PredefinedColor::Default;
};

constexpr auto operator|(  //
    StyleSpec::TextStyle lhs,
    StyleSpec::TextStyle rhs
) -> StyleSpec::TextStyle {
    return static_cast<StyleSpec::TextStyle>(
        static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs)
    );
}

constexpr auto operator&(  //
    StyleSpec::TextStyle lhs,
    StyleSpec::TextStyle rhs
) -> StyleSpec::TextStyle {
    return static_cast<StyleSpec::TextStyle>(
        static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs)
    );
}

constexpr auto operator~(StyleSpec::TextStyle style) -> StyleSpec::TextStyle {
    return static_cast<StyleSpec::TextStyle>(~static_cast<std::uint8_t>(style));
}

template <class StyleSheet, class Level, class = void>
struct is_style_sheet_impl : std::false_type { };  // NOLINT(readability-identifier-naming)

template <class StyleSheet, class Level>
struct is_style_sheet_impl<
    StyleSheet,
    Level,
    std::void_t<std::invoke_result_t<StyleSheet const&, Style const&, Level const&>>> :
    std::is_convertible<
        std::invoke_result_t<StyleSheet const&, Style const&, Level const&>,
        StyleSpec> { };

/// Checks if a callable type `StyleSheet` can be used as a style sheet for `HumanRenderer`.
///
/// The style sheet must be a callable object that accepts `Style` and `Level` as parameters. It
/// determines how text with the specified `Style` should be rendered when the diagnostic level is
/// `Level`. The rendering style is represented by the returned `StyleSpec`. Note that the return
/// type of the style sheet must be convertible to `StyleSpec`, but it does not have to be exactly
/// the same. This allows us to use `StyleSpec::PredefinedColor` as the return type, since it can be
/// implicitly converted to `StyleSpec`.
///
/// TODO: Add an example demonstrating the use of a style sheet.
template <class StyleSheet, class Level>
constexpr bool is_style_sheet = is_style_sheet_impl<StyleSheet, Level>::value;

/// Represents a style sheet that renders every `Style` in plain text format.
struct PlainTextStyleSheet {
    template <class Level>
    constexpr auto operator()(Style const& /*unused*/, Level&& /*unused*/) const -> StyleSpec {
        return {};
    }
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_STYLE_SPEC_HPP
