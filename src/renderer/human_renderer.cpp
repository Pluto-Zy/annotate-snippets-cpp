#include "annotate_snippets/renderer/human_renderer.hpp"

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "annotate_snippets/detail/unicode_display_width.hpp"
#include "annotate_snippets/style.hpp"
#include "annotate_snippets/styled_string.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ants {
namespace {
/// Calculates the number of decimal digits contained in `num` to determine how many spaces are
/// needed to display the integer.
constexpr auto compute_digits_num(unsigned num) -> unsigned {
    if (num == 0) {
        return 1;
    }

    unsigned result = 0;
    for (; num != 0; num /= 10) {
        ++result;
    }
    return result;
}
}  // namespace

auto HumanRenderer::compute_max_line_num_len(AnnotatedSource const& source) const -> unsigned {
    auto const span_trans = [](LabeledSpan const& span) {
        // Since `span.end` is exclusive, if `span.end` falls exactly at the start of a new line,
        // then the actual last annotated line number should be decreased by 1.
        if (span.end.col == 0) {
            return span.end.line - 1;
        } else {
            return span.end.line;
        }
    };

    unsigned const primary_max = source.primary_spans().empty()
        ? 0u
        : std::ranges::max(source.primary_spans() | std::views::transform(span_trans));

    unsigned const result = [&] {
        if (short_message) {
            // If `short_message` is `true`, secondary annotations are not displayed.
            return primary_max;
        } else {
            unsigned const secondary_max = source.secondary_spans().empty()
                ? 0u
                : std::ranges::max(source.secondary_spans() | std::views::transform(span_trans))
                    + source.first_line_number();

            return std::ranges::max(primary_max, secondary_max);
        }
    }();

    return compute_digits_num(result + source.first_line_number());
}

namespace detail {
namespace {
#ifdef __cpp_lib_unreachable
using std::unreachable;
#else
/// `std::unreachable()` implementation from
/// [cppreference](https://en.cppreference.com/w/cpp/utility/unreachable).
[[noreturn]] constexpr void unreachable() {
    // Uses compiler specific extensions if possible. Even if no extension is used, undefined
    // behavior is still raised by an empty function body and the noreturn attribute.
    #if defined(_MSC_VER) && !defined(__clang__)  // MSVC
    __assume(false);
    #else  // GCC, Clang
    __builtin_unreachable();
    #endif
}
#endif
}  // namespace
}  // namespace detail

namespace {
/// Renders a multi-line message `message` with indentation `indentation` onto `render_target`. The
/// first line of `message` will continue directly from the existing content in `render_target`,
/// while other lines will be rendered on new lines with the specified `indentation`. Any parts in
/// `message` with style `Style::Auto` will have their style replaced with `auto_replacement`.
void render_multiline_messages(
    StyledString& render_target,
    StyledStringView const& message,
    unsigned indentation,
    Style auto_replacement
) {
    std::vector<std::vector<StyledStringViewPart>> const lines = message.styled_line_parts();
    if (lines.empty()) {
        return;
    }

    // Render the first line.
    render_target.append(lines.front(), auto_replacement);

    // Render the subsequent lines. Before rendering each line, insert sufficient indentation.
    for (std::vector<StyledStringViewPart> const& parts : lines | std::views::drop(1)) {
        render_target.append_newline();
        render_target.append_spaces(indentation);
        render_target.append(parts, auto_replacement);
    }
}
}  // namespace

void HumanRenderer::render_title_message(
    StyledString& render_target,
    std::string_view level_title,
    std::string_view err_code,
    StyledStringView const& message,
    unsigned max_line_num_len,
    unsigned indentation,
    bool is_secondary,
    bool is_attached
) const {
    if (is_secondary && is_attached && !short_message) {
        // If we are rendering an attached sub-diagnostic item, we need to render the title text
        // differently.
        //
        // Example:
        //
        // 11 |
        //    = note: note something.   <--- The title and title message to be rendered.
        // ^^^^^ This is what we are rendering in this if block.

        // Append a sufficient number of spaces to align the "=" with the line number separator.
        render_target.append_spaces(max_line_num_len + 1);
        // Render "= ".
        render_target.append("= ", ants::Style::LineNumber);
        // We have already rendered a number of spaces equal to `max_line_num_len + 1` and a width
        // of 2 for "= ".
        indentation += max_line_num_len + 3;
    }

    ants::Style const title_style =
        is_secondary ? ants::Style::SecondaryTitle : ants::Style::PrimaryTitle;

    // Render the diagnostic level title (such as "error"). The colon following "error" is rendered
    // later because the error code may be inserted between "error" and ": ".
    render_target.append(level_title, title_style);
    indentation += detail::display_width(level_title);

    // If there is an error code, render it.
    if (!err_code.empty()) {
        std::string rendered_err_code = "[";
        rendered_err_code.append(err_code);
        rendered_err_code.push_back(']');

        render_target.append(rendered_err_code, title_style);
        indentation += detail::display_width(rendered_err_code);
    }

    render_target.append(": ", title_style);
    indentation += 2;

    // Render the title message.
    render_multiline_messages(
        render_target,
        message,
        indentation,
        is_secondary ? ants::Style::SecondaryMessage : ants::Style::PrimaryMessage
    );
}

auto HumanRenderer::render_file_line_col_short_message(
    StyledString& render_target,
    std::vector<AnnotatedSource> const& sources
) -> unsigned {
    unsigned final_width = 0;

    for (unsigned idx = 0; AnnotatedSource const& source :
                           sources | std::views::filter([](AnnotatedSource const& source) {
                               return !source.primary_spans().empty();
                           })) {
        if (idx != 0) {
            render_target.append_newline();
        }

        // Render the file name.
        render_target.append(source.origin(), ants::Style::OriginAndLocation);
        render_target.append(":", ants::Style::OriginAndLocation);

        SourceLocation const loc = source.primary_spans().front().beg;
        std::string const line = std::to_string(loc.line + source.first_line_number());
        std::string const col = std::to_string(loc.col + 1);

        // Render the line number and column number.
        render_target.append(line, ants::Style::OriginAndLocation);
        render_target.append(":", ants::Style::OriginAndLocation);
        render_target.append(col, ants::Style::OriginAndLocation);

        // When rendering a short message, we need to additionally render a ": " at the end.
        render_target.append(": ", ants::Style::OriginAndLocation);

        // Compute the width of the part already rendered. Since we've also drawn one ": " and two
        // ':', we need to add 4.
        final_width = detail::display_width(source.origin()) + line.size() + col.size() + 4;
        ++idx;
    }

    return final_width;
}

