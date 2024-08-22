#include "annotate_snippets/style_spec.hpp"

#include <rang.hpp>

#include <ostream>
#include <string_view>

namespace ants {
void StyleSpec::render_string(std::ostream& out, std::string_view content) const {
#define COLOR_SWITCH_BODY(ns) COLOR_CASES(ns) BRIGHT_COLOR_CASES(ns) DEFAULT_CASE(ns)

#define COLOR_CASES(ns) COLOR_CASES_IMPL(ns, EMPTY)
#define BRIGHT_COLOR_CASES(ns) COLOR_CASES_IMPL(ns##B, Bright)

#define COLOR_CASES_IMPL(ns, prefix)                                                               \
    COLOR_CASE(Black, black, ns, prefix)                                                           \
    COLOR_CASE(Red, red, ns, prefix)                                                               \
    COLOR_CASE(Green, green, ns, prefix)                                                           \
    COLOR_CASE(Yellow, yellow, ns, prefix)                                                         \
    COLOR_CASE(Blue, blue, ns, prefix)                                                             \
    COLOR_CASE(Magenta, magenta, ns, prefix)                                                       \
    COLOR_CASE(Cyan, cyan, ns, prefix)                                                             \
    COLOR_CASE(Gray, gray, ns, prefix)

#define EMPTY

#define COLOR_CASE(from, to, ns, name_prefix)                                                      \
    case StyleSpec::name_prefix##from:                                                             \
        out << rang::ns::to;                                                                       \
        break;

#define DEFAULT_CASE(ns)                                                                           \
    default:                                                                                       \
        out << rang::ns::reset;                                                                    \
        break;

    // setup foreground and background colors
    switch (foreground_) { COLOR_SWITCH_BODY(fg) }
    switch (background_) { COLOR_SWITCH_BODY(bg) }

    // clang-format off
#define TEXT_STYLE_LIST                                                                            \
    TEXT_STYLE_MAP(Bold, bold) TEXT_STYLE_MAP(Dim, dim) TEXT_STYLE_MAP(Italic, italic)             \
    TEXT_STYLE_MAP(Underline, underline) TEXT_STYLE_MAP(Reversed, reversed)                         \
    TEXT_STYLE_MAP(Conceal, conceal) TEXT_STYLE_MAP(Crossed, crossed)
    // clang-format on

    // setup text styles
#define TEXT_STYLE_MAP(from, to)                                                                   \
    if (has_text_style(StyleSpec::from))                                                           \
        out << rang::style::to;
    TEXT_STYLE_LIST
#undef TEXT_STYLE_MAP

    out << content << rang::fg::reset << rang::bg::reset << rang::style::reset;

#undef TEXT_STYLE_LIST
#undef DEFAULT_CASE
#undef COLOR_CASE
#undef EMPTY
#undef COLOR_CASES_IMPL
#undef BRIGHT_COLOR_CASES
#undef COLOR_CASES
#undef COLOR_SWITCH_BODY
}
}  // namespace ants