namespace {
/// Renders the line number and its separator portion without the line number itself.
void render_line_number(StyledString& render_target, unsigned max_line_num_len) {
    render_target.append_spaces(max_line_num_len + 1);
    render_target.append("|", ants::Style::LineNumber);
}

/// Renders line numbers according to the specified alignment, along with the vertical bar separator
/// between the line number and the source code.
void render_line_number(
    StyledString& render_target,
    unsigned max_line_num_len,
    unsigned line_num,
    HumanRenderer::LineNumAlignment line_num_alignment
) {
    std::string const line_num_str = std::to_string(line_num);

    switch (line_num_alignment) {
    case HumanRenderer::AlignLeft:
        render_target.append(line_num_str, ants::Style::LineNumber);
        // Adds sufficient spaces to align the separator.
        render_target.append_spaces(max_line_num_len + 1 - line_num_str.size());
        break;
    case HumanRenderer::AlignRight:
        // Adds sufficient spaces to ensure the line number text is right-aligned.
        render_target.append_spaces(max_line_num_len - line_num_str.size());
        render_target.append(line_num_str, ants::Style::LineNumber);
        // Adds a single space between the line number and the separator.
        render_target.append_spaces(1);
        break;
    default:
        detail::unreachable();
    }

    render_target.append("|", ants::Style::LineNumber);
}

/// When `short_message` is `false`, this is used to render the file name, line number, and column
/// number triplet in non-short message mode. Examples include "--> main.cpp:1:3" or ":::
/// main.cpp:1:3".
void render_file_line_col(
    StyledString& render_target,
    AnnotatedSource& source,
    unsigned max_line_num_len,
    bool is_first_source
) {
    // If `source` is the first source code being rendered in the current diagnostic entry, start
    // with "-->", otherwise start with ":::".
    if (is_first_source) {
        render_target.append_spaces(max_line_num_len);
        render_target.append("--> ", ants::Style::LineNumber);
    } else {
        // Since the current source is not the first, we first add an empty line.
        render_line_number(render_target, max_line_num_len);
        render_target.append_newline();

        render_target.append_spaces(max_line_num_len);
        render_target.append("::: ", ants::Style::LineNumber);
    }

    // Render the file name.
    render_target.append(source.origin(), ants::Style::OriginAndLocation);

    if (!source.primary_spans().empty()) {
        render_target.append(":", ants::Style::OriginAndLocation);

        SourceLocation const loc = source.primary_spans().front().beg;
        std::string const line = std::to_string(loc.line + source.first_line_number());
        std::string const col = std::to_string(loc.col + 1);

        // Render the line number and column number.
        render_target.append(line, ants::Style::OriginAndLocation);
        render_target.append(":", ants::Style::OriginAndLocation);
        render_target.append(col, ants::Style::OriginAndLocation);
    }
}

/// Replaces the tab characters in the `source` code with the number of spaces specified by
/// `display_tab_width`. If `display_tab_width` is 0, the tab characters are not replaced.
auto normalize_source(std::string_view source, unsigned display_tab_width) -> std::string {
    if (display_tab_width == 0) {
        return static_cast<std::string>(source);
    } else {
        std::string normalized_source;
        normalized_source.reserve(source.size());

        for (char const ch : source) {
            switch (ch) {
            case '\t':
                // Replace '\t' with the specified number of spaces.
                normalized_source.append(display_tab_width, ' ');
                break;
            default:
                normalized_source.push_back(ch);
            }
        }

        return normalized_source;
    }
}

/// `hash_combine` implementation from Boost.
template <class T>
auto hash_combine(std::size_t seed, T const& value) -> std::size_t {
    std::hash<T> hasher;
    seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

/// Represents a multi-line annotation.
///
/// This class is not used for rendering. Since the renderer needs to process single-line and
/// multi-line annotations differently, it converts all single-line annotations to `Annotation` and
/// multi-line annotations to `MultilineAnnotation` for processing (i.e., assigning depths to these
/// annotations). Ultimately, `MultilineAnnotation` will be further converted into `Annotation`.
struct MultilineAnnotation : public LabeledSpan {
    /// The rendering depth of this multi-line annotation.
    ///
    /// We use depth to control the rendering layers of multi-line annotations to prevent multiple
    /// multi-line annotations from overlapping and becoming indistinguishable. For example:
    ///
    ///     x |    foo(x1,
    ///       |  ______^
    ///     x | |      x2,
    ///       | |  ____^
    ///     x | | |    x3,
    ///       | | |____^
    ///     x | |      x4)
    ///       | |_______^
    ///         ^ depth = 0
    ///           ^ depth = 1
    unsigned depth;
    /// Records whether the current multi-line annotation is a primary annotation. This information
    /// will be passed to `Annotation`.
    bool is_primary;

    MultilineAnnotation(LabeledSpan span, bool is_primary) :
        LabeledSpan(std::move(span)), is_primary(is_primary) { }
};

/// Represents annotations attached to a single line.
///
/// This class is used in the rendering process, not the user interface. Specifically, the renderer
/// divides all annotations added by the user by line, making it easy to identify all annotations
/// attached to a particular line. We need this information to calculate the position of each
/// annotation and render them correctly after the corresponding line.
///
/// For single-line annotations, we directly store their start and end column numbers. For
/// multi-line annotations, we split them into three parts and store each separately. Please refer
/// to the documentation comments for the class member `type`.
struct Annotation {
    /// The label associated with this annotation.
    ///
    /// Here we store the return value of `StyledStringView::styled_line_parts()` because we need to
    /// use it in many places (such as when we calculate the display width of the label and when we
    /// render the label), which reduces the number of times we call `styled_line_parts()`.
    ///
    /// On the other hand, it's worth noting that in our current implementation, the type of the
    /// label is `StyledStringView`, which means the label itself does not own the underlying
    /// string. Thus, even if the label is destroyed, we can still use the return value of
    /// `styled_line_parts()`. This would not be the case with `StyledString`.
    std::vector<std::vector<StyledStringViewPart>> label;
    /// The display width of `label` in the console. It is calculated as the maximum display width
    /// of each line. Since we frequently use this value, we store the result here.
    unsigned label_display_width;
    /// The starting (`col_beg`) and ending (`col_end`) column numbers of this annotation. The
    /// ending column number points to the position right after the last byte of the annotated
    /// range.
    ///
    /// Each column number consists of two values. `byte` represents the offset of the *byte* in
    /// *this line*, typically used to extract the corresponding annotated part from the source
    /// code. `display` represents the position of this column when the source code is printed in
    /// the console, typically used in rendering to determine how we should align the annotation
    /// with the annotated code. In some cases, these two values differ, for instance, we consider
    /// the width of a tab character as `HumanRenderer::display_tab_width` (if it is not 0),
    /// although it occupies only 1 byte. Some non-ASCII characters (such as Chinese characters,
    /// emojis, etc.) also cause a discrepancy between the number of bytes and the display width.
    ///
    /// `byte` and `display` are both 0-indexed. For `MultilineHead`, `MultilineTail`, and
    /// `MultilineBody`, both `byte` and `display` in `col_beg` store the depth of the multi-line
    /// annotation. For `MultilineHead` and `MultilineTail`, `col_end` stores the end position of
    /// the annotation when rendered in the current line.
    struct {
        unsigned byte;
        unsigned display;
    } col_beg, col_end;
    /// Represents the rendering level of the current annotation within the line.
    ///
    /// We use rendering levels to control the position and order of annotations to prevent
    /// overlapping annotations and to reduce the number of times underlines cross.
    ///
    /// Specifically, we divide an annotation into two parts: the underline and the label. Each part
    /// can have its own rendering level. For example:
    ///
    ///     func(abc)
    ///          ^^^   <-- Underline at level 0
    ///
    ///     func(abc)
    ///          ^^^ label   <-- Underline and label at level 0
    ///
    ///     func(abc)
    ///          ^^^     <-- Underline at level 0
    ///          |
    ///          label   <-- Label at level 1
    ///
    ///     func(abc)
    ///          ^^^     <-- Underline at level 0
    ///          |
    ///          |
    ///          label   <-- Label at level 2
    ///
    ///     func(abc,
    ///  _______^  ^   <-- Multi-line annotation underline at level 0
    /// |  ________|   <-- Multi-line annotation underline at level 1
    /// | |      hello world)
    /// | |______^ label    ^       <-- Multi-line annotation underline and label at level 0
    /// |___________________|       <-- Multi-line annotation underline at level 1
    ///                     label   <-- Label at level 1
    ///
    /// In the current implementation, we have the following conventions:
    ///
    /// 1. All single-line annotations have their underlines at level 0: they are always rendered
    ///    immediately following the source code line. Therefore, for single-line annotations,
    ///    `render_level` represents the level of its label.
    /// 2. All multi-line annotations have the same level for both underlines and labels, where
    ///    `render_level` represents this level.
    /// 3. If a label's level is 0, it is rendered as an inline label, i.e., it directly follows the
    ///    corresponding underline:
    ///
    ///     func(abc)
    ///          ^^^ label   <-- Inline label
    ///
    ///    If the label's level is not 0, it is rendered as an inter-line label. The position of the
    ///    label then depends on `HumanRenderer::label_position`:
    ///
    ///     func(abc)
    ///          ^^^
    ///          |
    ///          label   <-- Inter-line label, with `HumanRenderer::label_position` set to `Left`.
    unsigned render_level;
    /// The type of the current annotation.
    enum : std::uint8_t {
        /// Annotation for a single line of code.
        SingleLine,

        // All multi-line annotations are divided into three parts, each of which is located on a
        // single line. The following three enumerators correspond to the three parts in the
        // rendering result, for example:
        //
        //     x |   foo(1 + bar(x,
        //       |  _________^              < MultilineHead
        //     x | |             y),        < MultilineBody
        //       | |______________^ label   < MultilineTail
        //     x |       z);

        /// The starting part of a multi-line code annotation, i.e., "______^", where `col.beg`
        /// stores the depth of the multi-line annotation, `col.end` stores the position of the
        /// right byte.
        MultilineHead,
        /// The body of a multi-line code annotation, represented by the "|" symbol between the line
        /// number separator and the actual code line. `col.beg` stores the depth of the annotation.
        MultilineBody,
        /// The ending part of a multi-line code annotation, i.e., "|_____^", where `col.beg` stores
        /// the depth of the multi-line annotation, `col.end` stores the position of the right byte.
        MultilineTail,
    } type;
    /// Indicates whether this annotation is a primary annotation. Primary and secondary annotations
    /// will have different underline symbols (controlled by `HumanRenderer::primary_underline` and
    /// `HumanRenderer::secondary_underline`).
    bool is_primary;

    Annotation() = default;

    /// Constructs an `Annotation` from a single-line annotation specified by `span`.
    static auto from_single_line_span(LabeledSpan const& span, bool is_primary) -> Annotation {
        return {
            /*label=*/span.label,
            /*col_beg=*/span.beg.col,
            /*col_end=*/span.end.col,
            /*type=*/Annotation::SingleLine,
            is_primary,
        };
    }

    /// Constructs the head corresponding single-line annotation from a multi-line annotation
    /// `annotation`.
    static auto from_multiline_head(MultilineAnnotation const& annotation) -> Annotation {
        return {
            /*label=*/ {},
            /*col_beg=*/annotation.depth,
            // `col_end` points to the position right after the last byte annotated, so we need to
            // add 1 here.
            /*col_end=*/annotation.beg.col + 1,
            /*type=*/Annotation::MultilineHead,
            /*is_primary=*/annotation.is_primary,
        };
    }

    /// Constructs the tail corresponding single-line annotation from a multi-line annotation
    /// `annotation`.
    static auto from_multiline_tail(MultilineAnnotation const& annotation) -> Annotation {
        return {
            /*label=*/annotation.label,
            /*col_beg=*/annotation.depth,
            /*col_end=*/annotation.end.col,
            /*type=*/Annotation::MultilineTail,
            /*is_primary=*/annotation.is_primary,
        };
    }

    /// Constructs the body corresponding single-line annotation from a multi-line annotation
    /// `annotation`.
    static auto from_multiline_body(MultilineAnnotation const& annotation) -> Annotation {
        return {
            /*label=*/ {},
            /*col_beg=*/annotation.depth,
            /*col_end=*/0,
            /*type=*/Annotation::MultilineBody,
            /*is_primary=*/annotation.is_primary,
        };
    }

    /// Returns the range of the underline display for the current annotation. The return value is a
    /// tuple of two unsigned integers, representing the start and end positions of the underline.
    auto underline_display_range() const -> std::tuple<unsigned, unsigned> {
        switch (type) {
        case Annotation::SingleLine:
            // For single-line annotations, the range of the underline is the same as its annotation
            // range:
            //
            //     func(arg)
            //          ^^^   <-- The range of the underline is the same as the annotation range
            return std::make_tuple(col_beg.display, col_end.display);
        case Annotation::MultilineHead:
        case Annotation::MultilineTail:
            // For the head and tail of multi-line annotations, we only render a width of 1
            // underline at the start (for the head) and end (for the tail) positions:
            //
            //     func(arg1,
            //  _______^        <-- For the head, only a width of 1 underline is rendered at the
            // |                    start position
            // |        arg2)
            // |____________^   <-- For the tail, only a width of 1 underline is rendered at the end
            //                      position
            //
            // Note that for `MultilineHead` and `MultilineTail`, their `col_beg` member stores the
            // depth of the multi-line annotation, while `col_end` points to the position just after
            // the last byte of the annotated range in this line.
            return std::make_tuple(col_end.display - 1, col_end.display);
        default:
            // For other types, do not render an underline.
            return std::make_tuple(0u, 0u);
        }
    }

    /// Calculate the display range for the label of the annotation.
    ///
    /// If the annotation is rendered inline (i.e., `render_level` is 0), the label follows the
    /// annotation's underline:
    ///
    ///     foo(variable)
    ///         ^^^^^^^^ inline label
    ///
    /// Otherwise, if `label_position` is `HumanRenderer::Left`, the label is rendered at the far
    /// left of the annotated "variable":
    ///
    ///     foo(variable + def)
    ///         ^^^^^^^^   ^^^
    ///         |
    ///         This label is rendered at the far left of the annotated "variable" word.
    ///
    /// Otherwise, it is rendered at the far right:
    ///
    ///     foo(variable + def)
    ///         ^^^^^^^^   ^^^
    ///                |
    ///                This label is rendered at the far right of the annotated "variable" word.
    ///
    /// It's worth noting that there is no need to differentiate between single-line and multiline
    /// annotations, as the underline width for multiline annotations is 1, which ensures that the
    /// rendering effect is the same regardless of the `label_position` value.
    auto label_display_range(  //
        HumanRenderer::LabelPosition label_position
    ) const -> std::tuple<unsigned, unsigned> {
        unsigned const label_beg = [&] {
            if (render_level == 0) {
                return col_end.display + 1;
            } else {
                auto const [underline_beg, underline_end] = underline_display_range();

                switch (label_position) {
                case HumanRenderer::Left:
                    return underline_beg;
                case HumanRenderer::Right:
                    return underline_end - 1;
                default:
                    detail::unreachable();
                }
            }
        }();

        return std::make_tuple(label_beg, label_beg + label_display_width);
    }

private:
    Annotation(
        StyledStringView const& label,
        unsigned col_beg,
        unsigned col_end,
        decltype(type) type,
        bool is_primary
    ) :
        label(label.styled_line_parts()),
        label_display_width(compute_label_display_width()),
        // We cannot calculate the value of the `display` member here, because we need to know the
        // source code associated with this annotation.
        col_beg { .byte = col_beg, .display = col_beg },
        col_end { .byte = col_end, .display = col_end },
        type(type),
        render_level(0),
        is_primary(is_primary) { }

    /// Returns the display width of the label `label`. It is calculated as the maximum width of all
    /// lines in `label`.
    auto compute_label_display_width() const -> unsigned {
        if (label.empty()) {
            return 0;
        } else {
            auto const compute_line_display_width =  //
                [](std::vector<StyledStringViewPart> const& line) {
                    unsigned width = 0;
                    for (auto const& [content, style] : line) {
                        width += detail::display_width(content);
                        // Suppresses warnings for unused variables.
                        (void) style;
                    }

                    return width;
                };

            return std::ranges::max(label | std::views::transform(compute_line_display_width));
        }
    }
};

/// Represents an annotated line of source code.
struct AnnotatedLine {
    /// The source code of the line.
    std::string_view source_line;
    /// All annotations associated with the current source line.
    std::vector<Annotation> annotations;
    /// The display width of the current source line (after normalization).
    unsigned line_display_width;
    /// Indicates that the current line will be omitted (rendered as "..."). If the unannotated
    /// lines between two annotated lines are far apart, they will be omitted. Similarly, if a
    /// multi-line annotation spans too many lines, some lines will also be omitted.
    ///
    /// Note that even if a line is omitted, there might still be some `MultilineBody` annotations
    /// passing through this line.
    bool omitted;

    explicit AnnotatedLine(bool omitted = false) : line_display_width(0), omitted(omitted) { }

    /// Renders this source code line and all its annotations into `render_target`.
    void render(
        StyledString& render_target,
        unsigned max_line_num_len,
        unsigned line_num,
        unsigned depth_num,
        HumanRenderer const& human_renderer
    ) {
        // Calculate how many lines are needed to accommodate all rendered annotations and their
        // labels. We use this array to store the number of lines required for each render level.
        std::vector<unsigned> const level_line_nums = compute_level_line_nums();

        // We create a `StyledString` for each line to facilitate later rendering.
        // `level_line_nums.back()` is the total number of lines required to render these
        // annotations.
        std::vector<StyledString> annotation_lines(level_line_nums.back());

        // Represents the starting rendering position for the source code line, and all annotations'
        // underlines and labels should start from this position. For example:
        //
        // 123 |       func(args)
        //     |  _____^
        //     | |
        //         ^^^^^^^^^^^^^^ Actual range of the code line
        //         |
        //         Starting rendering position of the code line
        //
        // The indentation before the code line consists of the following parts:
        //
        // 1. Space of width `max_line_num_len + 1` for displaying the line number.
        // 2. A line number separator of width 1.
        // 3. Space of width `depth_num + 1` reserved for the body of any multiline annotations. If
        //    `depth_num` is 0, no space is reserved.
        // 4. A space of width 1 between the line number separator and the code.
        //
        // Since we render the line number and its separator separately, here we only consider the
        // indentation for part 3.
        unsigned const source_code_indentation = depth_num == 0 ? 0 : depth_num + 1;

        // We first render the vertical and horizontal lines that connect labels and underlines.
        //
        // We want all vertical connection lines to be rendered above the horizontal connection
        // lines, and when vertical lines overlap, those with a lower rendering level should be on
        // top. We expect to achieve an output like this:
        //
        // 1 |     func(args)
        //   |     ^^^^     ^
        //   |  ______|_____|
        //   | |      |
        //
        // Instead of
        //
        // 1 |     func(args)
        //   |     ^^^^     ^
        //   |  ____________|   <-- Incorrect overlap relationship
        //   | |      |

        // First, render all horizontal connection lines to ensure they are on the bottom.
        render_horizontal_lines(level_line_nums, annotation_lines, source_code_indentation);

        // Next, we render the vertical connection lines in the required order.
        render_vertical_lines(
            level_line_nums,
            annotation_lines,
            source_code_indentation,
            human_renderer.label_position
        );

        // Render all labels.
        render_labels(
            level_line_nums,
            annotation_lines,
            source_code_indentation,
            human_renderer.label_position
        );

        // Render all underlines for the annotations.
        render_underlines(
            level_line_nums,
            annotation_lines,
            source_code_indentation,
            human_renderer.primary_underline,
            human_renderer.secondary_underline
        );

        // At this point, all annotations have been rendered. We will render the results into the
        // render target.

        // Render the source code line.
        StyledString const source_line = render_source_line(
            max_line_num_len,
            line_num,
            depth_num,
            human_renderer.line_num_alignment,
            human_renderer.display_tab_width
        );
        render_target.append(source_line.styled_line_parts().front());

        // Render the annotations.
        for (StyledString const& line : annotation_lines) {
            render_target.append_newline();

            // Render the line number and separator for each line. For annotation lines, these lines
            // are not associated with source code, so the line number part is empty.
            render_line_number(render_target, max_line_num_len);
            // There is always one space between the line number separator and the actual code line.
            render_target.append_spaces(1);

            render_target.append(line.styled_line_parts().front());
        }
    }

private:
    /// Calculates how many lines are needed for each rendering level to accommodate all associated
    /// annotations' underlines, connection lines, and labels.
    auto compute_level_line_nums() const -> std::vector<unsigned> {
        // We divide the lines needed for each annotation into two parts:
        //
        //     func(args)
        //         ^
        // ________|        <-- Number of lines needed for the horizontal connection line of the
        //                      multiline annotation.
        //         label1   --+ Number of lines needed for the annotation's label
        //         label2   --+
        //
        // We calculate separately the number of lines needed for these two parts for each
        // annotation level.

        // Number of lines needed for the labels of annotations at each level.
        std::vector<unsigned> level_label_line_nums;
        // Number of lines needed for the horizontal connection lines of multiline annotations at
        // each level.
        std::vector<std::uint8_t> level_connector_line_nums;

        for (Annotation const& annotation : annotations) {
            // Skip `MultilineBody` annotations, as these do not have associated underlines or
            // labels.
            if (annotation.type == Annotation::MultilineBody) {
                continue;
            }

            unsigned const level_count = std::ranges::max(
                static_cast<unsigned>(level_label_line_nums.size()),
                annotation.render_level + 1
            );

            level_label_line_nums.resize(level_count);
            level_connector_line_nums.resize(level_count);

            // Update the required line count information for the label.
            level_label_line_nums[annotation.render_level] = std::ranges::max(
                level_label_line_nums[annotation.render_level],
                // `label.size()` is the number of lines for the label.
                static_cast<unsigned>(annotation.label.size())
            );

            // If the current annotation is the head or tail of a multiline annotation, one line is
            // needed to draw the horizontal connection line linking its underline and body.
            level_connector_line_nums[annotation.render_level] = static_cast<std::uint8_t>(
                annotation.type == Annotation::MultilineHead
                || annotation.type == Annotation::MultilineTail
            );
        }

        // Total number of lines needed for each level.
        std::vector<unsigned>& level_line_nums = level_label_line_nums;

        if (!level_line_nums.empty()) {
            // Handle the number of lines needed for render level 0. Since annotations at render
            // level 0 are rendered inline, both the label and the horizontal connection line can be
            // on the same line.
            level_line_nums.front() = std::ranges::max({
                level_label_line_nums.front(),
                static_cast<unsigned>(level_connector_line_nums.front()),
                // Since we need to draw underlines on the first line, we need at least 1 line.
                1u,
            });

            // Special handling is needed for render level 1 because each non-inline rendered
            // annotation needs at least 1 line to render the connecting line from the underline to
            // the label. Ensuring that render level 1 has sufficient space means that all
            // subsequent levels will have enough space.
            if (level_line_nums.size() > 1) {
                // In the subsequent loop, the 1 line needed to render the connection line at render
                // level 1 will be added to render level 0, ensuring we have sufficient space.
                level_connector_line_nums[1] = 1;
            }

            // Handle other render levels.
            for (std::size_t i = 1; i != level_line_nums.size(); ++i) {
                // We incorporate the lines needed for rendering horizontal connection lines into
                // the previous render level, making each item in `level_line_nums` point to the
                // start of the *label* for the corresponding render level.
                level_line_nums[i - 1] += level_connector_line_nums[i];
            }
        }

        // Calculate the prefix sum. This allows us to quickly determine the range of lines for a
        // given render level. We reserve a 0 at the beginning of the prefix sum, so
        // [prefix_sum[level], prefix_sum[level + 1]) represents the range of lines corresponding to
        // render level `level`.
        std::vector<unsigned> prefix_sum(level_line_nums.size() + 1);
        std::partial_sum(
            level_line_nums.begin(),
            level_line_nums.end(),
            std::ranges::next(prefix_sum.begin())
        );

        return prefix_sum;
    }

    /// Renders horizontal connection lines for the heads and tails of multiline annotations. For
    /// example:
    ///
    /// 1 |      func(args)
    ///   |          ^
    ///   |  ________|      <-- Render this horizontal connection line.
    void render_horizontal_lines(
        std::span<unsigned const> level_line_nums,
        std::span<StyledString> annotation_lines,
        unsigned source_code_indentation
    ) const {
        for (Annotation const& annotation : annotations) {
            // For the head and tail of multiline annotations, we need to draw horizontal connecting
            // lines that link their body and the end of the connecting line.
            if (annotation.type == Annotation::MultilineHead
                || annotation.type == Annotation::MultilineTail) {
                // Calculate the start and end positions of the horizontal connecting line.
                //
                //     func(args)
                //  ____________-
                // ^            ^^
                // |            ||
                // col_beg      |col_end
                //              underline_display_range()[0]
                unsigned const connector_beg = annotation.col_beg.display + 1;
                unsigned const connector_end =
                    std::get<0>(annotation.underline_display_range()) + source_code_indentation;

                // The index of the line where the horizontal connecting line is to be inserted. If
                // the render level is 0 (i.e., inline format), it is inserted in the line of the
                // label, otherwise in the line above the label.
                unsigned const line_idx =
                    annotation.render_level == 0 ? 0 : level_line_nums[annotation.render_level] - 1;

                annotation_lines[line_idx].set_styled_content(
                    connector_beg,
                    std::string(connector_end - connector_beg, '_'),
                    annotation.is_primary ? Style::PrimaryUnderline : Style::SecondaryUnderline
                );
            }
        }
    }

    /// Renders vertical connection lines for annotations. For all annotations with a render level
    /// not equal to 0, we need to render vertical lines connecting their underlines to their
    /// labels:
    ///
    /// 1 |     func(args)
    ///   |          ^^^^
    ///   |          |      <-- Render this vertical line
    ///   |          label
    ///
    /// For all multiline annotations, we need to render the parts of their body that belong to this
    /// line:
    ///
    /// 1 |      func(args)
    ///   |  ________^
    ///   | |               <-- Render this vertical line
    void render_vertical_lines(
        std::span<unsigned const> level_line_nums,
        std::span<StyledString> annotation_lines,
        unsigned source_code_indentation,
        HumanRenderer::LabelPosition label_position
    ) {
        // We render in order from the highest to the lowest rendering levels to ensure that
        // vertical lines for lower render levels (i.e., shorter lines) appear on top.
        std::ranges::sort(annotations, [&](Annotation const& lhs, Annotation const& rhs) {
            return rhs.render_level < lhs.render_level;
        });

        // Draw all vertical connecting lines in the sorted order.
        for (Annotation const& annotation : annotations) {
            // Style of the connecting lines.
            Style const connector_style =
                annotation.is_primary ? Style::PrimaryUnderline : Style::SecondaryUnderline;

            // When the render level is not 0, render the connecting lines from the annotation
            // underline to the label.
            if (annotation.render_level != 0) {
                // Position of the connecting line.
                unsigned const connector_position =
                    std::get<0>(annotation.label_display_range(label_position))
                    + source_code_indentation;

                // clang-format off
                for (StyledString& line : annotation_lines
                        | std::views::take(level_line_nums[annotation.render_level])
                        | std::views::drop(1))
                // clang-format on
                {
                    line.set_styled_content(connector_position, "|", connector_style);
                }
            }

            // For all multiline annotations, we need to draw their body, i.e., the vertical line
            // connecting the head and tail.
            auto const body_lines = [&] {
                switch (annotation.type) {
                case Annotation::MultilineHead:
                    // For the head, it should start from the first line of the label and connect to
                    // the last line of `annotation_lines`. For inline rendered annotations, it
                    // starts from the line below the label.
                    //
                    // 123 |      func(args)
                    //     |  ________^
                    //     | |                  <-- Starting position
                    //
                    // Note that before P1739R4, providing `std::views::drop` with `std::span` would
                    // result in `std::ranges::drop_view` rather than `std::span`, causing
                    // inconsistent return types in some compilers (like g++-11) within this lambda
                    // expression's branches. Therefore, we supply `drop_view` as an argument to
                    // `std::span` explicitly.
                    return std::span(
                        annotation_lines
                        | std::views::drop(
                            annotation.render_level == 0 ? 1
                                                         : level_line_nums[annotation.render_level]
                        )
                    );
                case Annotation::MultilineTail:
                    // For the tail, it should start from the first line of `annotation_lines` and
                    // connect to the line where the horizontal connecting line is located.
                    //
                    // 123 | |    func(args)
                    //     | |________^         <-- Ending position
                    //
                    // Note that before P1739R4, providing `std::views::take` with `std::span` would
                    // result in `std::ranges::take_view` rather than `std::span`, causing
                    // inconsistent return types in some compilers (like g++-11) within this lambda
                    // expression's branches. Therefore, we supply `take_view` as an argument to
                    // `std::span` explicitly.
                    return std::span(
                        annotation_lines
                        | std::views::take(
                            annotation.render_level == 0 ? 1
                                                         : level_line_nums[annotation.render_level]
                        )
                    );
                case Annotation::MultilineBody:
                    // For the body of multiline annotations, it should traverse all lines.
                    return annotation_lines;
                default:
                    return std::span<StyledString>();
                }
            }();

            for (StyledString& line : body_lines) {
                line.set_styled_content(annotation.col_beg.display, "|", connector_style);
            }
        }
    }

    /// Renders all the labels of the annotations.
    ///
    /// 1 |     func(args)
    ///   |          ^^^^ label     <-- Render the label
    void render_labels(
        std::span<unsigned const> level_line_nums,
        std::span<StyledString> annotation_lines,
        unsigned source_code_indentation,
        HumanRenderer::LabelPosition label_position
    ) const {
        for (Annotation const& annotation : annotations) {
            if (!annotation.label.empty()) {
                // The starting column for rendering the label.
                unsigned const label_col_beg =
                    std::get<0>(annotation.label_display_range(label_position))
                    + source_code_indentation;
                // The starting line for rendering the label.
                unsigned const label_line_beg = level_line_nums[annotation.render_level];

                // Render the label line by line.
                for (unsigned const line_idx :
                     std::views::iota(std::size_t(0), annotation.label.size())) {
                    // The target for the `line_idx` line of the label.
                    StyledString& target_line = annotation_lines[label_line_beg + line_idx];
                    // The content of the `line_idx` line of the label.
                    //
                    // TODO: When the compiler supports it, rewrite this loop using
                    // `std::views::enumerate`.
                    std::vector<StyledStringViewPart> const& label_line =
                        annotation.label[line_idx];

                    target_line.set_styled_content(
                        label_col_beg,
                        label_line,
                        annotation.is_primary ? Style::PrimaryLabel : Style::SecondaryLabel
                    );
                }
            }
        }
    }

    /// Render all underlines for the annotations.
    ///
    /// We must render the underlines in a specific order to ensure that when underlines overlap,
    /// they maintain a certain order. Specifically, we need to meet the following 2 requirements:
    ///
    /// 1. Underlines of primary annotations should appear above those of secondary annotations. For
    /// example, the rendering of:
    ///
    ///     func(args)
    ///     ------
    ///         ^^^^^^
    ///
    /// should result in:
    ///
    ///     func(args)
    ///     ----^^^^^^
    ///
    /// rather than:
    ///     func(args)
    ///     ------^^^^
    ///
    /// 2. We should ensure as many underlines as possible are displayed. For example, the rendering
    /// of:
    ///
    ///     func(args)
    ///         ^^^^^^
    ///          ----
    ///
    /// should result in:
    ///
    ///     func(args)
    ///         ^----^
    ///
    /// rather than:
    ///
    ///     func(args)
    ///         ^^^^^^
    void render_underlines(
        std::span<unsigned const> level_line_nums,
        std::span<StyledString> annotation_lines,
        unsigned source_code_indentation,
        char primary_underline,
        char secondary_underline
    ) {
        auto const primary_annotations =
            std::ranges::partition(annotations, [](Annotation const& annotation) {
                return !annotation.is_primary;
            });
        auto const secondary_annotations = std::ranges::subrange(
            std::ranges::begin(annotations),
            std::ranges::begin(primary_annotations)
        );

        // We first render all secondary annotation underlines, then the primary annotation
        // underlines, ensuring we meet the first requirement.
        for (Annotation const& annotation : annotations) {
            auto const [underline_beg, underline_end] = annotation.underline_display_range();

            // This check is necessary. If the line contains no annotations with underlines but does
            // include some annotations with empty underlines (such as `MultilineBody`), then
            // `annotation_lines` may be empty, and we should not proceed further.
            if (underline_beg == underline_end) {
                continue;
            }

            std::string const underline(
                underline_end - underline_beg,
                annotation.is_primary ? primary_underline : secondary_underline
            );
            Style const underline_style =
                annotation.is_primary ? Style::PrimaryUnderline : Style::SecondaryUnderline;

            annotation_lines.front().set_styled_content(
                underline_beg + source_code_indentation,
                underline,
                underline_style
            );
        }

        // Next, we identify all the underlines of secondary annotations that are completely covered
        // by the underlines of primary annotations and render them to the forefront. This ensures
        // we meet the second requirement.
        for (Annotation const& secondary_annotation : secondary_annotations) {
            auto const [secondary_beg, secondary_end] =
                secondary_annotation.underline_display_range();

            for (Annotation const& primary_annotation : primary_annotations) {
                auto const [primary_beg, primary_end] =
                    primary_annotation.underline_display_range();

                // If the primary annotation `primary_annotation` completely covers the underline of
                // `secondary_annotation`, then render `secondary_annotation` at the front. Note
                // that if these two annotations have exactly the same underline range, we do not
                // prioritize `secondary_annotation` because this does not increase the number of
                // visible underlines.
                if (primary_beg <= secondary_beg && secondary_end <= primary_end
                    && primary_end - primary_beg != secondary_end - secondary_beg) {
                    annotation_lines.front().set_styled_content(
                        secondary_beg + source_code_indentation,
                        std::string(secondary_end - secondary_beg, secondary_underline),
                        Style::SecondaryUnderline
                    );
                }
            }
        }
    }

    /// Renders a source code line. For example:
    ///
    /// 10 | |     func(args)    <-- This function renders this line.
    ///    | |_________^
    ///
    /// Three parts need to be rendered:
    ///
    /// 10 | |     func(args)
    /// ^^^^ ^ ^^^^^^^^^^^^^^ Normalized source code line
    /// |    |
    /// |    Body of a multiline annotation (vertical line connecting the start and end of the
    /// |    multiline annotation)
    /// Line number and its separator
    auto render_source_line(
        unsigned max_line_num_len,
        unsigned line_num,
        unsigned depth_num,
        HumanRenderer::LineNumAlignment line_num_alignment,
        unsigned display_tab_width
    ) const -> StyledString {
        // Determine where to draw the vertical line "|" indicating the body of a multiline
        // annotation before the source code line. The following 2 scenarios require us to draw "|"
        // before the *source code line*:
        //
        // 1. The current line has a `MultilineBody` annotation, indicating that a multiline
        //    annotation passes through this line.
        // 2. The current line has a `MultilineTail` annotation, indicating that the current line is
        //    the end of a multiline annotation.
        //
        // For example:
        //
        //     func(arg1,
        //  _______^
        // |        arg2,    <-- Current line with a `MultilineBody` annotation.
        // |        arg3)    <-- Current line with a `MultilineTail` annotation.
        // |____________^

        // Rendering result of the vertical lines for the multiline annotation body that should be
        // inserted before the source code line.
        StyledString vertical_line_content;
        // All lines have the same indentation, which is `depth_num`.
        vertical_line_content.append_spaces(depth_num);

        for (Annotation const& annotation : annotations) {
            if (annotation.type == Annotation::MultilineBody
                || annotation.type == Annotation::MultilineTail) {
                // For `MultilineBody` and `MultilineTail`, their `col_beg` member stores the
                // position where the underline should be drawn (i.e., the depth of the annotation).
                unsigned const depth = annotation.col_beg.display;

                vertical_line_content[depth] = '|';
                vertical_line_content.set_style(
                    annotation.is_primary ? ants::Style::PrimaryUnderline
                                          : ants::Style::SecondaryUnderline,
                    depth,
                    depth + 1
                );
            }
        }

        StyledString render_target;

        if (omitted) {
            // If the code line is omitted, render it as "...".
            render_target.append("...", ants::Style::LineNumber);

            if (!vertical_line_content.empty()) {
                // Next, we need to add spaces to ensure that `vertical_line_content` is inserted in
                // the correct position.
                //
                // 123 | |   code line
                // 124 | |   code line
                // ...   |
                //    ^^^ We need to add these spaces
                //
                // There are characters with a width of `max_line_num_len + 1` before the line
                // number separator, plus the separator itself, and we need to insert another space
                // between the line number separator and the body of the multiline annotation, so we
                // need a total indentation of `max_line_num_len + 3`. As we have already rendered
                // the 3-width "..." string, we still need to insert `max_line_num_len` spaces.
                render_target.append_spaces(max_line_num_len);
                // Insert `vertical_line_content`.
                //
                // Since `vertical_line_content` is not empty, the return value of
                // `styled_line_parts()` is also not empty.
                render_target.append(vertical_line_content.styled_line_parts().front());
            }
        } else {
            // To fully render the code line, we need to render the line number.
            render_line_number(render_target, max_line_num_len, line_num, line_num_alignment);

            if (!vertical_line_content.empty()) {
                // Insert a space between the line number separator and the body of the multiline
                // annotation.
                render_target.append_spaces(1);
                // Insert `vertical_line_content`.
                //
                // Since `vertical_line_content` is not empty, the return value of
                // `styled_line_parts()` is also not empty.
                render_target.append(vertical_line_content.styled_line_parts().front());
            }

            // Insert the source code line. Note that we always insert a space before the source
            // code line.
            render_target.append_spaces(1);
            render_target.append(  //
                normalize_source(source_line, display_tab_width),
                ants::Style::SourceCode
            );
        }

        return render_target;
    }
};

class AnnotatedLines {
public:
    /// Constructs an `AnnotatedLines` object from the source code `source`.
    ///
    /// To construct the `AnnotatedLines` object, we need to divide all annotations by line.
    /// Single-line annotations are directly assigned to the corresponding line. Multi-line
    /// annotations are split into head, body, and tail parts, and assigned to the respective lines.
    ///
    /// Additionally, this function organizes annotations, which includes:
    /// 1. Appropriately adjusting annotation ranges, such as modifying empty spans to annotate a
    ///    single character (implemented by `adjust_span()`).
    /// 2. Assigns a depth to each multi-line annotation to reduce the potential for overlap during
    ///    rendering.
    /// 3. Determines the rendering approach for unannotated lines, whether to render fully or to
    ///    omit. Implemented by `handle_unannotated_lines()`.
    /// 4. Decides whether multi-line annotations should be folded, meaning omitting some lines.
    ///    Implemented by `fold_multiline_annotations()`.
    /// 5. Calculates the display width of annotations and source code lines. Implemented by
    ///    `compute_display_columns()`.
    /// 6. Assigns render levels to annotations line by line. Implemented by
    ///    `assign_annotation_levels()`.
    static auto from_source(  //
        AnnotatedSource& source,
        HumanRenderer const& renderer
    ) -> AnnotatedLines {
        AnnotatedLines result;

        for (LabeledSpan& span : source.primary_spans()) {
            adjust_span(source, span);
            result.add_span(std::move(span), /*is_primary=*/true);
        }

        for (LabeledSpan& span : source.secondary_spans()) {
            adjust_span(source, span);
            result.add_span(std::move(span), /*is_primary=*/false);
        }

        result.handle_multiline_spans();

        result.handle_unannotated_lines(renderer.max_unannotated_line_num);
        result.fold_multiline_annotations(renderer.max_multiline_annotation_line_num);

        result.compute_display_columns(source, renderer.display_tab_width);

        for (auto& [line_no, line] : result.lines_) {
            result.assign_annotation_levels(renderer.label_position, line);
            (void) line_no;
        }

        return result;
    };

    auto annotated_lines() -> std::map<unsigned, AnnotatedLine>& {
        return lines_;
    }

    auto annotated_lines() const -> std::map<unsigned, AnnotatedLine> const& {
        return lines_;
    }

    auto depth_num() const -> unsigned {
        return depth_num_;
    }

private:
    /// Stores the line numbers and their associated annotations. We use an ordered associative
    /// container to ensure sequential traversal of all annotated lines.
    std::map<unsigned, AnnotatedLine> lines_;
    /// Stores all multi-line annotations, as we need to handle them with different logic. For
    /// example, we need to assign depths to all multi-line annotations.
    ///
    /// This member is only used for storing intermediate results. Once `AnnotatedLines` is fully
    /// constructed, this member serves no further purpose.
    std::vector<MultilineAnnotation> multiline_annotations_;
    /// The number of different depths in all multi-line annotations associated with the current
    /// source code. As the algorithm allocates depths starting from 0 and assigns them
    /// sequentially, this value actually represents the highest allocated depth value plus one.
    ///
    /// For example, if the algorithm allocates depths of 0, 1, 2, 0 for 4 multi-line annotations,
    /// then `depth_num_` would be 3.
    unsigned depth_num_;

    static void adjust_span(AnnotatedSource& source, LabeledSpan& span) {
        // We handle empty annotation ranges specially. In some cases, a user may want to annotate a
        // single character but provides an empty range (i.e., `span.beg` and `span.end` are equal),
        // for example, when attempting to annotate EOF, the front end may not provide a position
        // like `EOF + 1`. Therefore, we modify empty ranges here to annotate a single character.
        if (span.beg == span.end) {
            ++span.end.col;
        }

        // Sometimes we will extend the annotation to the end of a line. In the user interface,
        // since we allow users to specify the range of bytes annotated (rather than line and column
        // numbers), `span.end` will be set to the position right after the last character of this
        // line. This causes `span.end` to actually point to the first character of the next line,
        // rather than a non-existent character right after the newline character of the current
        // line. Similarly, since we always consider EOF (or any position beyond the valid byte
        // range of the source code) to belong to a hypothetical line after the last line, the same
        // situation can occur: the user intends to annotate EOF, but `span.end` points to some
        // position in a hypothetical line.
        //
        // Therefore, when `span.end` points to the start of a line, we adjust it to point to a
        // non-existent character right after the last character of the previous line. This does not
        // affect the rendering result but allows us to correctly determine the properties of the
        // annotation, such as preventing us from incorrectly judging a single-line annotation as a
        // multi-line annotation.
        if (span.end.col == 0) {
            // To get the end position of the previous line, we calculate the offsets of the first
            // characters of the previous line and the current line respectively. This may involve
            // caching, but it does not introduce unnecessary calculations, as our results will also
            // be used again when rendering actual code lines.
            std::size_t const prev_line_start = source.line_offset(span.end.line - 1);
            std::size_t const cur_line_start = source.line_offset(span.end.line);

            span.end.col = static_cast<unsigned>(cur_line_start - prev_line_start);
            --span.end.line;
        }
    }

    /// Constructs `Annotation` or `MultilineAnnotation` based on `LabeledSpan`. Single-line
    /// annotations are added to `lines_`, while multi-line annotations are added to
    /// `multiline_annotations_`.
    ///
    /// Since this function consumes `span`, the parameter passed to it should not be used after the
    /// function ends.
    void add_span(LabeledSpan&& span, bool is_primary) {
        if (span.beg.line == span.end.line) {
            lines_[span.beg.line].annotations.push_back(
                Annotation::from_single_line_span(span, is_primary)
            );
        } else {
            multiline_annotations_.emplace_back(std::move(span), is_primary);
        }
    }

    /// Handles all multi-line annotations separately. We need to assign depths to all multi-line
    /// annotations and count how many different depths have been allocated. Once processing is
    /// complete, we convert all multi-line annotations into `Annotation`.
    void handle_multiline_spans() {
        assign_multiline_depth();

        // Compute the maximum depth. Note, if `multiline_annotations_` is not empty, what we're
        // actually calculating is the maximum depth plus 1, as explained in the documentation
        // comments for `depth_num_`.
        depth_num_ = multiline_annotations_.empty()
            ? 0
            : std::ranges::max(
                  multiline_annotations_
                  | std::views::transform([](MultilineAnnotation const& annotation) {
                        return annotation.depth;
                    })
              ) + 1;

        // Convert `MultilineAnnotation` into `Annotation`.
        for (MultilineAnnotation const& annotation : multiline_annotations_) {
            // We need to split the `MultilineAnnotation` into 3 parts.
            lines_[annotation.beg.line].annotations.push_back(
                Annotation::from_multiline_head(annotation)
            );
            lines_[annotation.end.line].annotations.push_back(
                Annotation::from_multiline_tail(annotation)
            );

            // All intermediate lines of the multi-line annotation. Note that even though the '|'
            // character also needs to be rendered in the end line, we do not consider the end line
            // as `MultilineBody`.
            //
            // For multi-line annotations we have `annotation.beg.line != annotation.end.line`, so
            // here we can safely add 1 here.
            for (unsigned const line :
                 std::views::iota(annotation.beg.line + 1, annotation.end.line)) {
                lines_[line].annotations.push_back(Annotation::from_multiline_body(annotation));
            }
        }
    }

    /// Assigns a depth to each multi-line annotation in `multiline_annotations_` to reduce the
    /// potential for overlap during rendering.
    ///
    /// In implementation, we convert all multi-line annotations into an [interval
    /// graph](https://en.wikipedia.org/wiki/Interval_graph). Specifically, we treat the interval
    /// formed by the start and end lines of each multi-line annotation as vertices of the graph. If
    /// two intervals overlap, an edge is added between them. We then color the vertices and assign
    /// depths to the multi-line annotations based on the coloring results. Since vertices of the
    /// same edge will not have the same color, we ensure that overlapping multi-line annotations do
    /// not share the same depth.
    void assign_multiline_depth() {
        // Sort `multiline_annotations_` to produce as visually appealing and intersection-free a
        // rendering result as possible. We dictate that the smaller the depth, the closer the
        // annotation is to the line number separator. Generally, we desire:
        //
        // 1. If two multi-line annotations have different starting line numbers, the one with the
        // smaller starting line number should have a smaller depth, for example:
        //
        //     x |    foo(x1,
        //       |  ______^        <-- Start of multi-line annotation A
        //     x | |      x2,
        //       | |  ____^        <-- Start of multi-line annotation B
        //     x | | |    x3,
        //       | | |____^        <-- End of multi-line annotation B
        //     x | |      x4)
        //       | |_______^       <-- End of multi-line annotation A
        // We assign a smaller depth to multi-line annotation A so it can fully contain B without
        // intersections. Conversely, if the ending line of B is less than the ending line of A,
        // intersections are inevitable regardless of their depths.
        //
        // 2. If two multi-line annotations start on the same line, the one with the greater ending
        // line should have a smaller depth, for example:
        //
        //     x |    foo(x1,
        //       |  ______^        <-- Start of multi-line annotation A
        //       | |  ____|        <-- Start of multi-line annotation B
        //     x | | |    x2,
        //     x | | |    x3,
        //       | | |____^        <-- End of multi-line annotation B
        //     x | |      x4)
        //       | |_______^       <-- End of multi-line annotation A
        //
        // 3. If two multi-line annotations share the same starting and ending line numbers, the one
        // with the smaller starting column number should have a smaller depth, for example:
        //
        //     x |    foo(x1,
        //       |  __^   ^        <-- Start of multi-line annotations A and B
        //       | |  ____|
        //     x | | |    x2,
        //     x | | |    x3,
        //     x | | |    x4)
        //       | | |____^ ^
        //       | |________|      <-- End of multi-line annotations A and B
        // Note that the rendering order of the ending lines depends on the column number of the
        // ending positions rather than the depth, so if both annotations have labels, intersections
        // at the ending lines are independent of their depths.
        //
        // *Note:* The scenarios described are our desired results, but we cannot guarantee that the
        // greedy algorithm produces these exact outcomes, as it also attempts to minimize the
        // number of depths. By controlling the order of annotations in `multiline_annotations`, we
        // can influence the distribution of depths to some extent but not decisively.
        //
        // Additionally, all members of `LabeledSpan`'s `beg` and `end` participate in sorting,
        // ensuring annotations with the same range are contiguous: we need to assign the same depth
        // to the same multi-line annotations because we want to avoid outcomes like:
        //
        //     x |    foo(x1,
        //       |  ______^
        //       | |  ____|
        //     x | | |    x2,
        //     x | | |    x3,
        //     x | | |    x4)
        //       | | |______^ label a
        //       | |________|
        //       |          label b
        // Instead, we aim for outputs like:
        //
        //     x |    foo(x1,
        //       |  ______^
        //     x | |      x2,
        //     x | |      x3,
        //     x | |      x4)
        //       | |________^ label a
        //       |            label b
        std::ranges::sort(
            multiline_annotations_,
            [](MultilineAnnotation const& lhs, MultilineAnnotation const& rhs) {
                return std::tie(lhs.beg.line, rhs.end.line, lhs.beg.col, lhs.end.col)
                    < std::tie(rhs.beg.line, lhs.end.line, rhs.beg.col, rhs.end.col);
            }
        );

        /// Represents the vertices of the interval graph.
        struct Vertex {
            /// All multi-line annotations bound to the current vertex. They will have the same
            /// depth.
            std::span<MultilineAnnotation> annotation_range;
            std::vector<Vertex*> neighbors;
            /// The depth value associated with the current vertex plus 1. If `depth` is 0, it
            /// indicates no depth has been assigned yet.
            unsigned depth = 0;

            /// Determines whether the intervals represented by two `Vertex` overlap.
            auto overlap(Vertex const& other) const -> bool {
                // We assume that the `annotation_range` in both vertices is non-empty, and that all
                // elements in an `annotation_range` of a `Vertex` have the same line range.
                unsigned const self_line_beg = annotation_range.front().beg.line;
                unsigned const self_line_end = annotation_range.front().end.line;
                unsigned const other_line_beg = other.annotation_range.front().beg.line;
                unsigned const other_line_end = other.annotation_range.front().end.line;

                return (self_line_beg <= other_line_beg && other_line_beg < self_line_end)
                    || (other_line_beg <= self_line_beg && self_line_beg < other_line_end);
            }
        };

        std::vector<Vertex> interval_graph;
        interval_graph.reserve(multiline_annotations_.size());

        // We combine annotations with the same range into one `Vertex`, so they will have the same
        // depth.
        for (auto iter = multiline_annotations_.begin(); iter != multiline_annotations_.end();) {
            // `end_iter` points to the first annotation that does not have the same range as the
            // element pointed to by `iter`.
            auto const end_iter = std::ranges::find_if_not(
                iter,
                multiline_annotations_.end(),
                [&](MultilineAnnotation const& span) {
                    return std::tie(iter->beg, iter->end) == std::tie(span.beg, span.end);
                }
            );

            // Bind the range formed by `iter` and `end_iter` to a vertex.
            // clang-format off
            interval_graph.push_back(Vertex { .annotation_range { iter, end_iter } });
            // clang-format on

            iter = end_iter;
        }

        // Build neighbor relationships between `Vertex` instances: we need to add an edge for each
        // pair of overlapping intervals.
        for (auto cur_iter = interval_graph.begin(); cur_iter != interval_graph.end(); ++cur_iter) {
            for (auto neighbor_iter = std::ranges::next(cur_iter);
                 neighbor_iter != interval_graph.end();
                 ++neighbor_iter) {
                if (cur_iter->overlap(*neighbor_iter)) {
                    cur_iter->neighbors.push_back(std::to_address(neighbor_iter));
                    neighbor_iter->neighbors.push_back(std::to_address(cur_iter));
                } else {
                    // Since we have sorted `multiline_annotations`, overlapping intervals are
                    // contiguous.
                    break;
                }
            }
        }

        // Assign depths to each `Vertex`.
        for (Vertex& vertex : interval_graph) {
            // Number of states: in the worst case, we need to assign an unique depth to each
            // vertex. Additionally, we reserve 0 to indicate the "unassigned" state.
            std::vector<std::uint8_t> depth_bucket(interval_graph.size() + 1);
            for (Vertex* const neighbor : vertex.neighbors) {
                depth_bucket[neighbor->depth] = 1;
            }

            // Find the first unused depth and assign it to `vertex`. Note that we need to ignore
            // the first element of `depth_bucket` because we need to ignore all vertices that have
            // not been assigned a depth.
            auto const first_unused = std::ranges::find(depth_bucket | std::views::drop(1), 0);
            vertex.depth = std::ranges::distance(depth_bucket.begin(), first_unused);
        }

        // Convert `Vertex`'s `depth` to the depths assigned to each multi-line annotation.
        for (Vertex const& vertex : interval_graph) {
            for (MultilineAnnotation& annotation : vertex.annotation_range) {
                // Since `vertex.depth` starts from 1, we need to subtract 1 additionally.
                annotation.depth = vertex.depth - 1;
            }
        }
    }

    /// Handles unannotated lines. If the number of unannotated lines between two annotated lines
    /// exceeds `max_unannotated_line_num`, they are replaced with an omitted line. Otherwise, these
    /// unannotated lines are displayed in full.
    void handle_unannotated_lines(unsigned max_unannotated_line_num) {
        if (lines_.empty()) {
            return;
        }

        // In the loop below, we need to insert elements into `lines_` while iterating through it.
        // However, since inserting elements does not invalidate any iterators of `std::map`, we can
        // safely use the original iterators after inserting elements. Moreover, since we always
        // insert elements forward, this ensures that we do not access elements we just inserted.
        for (auto prev_iter = lines_.begin(), cur_iter = std::ranges::next(prev_iter);
             cur_iter != lines_.end();
             prev_iter = cur_iter++) {
            unsigned const prev_line_no = prev_iter->first;
            unsigned const cur_line_no = cur_iter->first;
            if (cur_line_no - prev_line_no - 1 > max_unannotated_line_num) {
                // Insert a line that is marked as omitted.
                if (cur_line_no - prev_line_no != 1) {
                    // Since the omitted line does not display a line number, the line number here
                    // has no significance.
                    lines_.emplace(prev_line_no + 1, AnnotatedLine(/*omitted=*/true));
                }
            } else {
                // Fully display unannotated lines.
                // clang-format off
                auto rng = std::views::iota(prev_line_no + 1, cur_line_no)
                    | std::views::transform([](unsigned line) {
                          return std::make_pair(line, AnnotatedLine(/*omitted=*/false));
                      })
                    | std::views::common;
                // clang-format on
                lines_.insert(std::ranges::begin(rng), std::ranges::end(rng));
            }
        }
    }

    /// Handles the body parts of multi-line annotations. If there are consecutive lines containing
    /// only `Annotation::MultilineBody` annotations, and the number of these lines exceeds
    /// `max_multiline_annotation_line_num`, then the excess lines are folded (several middle lines
    /// are merged into one omitted line to meet the remaining line count requirement).
    void fold_multiline_annotations(unsigned max_multiline_annotation_line_num) {
        // All multi-line annotations will be fully rendered if `max_multiline_annotation_line_num`
        // is less than 2.
        if (max_multiline_annotation_line_num < 2) {
            return;
        }

        // Since we are only counting consecutive `MultilineBody` lines, we need to exclude
        // `MultilineHead` and `MultilineTail` from these counts.
        max_multiline_annotation_line_num -= 2;

        // This function attempts to fold several lines within the foldable area formed by
        // [line_beg, line_end) so that the number of remaining lines does not exceed
        // `max_multiline_annotation_line_num`.
        auto const fold_lines = [&](unsigned line_beg, unsigned line_end) {
            // Number of lines in the foldable area.
            unsigned const foldable_lines_num = line_end - line_beg;

            if (foldable_lines_num > max_multiline_annotation_line_num) {
                // All lines within the same foldable area have exactly the same annotations because
                // these lines can only carry several `MultilineBody` annotations, which only change
                // with the appearance of `MultilineHead` and `MultilineTail`.

                // Number of lines to be folded.
                unsigned const folded_lines_num =
                    foldable_lines_num - max_multiline_annotation_line_num;

                // Start from the middle of the foldable area and spread outwards until the
                // remaining number of lines does not exceed `max_multiline_annotation_line_num`.
                //
                // We make the first line to be folded omitted, and remove the others.
                auto const folded_range = std::views::iota(
                    line_beg + max_multiline_annotation_line_num / 2,
                    line_beg + max_multiline_annotation_line_num / 2 + folded_lines_num
                );

                lines_[folded_range.front()].omitted = true;
                for (unsigned const line : folded_range | std::views::drop(1)) {
                    // Note that we are modifying `lines_` here, and this function will be called in
                    // the loop below, where we iterate over `lines_`. However, this is safe because
                    // deleting elements from a `std::map` does not invalidate other iterators, and
                    // we are not deleting the element currently being accessed in the loop.
                    lines_.erase(line);
                }
            }
        };

        // Start of the current foldable area.
        unsigned foldable_area_beg = 0;
        // End of the current foldable area.
        unsigned foldable_area_end = static_cast<unsigned>(-1);
        for (auto const& [line_no, annotated_line] : lines_) {
            // Determine if a line can be folded. We require that this line must have annotations
            // (lines without annotations should be handled in `handle_unannotated_lines`), and all
            // annotations must be of type `Annotation::MultilineBody`.
            if (annotated_line.annotations.empty()
                || std::ranges::any_of(
                    annotated_line.annotations,
                    [](Annotation const& annotation) {
                        return annotation.type != Annotation::MultilineBody;
                    }
                )) {
                continue;
            }

            if (line_no - foldable_area_end != 1) {
                // If two consecutive lines are not adjacent, then this foldable area ends. The
                // range of this area is [foldable_area_beg, foldable_area_end].
                fold_lines(foldable_area_beg, foldable_area_end + 1);
                // Start a new foldable area.
                foldable_area_beg = line_no;
            }

            // Add the current line to the foldable area.
            foldable_area_end = line_no;
        }

        // Don't forget to handle the last foldable area.
        fold_lines(foldable_area_beg, foldable_area_end + 1);
    }

    /// Calculates the display offsets (i.e., the `display` member values) for all column
    /// numbers (`col_beg` and `col_end`) of annotations in `lines_`.
    ///
    /// Additionally, this function assigns source code lines to all unomitted annotations
    /// (`AnnotatedLine::source_line`) and calculates the display width of the source code lines
    /// (`AnnotatedLine::line_display_width`).
    void compute_display_columns(AnnotatedSource& source, unsigned display_tab_width) {
        for (auto& [line_no, annotated_line] : lines_) {
            if (annotated_line.omitted) {
                // Since we do not render source code for omitted lines, there is no need to
                // allocate source code for them or process their annotations.
                continue;
            }

            // Assigns the source code line.
            annotated_line.source_line = source.line_content(line_no);

            // Collect all columns to be processed.
            //
            // Although for some types of annotations, such as `MultilineHead` and `MultilineTail`,
            // their `col_beg` and `col_end` may not refer to columns in the source code, we still
            // treat them as such to simplify the code. These columns will be skipped when the
            // calculation results are written into `annotations`.

            // Stores the mapping from column byte offset to display offset. We aim to process these
            // columns in order to reduce the number of string traversals.
            std::map<unsigned, unsigned> col_display;
            for (Annotation const& annotation : annotated_line.annotations) {
                col_display.emplace(annotation.col_beg.byte, 0);
                col_display.emplace(annotation.col_end.byte, 0);
            }

            // We also need to include the length of the source line in `col_display` so that we can
            // calculate the display width of the source line simultaneously.
            col_display.emplace(annotated_line.source_line.size(), 0);

            for (unsigned display_width = 0, chunk_begin = 0; auto& [byte, display] : col_display) {
                std::string const normalized_source_chunk = normalize_source(
                    annotated_line.source_line.substr(chunk_begin, byte - chunk_begin),
                    display_tab_width
                );
                display_width += detail::display_width(normalized_source_chunk);

                if (byte > annotated_line.source_line.size()) {
                    // If `byte` exceeds the length of `annotated_line.source_line`, the user is
                    // attempting to annotate characters that do not exist in this line. We allow
                    // this, as the user might be annotating the end of this line to indicate
                    // something is missing. We treat these non-existent characters as spaces.
                    display_width += byte - annotated_line.source_line.size();
                    chunk_begin = annotated_line.source_line.size();
                } else {
                    chunk_begin = byte;
                }

                display = display_width;
            }

            // Finally, we write the results back to `annotated_line.annotations`.
            for (Annotation& annotation : annotated_line.annotations) {
                switch (annotation.type) {
                case Annotation::SingleLine:
                    // For `SingleLine`, both `col_beg` and `col_end` refer to the source code, so
                    // both need to be set.
                    annotation.col_beg.display = col_display.find(annotation.col_beg.byte)->second;
                    [[fallthrough]];
                case Annotation::MultilineHead:
                case Annotation::MultilineTail:
                    // For `MultilineHead` and `MultilineTail`, only `col_end` refers to the source
                    // code, while `col_beg` stores the depth of the multiline annotation.
                    annotation.col_end.display = col_display.find(annotation.col_end.byte)->second;
                    [[fallthrough]];
                default:
                    // For other categories, neither `col_beg` nor `col_end` refer to the source
                    // code.
                    break;
                }
            }

            // Calculate the display width of the source line.
            annotated_line.line_display_width =
                col_display.find(annotated_line.source_line.size())->second;
        }
    }

    /// Merges annotations with the same range to prevent assigning different rendering levels to
    /// these annotations.
    ///
    /// Typically, we do not generate multiple redundant annotations as shown in the following
    /// example:
    ///
    ///     func(arg)
    ///          ^^^ label1
    ///          |
    ///          label2
    ///          |
    ///          label3
    ///
    /// Instead, we merge them into a single annotation:
    ///
    ///     func(arg)
    ///          ^^^ label1
    ///              label2
    ///              label3
    static auto merge_annotations(std::vector<Annotation> annotations) -> std::vector<Annotation> {
        // We use `std::unordered_set` to eliminate duplicates in `annotations`, but not all members
        // participate in equality comparison. We only merge annotations that have the same range.
        //
        // For `SingleLine`, `col_beg` and `col_end` define the annotation range. For
        // `MultilineHead` and `MultilineTail`, if their `col_end` are the same, they are considered
        // to have the same range. Moreover, we need to distinguish multi-line annotations of
        // different depths, for example:
        //
        //     func(arg)
        //  _______^
        // |  _____|
        // | |
        //
        // In this example, the two multi-line annotations have the same `col_end` but different
        // depths. We cannot merge these annotations because they correspond to different parts of
        // the multi-line annotation's tail. Thus, for multi-line annotations, we also need to
        // consider the value of their `col_beg` member, because for multi-line annotations,
        // `col_beg` stores its depth.
        //
        // We also need to consider the type of `Annotation`: we cannot merge annotations of
        // different types, for example:
        //
        // |   func(arg)
        // |_______^
        //  _______|
        // |
        //
        // In this example, these two multi-line annotations have the same `col_beg` and `col_end`,
        // but one is `MultilineHead` and the other is `MultilineTail`. We cannot merge them.
        //
        // Therefore, we only consider merging annotations when `col_beg`, `col_end`, and `type` are
        // all the same.

        // Provides a hasher for `Annotation`. We do not specialize `std::hash<>` outside the class
        // because here we only deal with part of the `Annotation` members.
        auto const annotation_hasher = [](Annotation const& annotation) {
            std::size_t seed = 0;
            seed = hash_combine(seed, annotation.col_beg.display);
            seed = hash_combine(seed, annotation.col_end.display);
            seed = hash_combine(seed, annotation.type);
            return seed;
        };

        // Provides a comparator for `Annotation`. We do not overload `operator==` for `Annotation`
        // because we only deal with part of the `Annotation` members.
        auto const annotation_eq = [](Annotation const& lhs, Annotation const& rhs) {
            return std::tie(lhs.col_beg.display, lhs.col_end.display, lhs.type)
                == std::tie(rhs.col_beg.display, rhs.col_end.display, rhs.type);
        };

        std::unordered_set<Annotation, decltype(annotation_hasher), decltype(annotation_eq)>
            merged_annotations;

        for (Annotation& annotation : annotations) {
            auto const [target, inserted] = merged_annotations.insert(std::move(annotation));
            if (!inserted) {
                // If insertion fails, it means this annotation already exists, and we need to merge
                // these two annotations.
                auto handler = merged_annotations.extract(target);
                Annotation& old_annotation = handler.value();

                // Merge the labels of these two annotations: akin to creating a new line and adding
                // the new label to it.
                old_annotation.label.insert(
                    old_annotation.label.end(),
                    std::make_move_iterator(annotation.label.begin()),
                    std::make_move_iterator(annotation.label.end())
                );

                old_annotation.label_display_width = std::ranges::max(
                    old_annotation.label_display_width,
                    annotation.label_display_width
                );

                // We prioritize displaying primary annotations: if either of the annotations is
                // primary, the merged annotation should also be primary.
                old_annotation.is_primary = old_annotation.is_primary || annotation.is_primary;

                merged_annotations.insert(target, std::move(handler));
            }
        }

        std::vector<Annotation> result;
        result.reserve(merged_annotations.size());

        for (auto iter = merged_annotations.begin(); iter != merged_annotations.end();) {
            result.push_back(std::move(merged_annotations.extract(iter++).value()));
        }

        return result;
    }

    /// Assigns a rendering level to the annotation, i.e., calculates the value of the
    /// `Annotation::render_level` member. For information on the function of rendering levels,
    /// please refer to the documentation comment of `Annotation::render_level`.
    void assign_annotation_levels(
        HumanRenderer::LabelPosition label_position,
        AnnotatedLine& line
    ) {
        // Merges annotations with the same range.
        std::vector<Annotation> annotations = merge_annotations(std::move(line.annotations));

        // Now, we need to identify all annotations that can be rendered inline.
        //
        // Inline rendering means that labels and underlines appear on the same line, for example:
        //
        //     func(arg)
        //     ^^^^ ^^^ label1   <-- inline rendering
        //     |
        //     label2            <-- non-inline rendering
        //
        // Specifically, an annotation is considered inline if:
        //
        // 1. The annotation's label does not overlap with any other annotation's underline. Note
        // that overlap between labels, such as:
        //
        //     func(arg)
        //     ^^^^ ^^^ label1
        //     |
        //     long label2
        //
        // Here, "label1" does not overlap with any underlines but overlaps with "long label2". This
        // overlap doesn't prevent "label1" from being rendered inline.
        //
        // 2. The boundaries of the annotation's underline must be clear, meaning no other
        // annotation's underline should blend into the boundaries of this underline, for example:
        //
        //     func(arg)
        //     ^^^^^ label1
        //     |
        //     label2
        //
        // This would misleadingly suggest that the range of "label1" is "func(". Therefore,
        // "label1" should not be rendered inline but rather like this:
        //
        //     func(arg)
        //     ^^^^^
        //     |   |
        //     |   label1
        //     label2
        //
        // 3. For multiline annotations, if any other annotation's underline is on the left side of
        // its underline, it cannot be rendered inline, for example:
        //
        //     func(arg)
        //     ^^^^ ^
        //  ________|
        //
        // Here, we cannot render this multiline annotation inline (i.e., at render level 0) because
        // we need to raise its render level to avoid obscuring the annotation to its left.
        //
        // Furthermore, any single-line annotations without labels are considered for inline
        // rendering to reduce the computational overhead in subsequent calculations of the render
        // level.

        for (Annotation& self : annotations) {
            // If a single-line annotation does not contain a label, it is always rendered inline.
            if (self.type == Annotation::SingleLine && self.label.empty()) {
                continue;
            }

            // The display range for the annotation's label. Here, we consider the space occupied by
            // the label and one additional space on each side to ensure that no other annotation's
            // underline appears within this range:
            //
            //     func(args)
            //          ---- label
            //              ^^^^^^^
            //              This range is considered the label's display range, within which no
            //              other annotation's underline should appear.
            unsigned const label_beg = self.col_end.display;
            unsigned const label_end = label_beg + self.label_display_width + 2;

            // The display range for the current annotation's underline.
            auto const [underline_beg, underline_end] = self.underline_display_range();

            // Compare the current annotation with others. We do not need to check if `self` and
            // `other` are the same object, because an annotation's underline will neither overlap
            // with its own label nor merge with the boundaries of its own underline.
            for (Annotation const& other : annotations) {
                // The underline range for `other`.
                auto const [other_beg, other_end] = other.underline_display_range();

                // Check if `self`'s label overlaps with any annotation's underline.
                //
                // Note that, although not explicitly stated, we still consider cases where
                // `other`'s underline range might be empty (e.g., when `other.type` is
                // `MultilineBody`): if `other_beg` and `other_end` are equal, this `if` statement
                // will not be executed, thus not affecting the result.
                if (std::ranges::max(label_beg, other_beg)
                    < std::ranges::min(label_end, other_end)) {
                    self.render_level = 1;
                    break;
                }

                // Check if `self`'s underline can be distinctly rendered. We only need to verify
                // that no other annotation's underline obscures the ends of `self`'s underline.
                // Note that this is different from checking if there is an overlap of underlines,
                // for example:
                //
                //     func(args)
                //         ^^^^^^
                //          ----
                //
                // Although the underlines ^ and - overlap here, we can still render the underline ^
                // inline, because its start and end positions are clear and unobstructed.
                //
                // Note that, although not explicitly stated, we still consider cases where
                // `other`'s underline range might be empty: if `other_beg` and `other_end` are
                // equal, this `if` statement will not be executed, thus not affecting the result.
                if (other_beg < underline_beg && underline_beg <= other_end
                    || other_beg <= underline_end && underline_end < other_end) {
                    self.render_level = 1;
                    break;
                }

                // If the current annotation is the head or tail of a multiline annotation, check if
                // there is an underline from another annotation on its left side.
                if (self.type == Annotation::MultilineHead
                    || self.type == Annotation::MultilineTail) {
                    // Note that we need to explicitly consider whether `other`'s underline range is
                    // empty. We also need to check whether `self` and `other` are the same object,
                    // because we are using `<=` here.
                    if (other_beg != other_end && other_beg <= underline_beg && &self != &other) {
                        self.render_level = 1;
                        break;
                    }
                }
            }
        }

        // Next, assign render levels to all annotations that cannot be rendered inline. We follow
        // these three principles:
        //
        // 1. For two annotations A and B, with display ranges for their labels [a1, a2) and [b1,
        // b2) respectively. If a1 < b1 <= a2, then the render level of A should be greater than
        // that of B.
        //
        // This rule addresses the scenario as follows:
        //
        //     func(arg)
        //     ^^^^ ^^^
        //     |    |
        //     |    label1
        //     long label2
        //
        // Here, the render level of 'long label2' should be higher than that of 'label1'.
        //
        // 2. For two annotations A and B, with label display ranges [a1, a2) and [b1, b2)
        // respectively. If a1 == b1, then the later occurring annotation should have a higher
        // render level. This simply provides a consistent order for annotations A and B, as seen in
        // the situation below:
        //
        //     func(arg)
        //     ^^^^
        //     |
        //     label1
        //     label2
        //
        // 3. For multiline annotations A and B, with display ranges [a1, a2) and [b1, b2)
        // respectively. If b2 < a1, then the render level of A should be greater than that of B.
        //
        // This rule is demonstrated in the following scenario:
        //
        //     func(arg)
        //     ^^^^ ^^^
        //     |    |
        //     text |
        // _________|
        //
        // To assign render levels to all annotations and satisfy the above requirements, we
        // construct a directed graph and use topological sorting to allocate levels for each
        // annotation. If annotation A should be higher than annotation B, there should be a
        // directed edge from B to A.

        /// Vertices of the directed graph.
        struct Vertex {
            /// The annotation corresponding to this vertex.
            Annotation* annotation;
            /// The indegree of this vertex.
            unsigned indegree;
            // All successor neighbor vertices of this vertex.
            std::vector<Vertex*> neighbors;

            Vertex() = default;
            explicit Vertex(Annotation& annotation) : annotation(&annotation), indegree(0) { }
        };

        class AnnotationGraph {
        public:
            // clang-format off
            AnnotationGraph(
                std::vector<Annotation>& annotations,
                HumanRenderer::LabelPosition label_position
            ) :
                label_position_(label_position)
            // clang-format on
            {
                vertices_.reserve(annotations.size());
                for (Annotation& annotation : annotations) {
                    // Add all annotations that cannot be rendered inline (i.e., those with a render
                    // level not equal to 0) to the vertex set.
                    if (annotation.render_level != 0) {
                        vertices_.emplace_back(annotation);
                    }
                }

                build_graph();
            }

            /// Assign render levels to annotations associated with vertices via topological
            /// sorting.
            void assign_levels() const {
                std::queue<Vertex const*> vertex_queue;
                // Add all vertices with an indegree of 0 to the queue.
                for (Vertex const& vertex : vertices_) {
                    if (vertex.indegree == 0) {
                        vertex_queue.push(&vertex);
                    }
                }

                while (!vertex_queue.empty()) {
                    Vertex const* const cur_vertex = vertex_queue.front();
                    vertex_queue.pop();

                    unsigned const required_level = cur_vertex->annotation->render_level + 1;

                    for (Vertex* const neighbor : cur_vertex->neighbors) {
                        // Calculate the maximum level required by all predecessor nodes for
                        // `neighbor`.
                        neighbor->annotation->render_level =
                            std::ranges::max(neighbor->annotation->render_level, required_level);

                        if (--neighbor->indegree == 0) {
                            vertex_queue.push(neighbor);
                        }
                    }
                }
            }

        private:
            std::vector<Vertex> vertices_;
            HumanRenderer::LabelPosition label_position_;

            static void add_edge(Vertex& from, Vertex& to) {
                from.neighbors.push_back(&to);
                ++to.indegree;
            }

            /// Build edges in the directed graph based on the three rules above: if vertex A's
            /// level needs to be greater than vertex B's, establish an edge from B to A.
            void build_graph() {
                for (Vertex& self : vertices_) {
                    auto const [self_beg, self_end] =
                        self.annotation->label_display_range(label_position_);
                    bool const is_multiline = self.annotation->type == Annotation::MultilineHead
                        || self.annotation->type == Annotation::MultilineTail;

                    for (Vertex& other : vertices_) {
                        auto const [other_beg, other_end] =
                            other.annotation->label_display_range(label_position_);

                        // Rule 1: If a1 < b1 <= a2, then A's render level should be greater than
                        // B's.
                        if (self_beg < other_beg && other_beg <= self_end) {
                            // `self`'s level should be greater than `other`'s, hence an edge is
                            // established from `other` to `self`.
                            add_edge(other, self);
                        }

                        // Rule 2: If a1 == b1, the later occurring annotation should have a greater
                        // render level.
                        if (self_beg == other_beg && self.annotation < other.annotation) {
                            // `other`'s level should be greater than `self`'s, hence an edge is
                            // established from `self` to `other`.
                            add_edge(self, other);
                        }

                        // Rule 3: If b2 < a1 and A is a multiline annotation, then A's render level
                        // should be at least 2 greater than B's.
                        if (is_multiline && other_end < self_beg) {
                            // `self`'s level should be greater than `other`'s, hence an edge is
                            // established from `other` to `self`.
                            add_edge(other, self);
                        }
                    }
                }
            }
        };

        AnnotationGraph const annotation_graph(annotations, label_position);
        // Assign levels to all annotations that cannot be rendered inline. Since `AnnotationGraph`
        // does not own the `Annotation` objects, the results are directly written back to the
        // original `Annotation` objects.
        annotation_graph.assign_levels();

        line.annotations = std::move(annotations);
    }
};

void render_annotated_source(
    StyledString& render_target,
    AnnotatedSource& source,
    HumanRenderer const& renderer,
    unsigned max_line_num_len
) {
    // Add an empty line between the (filename:line number:column number) tuple and the rendered
    // source code to separate them.
    render_line_number(render_target, max_line_num_len);

    AnnotatedLines annotated_lines = AnnotatedLines::from_source(source, renderer);
    for (auto& [line_num, line] : annotated_lines.annotated_lines()) {
        render_target.append_newline();

        line.render(
            render_target,
            max_line_num_len,
            line_num + source.first_line_number(),
            annotated_lines.depth_num(),
            renderer
        );
    }
}
}  // namespace

void HumanRenderer::render_annotated_sources(
    StyledString& render_target,
    std::vector<AnnotatedSource>& sources,
    unsigned max_line_num_len
) const {
    for (unsigned source_idx = 0; AnnotatedSource & source : sources) {
        if (source.primary_spans().empty() && source.secondary_spans().empty()) {
            continue;
        }

        render_target.append_newline();
        render_file_line_col(render_target, source, max_line_num_len, source_idx == 0);

        render_target.append_newline();
        render_annotated_source(render_target, source, *this, max_line_num_len);

        ++source_idx;
    }
}
}  // namespace ants
