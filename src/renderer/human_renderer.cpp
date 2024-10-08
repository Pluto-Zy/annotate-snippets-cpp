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
        render_target.append("= ", Style::LineNumber);
        // We have already rendered a number of spaces equal to `max_line_num_len + 1` and a width
        // of 2 for "= ".
        indentation += max_line_num_len + 3;
    }

    Style const title_style = is_secondary ? Style::SecondaryTitle : Style::PrimaryTitle;

    // Render the diagnostic level title (such as "error"). The colon following "error" is rendered
    // later because the error code may be inserted between "error" and ": ".
    render_target.append(level_title, title_style);
    indentation += static_cast<unsigned>(detail::display_width(level_title));

    // If there is an error code, render it.
    if (!err_code.empty()) {
        std::string rendered_err_code = "[";
        rendered_err_code.append(err_code);
        rendered_err_code.push_back(']');

        render_target.append(rendered_err_code, title_style);
        indentation += static_cast<unsigned>(detail::display_width(rendered_err_code));
    }

    render_target.append(": ", title_style);
    indentation += 2;

    // Render the title message.
    render_multiline_messages(
        render_target,
        message,
        indentation,
        is_secondary ? Style::SecondaryMessage : Style::PrimaryMessage
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
        render_target.append(source.origin(), Style::OriginAndLocation);
        render_target.append(":", Style::OriginAndLocation);

        SourceLocation const loc = source.primary_spans().front().beg;
        std::string const line = std::to_string(loc.line + source.first_line_number());
        std::string const col = std::to_string(loc.col + 1);

        // Render the line number and column number.
        render_target.append(line, Style::OriginAndLocation);
        render_target.append(":", Style::OriginAndLocation);
        render_target.append(col, Style::OriginAndLocation);

        // When rendering a short message, we need to additionally render a ": " at the end.
        render_target.append(": ", Style::OriginAndLocation);

        // Compute the width of the part already rendered. Since we've also drawn one ": " and two
        // ':', we need to add 4.
        final_width = static_cast<unsigned>(
            detail::display_width(source.origin()) + line.size() + col.size() + 4
        );
        ++idx;
    }

    return final_width;
}

namespace {
/// Renders the line number and its separator portion without the line number itself.
void render_line_number(StyledString& render_target, unsigned max_line_num_len) {
    render_target.append_spaces(max_line_num_len + 1);
    render_target.append("|", Style::LineNumber);
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
        render_target.append(line_num_str, Style::LineNumber);
        // Adds sufficient spaces to align the separator.
        render_target.append_spaces(max_line_num_len + 1 - line_num_str.size());
        break;
    case HumanRenderer::AlignRight:
        // Adds sufficient spaces to ensure the line number text is right-aligned.
        render_target.append_spaces(max_line_num_len - line_num_str.size());
        render_target.append(line_num_str, Style::LineNumber);
        // Adds a single space between the line number and the separator.
        render_target.append_spaces(1);
        break;
    default:
        detail::unreachable();
    }

    render_target.append("|", Style::LineNumber);
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
        render_target.append("--> ", Style::LineNumber);
    } else {
        // Since the current source is not the first, we first add an empty line.
        render_line_number(render_target, max_line_num_len);
        render_target.append_newline();

        render_target.append_spaces(max_line_num_len);
        render_target.append("::: ", Style::LineNumber);
    }

    // Render the file name.
    render_target.append(source.origin(), Style::OriginAndLocation);

    if (!source.primary_spans().empty()) {
        render_target.append(":", Style::OriginAndLocation);

        SourceLocation const loc = source.primary_spans().front().beg;
        std::string const line = std::to_string(loc.line + source.first_line_number());
        std::string const col = std::to_string(loc.col + 1);

        // Render the line number and column number.
        render_target.append(line, Style::OriginAndLocation);
        render_target.append(":", Style::OriginAndLocation);
        render_target.append(col, Style::OriginAndLocation);
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
    std::hash<T> const hasher;
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
    /// Represents the index of the line where the current label should be rendered. When labels may
    /// overlap with other elements (such as other annotations' underlines or connecting lines), we
    /// need to adjust the line of the label to reduce overlaps.
    ///
    /// We consider the line index of the underline as 0, for example:
    ///
    ///     func(args)
    ///     ^^^^ ^^^^ label1
    ///     |
    ///     label2
    /// Here, the `label_line_position` of "label1" is 0, and for "label2" it is 2.
    ///
    /// For multiline annotations, `label_line_position` also indicates the position of its
    /// horizontal connection lines, regardless of whether the annotation has an associated label.
    /// Specifically, if we assume that the multiline annotation has an associated label, then its
    /// horizontal connection line is located on the line above where the label is positioned. For
    /// example:
    ///
    ///     func(args)
    /// ________^    ^
    /// _____________|  <-- `label_line_position` is 2, hence its horizontal connection line is
    ///                     drawn on line 1.
    unsigned label_line_position;
    /// The type of the current annotation.
    enum AnnotationType : std::uint8_t {
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
    /// If the annotation is rendered inline (i.e., `label_line_position` is 0), the label follows
    /// the annotation's underline:
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
            if (label_line_position == 0) {
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
        label_line_position(0),
        type(type),
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
                        width += static_cast<unsigned>(detail::display_width(content));
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
        // labels.
        unsigned const annotation_line_count = [&] {
            if (annotations.empty()) {
                return 0u;
            } else {
                return std::ranges::max(
                    annotations | std::views::transform([](Annotation const& annotation) {
                        // Skip `MultilineBody` annotations, as these do not have associated
                        // underlines or labels.
                        if (annotation.type == Annotation::MultilineBody) {
                            return 0u;
                        } else {
                            unsigned const label_end_line = annotation.label_line_position
                                + static_cast<unsigned>(annotation.label.size());

                            if (annotation.label_line_position == 0) {
                                // Since the first line is used for drawing the underline, we need
                                // at least 1 line.
                                return std::ranges::max(label_end_line, 1u);
                            } else {
                                return label_end_line;
                            }
                        }
                    })
                );
            }
        }();

        // We create a `StyledString` for each line to facilitate later rendering.
        std::vector<StyledString> annotation_lines(annotation_line_count);

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
        // lines, and when vertical lines overlap, annotations with labels starting on earlier lines
        // should be rendered above. We expect to achieve an output like this:
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
        render_horizontal_lines(annotation_lines, source_code_indentation);

        // Next, we render the vertical connection lines in the required order.
        render_vertical_lines(
            annotation_lines,
            source_code_indentation,
            human_renderer.label_position
        );

        // Render all labels.
        render_labels(annotation_lines, source_code_indentation, human_renderer.label_position);

        // Render all underlines for the annotations.
        render_underlines(
            annotation_lines,
            source_code_indentation,
            human_renderer.primary_underline,
            human_renderer.secondary_underline
        );

        // At this point, all annotations have been rendered. We will render the results into the
        // render target.

        // Render the source code line.
        StyledString const source_code_line = render_source_line(
            max_line_num_len,
            line_num,
            depth_num,
            human_renderer.line_num_alignment,
            human_renderer.display_tab_width
        );
        render_target.append(source_code_line.styled_line_parts().front());

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
    /// Renders horizontal connection lines for the heads and tails of multiline annotations. For
    /// example:
    ///
    /// 1 |      func(args)
    ///   |          ^
    ///   |  ________|      <-- Render this horizontal connection line.
    void render_horizontal_lines(
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
                // the label is on the first line (i.e., inline format), it is inserted in the line
                // of the label, otherwise in the line above the label.
                unsigned const line_idx =
                    annotation.label_line_position == 0 ? 0 : annotation.label_line_position - 1;

                annotation_lines[line_idx].set_styled_content(
                    connector_beg,
                    std::string(connector_end - connector_beg, '_'),
                    annotation.is_primary ? Style::PrimaryUnderline : Style::SecondaryUnderline
                );
            }
        }
    }

    /// Renders vertical connection lines for annotations. For all annotations whose labels are not
    /// on the first line (i.e., rendered in non-inline form), we need to render vertical lines
    /// connecting their underlines to their labels:
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
        std::span<StyledString> annotation_lines,
        unsigned source_code_indentation,
        HumanRenderer::LabelPosition label_position
    ) {
        // We render from back to front according to the order of the lines where the labels are
        // located, to ensure the correct overlap relationship.
        std::ranges::sort(annotations, [&](Annotation const& lhs, Annotation const& rhs) {
            return rhs.label_line_position < lhs.label_line_position;
        });

        // Draw all vertical connecting lines in the sorted order.
        for (Annotation const& annotation : annotations) {
            // Style of the connecting lines.
            Style const connector_style =
                annotation.is_primary ? Style::PrimaryUnderline : Style::SecondaryUnderline;

            // When the label is not on the first line, render the vertical line connecting the
            // underline to the label.
            if (annotation.label_line_position != 0) {
                // Position of the connecting line.
                unsigned const connector_position =
                    std::get<0>(annotation.label_display_range(label_position))
                    + source_code_indentation;

                for (StyledString& line :
                     annotation_lines.subspan(1, annotation.label_line_position - 1)) {
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
                        | std::views::drop(std::ranges::max(1u, annotation.label_line_position))
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
                        | std::views::take(std::ranges::max(1u, annotation.label_line_position))
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

                // Render the label line by line.
                for (unsigned const line_idx :
                     std::views::iota(0u, static_cast<unsigned>(annotation.label.size()))) {
                    // The target for the `line_idx` line of the label.
                    StyledString& target_line =
                        annotation_lines[annotation.label_line_position + line_idx];
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
                    annotation.is_primary ? Style::PrimaryUnderline : Style::SecondaryUnderline,
                    depth,
                    depth + 1
                );
            }
        }

        StyledString render_target;

        if (omitted) {
            // If the code line is omitted, render it as "...".
            render_target.append("...", Style::LineNumber);

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
                Style::SourceCode
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
    /// 6. Calculates the lines on which labels are placed to minimize overlaps. Implemented by
    ///    `compute_label_line_positions()`.
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
            AnnotatedLines::compute_label_line_positions(renderer.label_position, line);
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

            Vertex() = default;
            explicit Vertex(std::span<MultilineAnnotation> range) : annotation_range(range) { }

            /// Determines whether the intervals represented by two `Vertex` overlap.
            auto overlap(Vertex const& other) const -> bool {
                // We assume that the `annotation_range` in both vertices is non-empty, and that all
                // elements in an `annotation_range` of a `Vertex` have the same line range.
                unsigned const self_line_beg = annotation_range.front().beg.line;
                unsigned const self_line_end = annotation_range.front().end.line;
                unsigned const other_line_beg = other.annotation_range.front().beg.line;
                unsigned const other_line_end = other.annotation_range.front().end.line;

                // Note that for line numbers, the interval (e.g., [`self_line_beg`,
                // `self_line_end`]) is inclusive on both ends.
                return (self_line_beg <= other_line_beg && other_line_beg <= self_line_end)
                    || (other_line_beg <= self_line_beg && self_line_beg <= other_line_end);
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
            interval_graph.emplace_back(std::span(iter, end_iter));

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
            vertex.depth =
                static_cast<unsigned>(std::ranges::distance(depth_bucket.begin(), first_unused));
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
        auto foldable_area_end = static_cast<unsigned>(-1);
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
            col_display.emplace(static_cast<unsigned>(annotated_line.source_line.size()), 0u);

            for (unsigned display_width = 0, chunk_begin = 0; auto& [byte, display] : col_display) {
                std::string const normalized_source_chunk = normalize_source(
                    annotated_line.source_line.substr(chunk_begin, byte - chunk_begin),
                    display_tab_width
                );
                display_width +=
                    static_cast<unsigned>(detail::display_width(normalized_source_chunk));

                if (byte > annotated_line.source_line.size()) {
                    // If `byte` exceeds the length of `annotated_line.source_line`, the user is
                    // attempting to annotate characters that do not exist in this line. We allow
                    // this, as the user might be annotating the end of this line to indicate
                    // something is missing. We treat these non-existent characters as spaces.
                    display_width +=
                        byte - static_cast<unsigned>(annotated_line.source_line.size());
                    chunk_begin = static_cast<unsigned>(annotated_line.source_line.size());
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
                col_display.find(static_cast<unsigned>(annotated_line.source_line.size()))->second;
        }
    }

    /// Merge annotations with the same range to prevent the generation of visually unappealing
    /// multiline annotation renderings.
    ///
    /// Typically, we do not generate multiple redundant annotations as shown in the following
    /// example:
    ///
    /// xx | |     func(args)
    ///    | |_________^ label1
    ///    | |_________|
    ///    |           label2
    ///
    ///
    /// Instead, we merge them into a single annotation:
    ///
    /// xx | |     func(args)
    ///    | |_________^ label1
    ///    |             label2
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
                    // NOLINTBEGIN(bugprone-use-after-move): If insertion fails, `annotation` won't
                    // be moved away.
                    std::make_move_iterator(annotation.label.begin()),
                    std::make_move_iterator(annotation.label.end())
                    // NOLINTEND(bugprone-use-after-move)
                );

                old_annotation.label_display_width = std::ranges::max(
                    old_annotation.label_display_width,
                    annotation.label_display_width
                );

                // We prioritize displaying primary annotations: if either of the annotations is
                // primary, the merged annotation should also be primary.
                old_annotation.is_primary = old_annotation.is_primary || annotation.is_primary;

                // Note that we cannot use `target` here because the iterator pointing to the
                // extracted element is invalid.
                merged_annotations.insert(std::move(handler));
            }
        }

        std::vector<Annotation> result;
        result.reserve(merged_annotations.size());

        for (auto iter = merged_annotations.begin(); iter != merged_annotations.end();) {
            result.push_back(std::move(merged_annotations.extract(iter++).value()));
        }

        return result;
    }

    /// Calculates the position of the first line of the annotation's label, i.e., the value of the
    /// `Annotation::label_line_position` member.
    static void compute_label_line_positions(
        HumanRenderer::LabelPosition label_position,
        AnnotatedLine& line
    ) {
        // Merges annotations with the same range.
        std::vector<Annotation> annotations = merge_annotations(std::move(line.annotations));

        // Now, we need to identify all annotations that can be rendered inline. For annotations
        // that can be rendered inline, set their `label_line_position` to 0, otherwise set it to 1.
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
        // Here, we cannot render this multiline annotation inline because we need to raise its
        // label position to avoid obscuring the annotation to its left.
        //
        // Furthermore, any single-line annotations without labels are considered for inline
        // rendering to reduce the computational overhead in subsequent calculations of the label
        // position.

        // Height of the first line. We need to count the number of lines for labels of annotations
        // that will be rendered inline, to determine from which line we should start rendering the
        // labels of annotations that are not rendered inline.
        unsigned first_line_height = 1;

        for (Annotation& self : annotations) {
            // If a single-line annotation does not contain a label, it is always rendered inline.
            if (self.type != Annotation::MultilineHead && self.type != Annotation::MultilineTail
                && self.label.empty()) {
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
                    self.label_line_position = 1;
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
                if ((other_beg < underline_beg && underline_beg <= other_end)
                    || (other_beg <= underline_end && underline_end < other_end)) {
                    self.label_line_position = 1;
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
                        self.label_line_position = 1;
                        break;
                    }
                }
            }

            if (self.label_line_position == 0) {
                first_line_height =
                    std::ranges::max(first_line_height, static_cast<unsigned>(self.label.size()));
            }
        }

        // Next, compute label positions for all annotations that cannot be rendered inline. We
        // follow these three principles:
        //
        // 1. For two annotations A and B, with display ranges for their labels [a1, a2) and [b1,
        // b2) respectively. If a1 < b1 <= a2, then the line of A's label should be after the line
        // of B's label.
        //
        // This rule addresses the scenario as follows:
        //
        //     func(arg)
        //     ^^^^ ^^^
        //     |    |
        //     |    label1  <-- B
        //     long label2  <-- A
        //
        // 2. For two annotations A and B, with label display ranges [a1, a2) and [b1, b2)
        // respectively. If a1 == b1, then A's label should be placed above B's label under the
        // following conditions:
        //
        // (1) A's label is shorter than B's label. The reason for placing the shorter label above
        // is because we expect the output to look like this:
        //
        //     func(args)
        //     ^     ^
        //     |     |
        //     label |
        // ____|_____|
        //     |     label2
        //     label3
        //
        // rather than:
        //
        //     func(args)
        //     ^     ^
        // ____|_____|
        //     |     label2
        //     label3
        //     label
        //
        // (2) Otherwise, if A is a single-line annotation and B is a multi-line annotation.
        //
        // (3) Otherwise, if A is the tail of a multi-line annotation and B is the head of a
        // multi-line annotation. This can reduce the number of crossing lines to some extent:
        //
        // |      func(args)
        // |__________^
        //  __________|
        // |
        //
        // (4) Otherwise, if both A and B are single-line annotations and A's underline is shorter
        // than B's. This condition is not for the aesthetic of the rendering, but to assign a
        // uniform order to all single-line annotations. Note that if A and B are both single-line
        // annotations, their underline lengths will not be the same, as annotations with the same
        // range would have already been merged.
        //
        // (5) Otherwise, if both A and B are heads of multi-line annotations and A's depth is less.
        // This can reduce the number of crossing lines:
        //
        //        func(args)
        //  __________^
        // | _________|
        // ||
        //
        // (6) Otherwise, if both A and B are tails of multi-line annotations and A's depth is
        // greater. This can reduce the number of crossing lines:
        //
        // ||     func(args)
        // ||_________^
        // |__________|
        //
        // 3. For multiline annotations A and B, with display ranges [a1, a2) and [b1, b2)
        // respectively. If b2 < a1, then the line of the horizontal connecting line of multiline
        // annotation A should be after the line of B's label. Don't forget that the label of a
        // multiline annotation will further be one line after its horizontal connection line.
        //
        // This rule is demonstrated in the following scenario:
        //
        //     func(arg)
        //     ^^^^ ^^^
        //     |    |
        //     text |
        // _________|
        //
        // 4. The label positions of all multiline annotations cannot be the same, as the horizontal
        // connection lines of all multiline annotations must be on different lines.
        //
        // To assign label positions to all annotations and satisfy the above requirements, we
        // construct a directed graph and use topological sorting to allocate levels for each
        // annotation. If the first line of the label of annotation A should be n lines after the
        // last line of the label of annotation B, there exists a directed edge from B to A with a
        // weight of n. For Rule 4, when assigning label positions to each annotation based on the
        // topological order, we need to check whether Rule 4 is violated.
        //
        // Note that in some cases we encounter cyclic dependencies, where a cycle appears in the
        // directed graph we construct. For example:
        //
        // 3 | ||||     std::cout << "World";
        //   | ||||     -  ^  -  ^
        //   | ||||_____|__|__|__|
        //   |  |||_____|__|__|  label3
        //   |   ||_____|__|  label4
        //   |    |_____|  label5
        //   |          label6
        //
        // According to Rule 1, labels from `label3` to `label6` should be arranged in the order
        // provided above. However, Rule 3 requires that `label3` should be positioned below
        // `label6`, creating a contradiction with Rule 1. This contradiction results in a cycle in
        // the directed graph we constructed, making topological sorting infeasible.
        //
        // To address this, we maintain a Disjoint Set Union while constructing the directed graph
        // and add all overlapping annotations to the same set. When applying Rule 3, we not only
        // check the overlap between multiline annotations A and B, but also check those annotations
        // that belong to the same set as B, ensuring that they do not overlap with annotation A.
        // See https://github.com/Pluto-Zy/annotate-snippets-cpp/pull/3 for more information.

        /// Vertices of the directed graph.
        struct Vertex {
            // The following two members are used to organize annotations into a directed graph.

            /// The annotation corresponding to this vertex.
            Annotation* annotation;
            // All successor neighbor vertices of this vertex, each associated with a weight
            // representing the weight of the directed edge from this vertex to the neighbor.
            std::vector<std::pair<Vertex*, unsigned>> neighbors;

            // The following two members maintain a disjoint set union for the vertices.

            /// The parent of this node in its set (tree).
            Vertex* parent;
            /// The maximum value of the right endpoint of all annotations in the subtree rooted at
            /// this node.
            unsigned rightmost;

            /// The indegree of this vertex.
            unsigned indegree;

            Vertex() = default;
            explicit Vertex(Annotation& annotation, HumanRenderer::LabelPosition label_position) :
                annotation(&annotation),
                parent(this),
                rightmost(std::get<1>(annotation.label_display_range(label_position))),
                indegree(0) { }

            /// Since the member `parent` might point to the object itself, we manually implement
            /// the move constructor and move assignment operator to correctly handle the situation
            /// where `parent` points to `*this`. However, this does not handle all cases, such as
            /// when an object is moved, all objects that have this object as their `parent` should
            /// update their `parent`, which we cannot do via the constructor. In practice, however,
            /// we do not move them after creating all nodes (for example, we reserve enough space
            /// in the `vector` to avoid triggering object moves), thus avoiding this issue.
            /// Moreover, we do not even need this move constructor, as it will not be called, but
            /// we still provide it to remind developers to consider the case where `parent` points
            /// to itself.
            Vertex(Vertex&& other) noexcept :
                annotation(other.annotation),
                neighbors(std::move(other.neighbors)),
                parent(other.parent == &other ? this : other.parent),
                rightmost(other.rightmost),
                indegree(other.indegree) { }

            auto operator=(Vertex&& other) noexcept -> Vertex& {
                if (this != &other) {
                    std::tie(annotation, rightmost, indegree) =
                        std::tie(other.annotation, other.rightmost, other.indegree);

                    neighbors = std::move(other.neighbors);
                    parent = (other.parent == &other ? this : other.parent);
                }

                return *this;
            }

            /// Returns the root of the set to which this vertex belongs.
            auto root() -> Vertex& {
                if (parent == this) {
                    return *this;
                } else {
                    // We perform path compression while finding the root.
                    parent = &parent->root();
                    return *parent;
                }
            }

            /// Merges the sets containing `*this` and `other` and updates the `rightmost` value of
            /// the root node.
            void merge(Vertex& other) {
                Vertex& self_root = root();
                Vertex& other_root = other.root();

                other_root.parent = &self_root;
                self_root.rightmost = std::ranges::max(self_root.rightmost, other_root.rightmost);
            }

            /// Queries the `rightmost` of the set to which this node belongs. Essentially, this
            /// involves finding the maximum right endpoint of all annotations that overlap with
            /// this annotation, and recursively, the rightmost endpoint of all annotations that
            /// overlap with those overlapping annotations.
            auto query_rightmost() -> unsigned {
                return root().rightmost;
            }

            auto is_multiline() const -> bool {
                return annotation->type == Annotation::MultilineHead
                    || annotation->type == Annotation::MultilineTail;
            }

            auto label_line_position() const -> unsigned {
                return annotation->label_line_position;
            }

            // NOLINTNEXTLINE(readability-make-member-function-const)
            auto label_line_position() -> unsigned& {
                return annotation->label_line_position;
            }
        };

        class AnnotationGraph {
        public:
            // clang-format off
            AnnotationGraph(
                std::vector<Annotation>& annotations,
                HumanRenderer::LabelPosition label_position,
                unsigned first_line_height
            ) :
                label_position_(label_position)
            // clang-format on
            {
                vertices_.reserve(annotations.size());

                // Determine the starting line for the labels of all annotations that are not drawn
                // inline. Here, we need to distinguish between single-line and multi-line
                // annotations.
                //
                // For single-line annotations, we need to draw vertical lines from
                // the underline to their labels, as shown:
                //
                //     func(args)
                //     ^^^^ ^^^^ label
                //     |            <-- This vertical line is essential for all non-inline
                //     label            annotations.
                //
                // We need to ensure there is enough space to draw this vertical line. If the height
                // of the first line is only 1, we need to add another line. However, if the height
                // of the first line is more than 1, there is no need to add another line:
                //
                //     func(args)
                //     ^^^^ ^^^^ line1
                //     |         line2  <-- No need to add another line
                //     label
                //
                // For multi-line annotations, we always need to add a new line to draw the
                // horizontal connecting line:
                //
                //     func(args)
                //     ^    ^^^^ label
                // ____|                <-- A new line must be added
                //
                //     func(args)
                //     ^    ^^^^ line1
                //     |         line2
                // ____|                <-- A new line must be added
                unsigned const singleline_beg = std::ranges::max(first_line_height, 2u);
                unsigned const multiline_beg = first_line_height + 1;

                for (Annotation& annotation : annotations) {
                    // Add all annotations that cannot be rendered inline (i.e., those with a label
                    // position not equal to 0) to the vertex set.
                    if (annotation.label_line_position != 0) {
                        // All annotations that need to be rendered in non-inline form should start
                        // rendering from `first_line_height`.
                        if (annotation.type == Annotation::MultilineHead
                            || annotation.type == Annotation::MultilineTail) {
                            annotation.label_line_position = multiline_beg;
                        } else {
                            annotation.label_line_position = singleline_beg;
                        }
                        vertices_.emplace_back(annotation, label_position);
                    }
                }

                build_graph();
            }

            /// Assign label line positions to annotations associated with vertices via topological
            /// sorting.
            void assign_label_line_positions() const {
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

                    // The end position of current annotation's label.
                    unsigned const cur_label_end_position = cur_vertex->label_line_position()
                        + static_cast<unsigned>(cur_vertex->annotation->label.size());

                    for (auto const& [neighbor, weight] : cur_vertex->neighbors) {
                        // Determine the line on which the neighbor's label will be based on the
                        // last line of current node's label and the edge's weight.
                        neighbor->label_line_position() = std::ranges::max(
                            neighbor->label_line_position(),
                            cur_label_end_position + weight
                        );

                        if (cur_vertex->is_multiline() && neighbor->is_multiline()) {
                            // Check if these two multiline annotations have the same label
                            // position.
                            if (cur_vertex->label_line_position()
                                == neighbor->label_line_position()) {
                                // According to Rule 4, the label positions of two multiline
                                // annotations cannot be the same. Since we require the label
                                // position of `neighbor` to be greater than that of `cur_vertex`,
                                // we increase the label position of `neighbor` to differentiate
                                // them.
                                ++neighbor->label_line_position();
                            }
                        }

                        if (--neighbor->indegree == 0) {
                            vertex_queue.push(neighbor);
                        }
                    }
                }
            }

        private:
            std::vector<Vertex> vertices_;
            HumanRenderer::LabelPosition label_position_;

            static void add_edge(Vertex& from, Vertex& to, unsigned weight) {
                from.neighbors.emplace_back(&to, weight);
                ++to.indegree;
            }

            /// Build edges in the directed graph based on the three rules mentioned above: If the
            /// first line of annotation A's label should be n lines after the last line of
            /// annotation B's label, then establish a directed edge from B to A with a weight of n.
            void build_graph() {
                // We first check Rules 1 and 2, and build the DSU alongside constructing the graph.
                for (Vertex& self : vertices_) {
                    auto const [self_beg, self_end] =
                        self.annotation->label_display_range(label_position_);

                    for (Vertex& other : vertices_) {
                        auto const [other_beg, other_end] =
                            other.annotation->label_display_range(label_position_);

                        // Rule 1: If a1 < b1 <= a2, then the first line of A's label should be
                        // after B's label.
                        if (self_beg < other_beg && other_beg <= self_end) {
                            add_edge(other, self, /*weight=*/0);

                            // `self` and `other` overlap, so we merge their sets.
                            //
                            // Note that we require `self` and `other` to overlap but not contain
                            // each other, otherwise, the constraints would be too strict. See
                            // https://github.com/Pluto-Zy/annotate-snippets-cpp/pull/3#issuecomment-2241049973
                            // for more information.
                            if (self_end < other_end && &self.root() != &other.root()) {
                                self.merge(other);
                            }
                        }

                        // Rule 2: If a1 == b1, then A's label should only be placed above B's label
                        // under a series of conditions.
                        if (self_beg == other_beg) {
                            // Determine whether an edge from `self` to `other` should be added
                            // according to a series of conditions described in Rule 2 .
                            bool const should_add_edge = [&] {
                                if (self_end != other_end) {
                                    // Condition (1): The shorter label is placed above.
                                    return self_end < other_end;
                                } else if (self.annotation->type != other.annotation->type) {
                                    // Conditions (2) and (3): Order as Single-line annotation,
                                    // Multiline tail, and Multiline head. We assign an integer
                                    // value to each type of annotation for sorting purposes.
                                    auto const compute_type_value =
                                        [](Annotation::AnnotationType annotation_type) {
                                            switch (annotation_type) {
                                            case Annotation::SingleLine:
                                                return 0;
                                            case Annotation::MultilineTail:
                                                return 1;
                                            case Annotation::MultilineHead:
                                                return 2;
                                            default:
                                                detail::unreachable();
                                            }
                                        };

                                    return compute_type_value(self.annotation->type)
                                        < compute_type_value(other.annotation->type);
                                } else {
                                    switch (self.annotation->type) {
                                    case Annotation::SingleLine: {
                                        auto const [self_underline_beg, self_underline_end] =
                                            self.annotation->underline_display_range();
                                        auto const [other_underline_beg, other_underline_end] =
                                            other.annotation->underline_display_range();
                                        // Condition (4): For single-line annotations, the one with
                                        // the shorter underline is placed above.
                                        return self_underline_end - self_underline_beg
                                            < other_underline_end - other_underline_beg;
                                    }
                                    case Annotation::MultilineHead:
                                        // Condition (5): For the heads of multiline annotations,
                                        // the one with the smaller `depth` is placed above.
                                        return self.annotation->col_beg.byte
                                            < other.annotation->col_beg.byte;
                                    case Annotation::MultilineTail:
                                        // Condition (6): For the tails of multiline annotations,
                                        // the one with the greater `depth` is placed above.
                                        return other.annotation->col_beg.byte
                                            < self.annotation->col_beg.byte;
                                    default:
                                        detail::unreachable();
                                    };
                                }
                            }();

                            if (should_add_edge) {
                                add_edge(self, other, /*weight=*/0);
                            }
                        }
                    }
                }

                // At this point, we have built part of the directed graph according to Rules 1 and
                // 2, and constructed the DSU. Now we check Rule 3, and with the help of the DSU,
                // determine if Rule 3 can be applied.
                for (Vertex& self : vertices_) {
                    unsigned const self_beg =
                        std::get<0>(self.annotation->label_display_range(label_position_));

                    for (Vertex& other : vertices_) {
                        // Rule 3: If b2 < a1 and A is a multiline annotation, then A's horizontal
                        // connection line should be after B's label, and A's label further a line
                        // after its horizontal connection line, so an additional 1 is needed.
                        //
                        // We need to ensure that `self` not only does not overlap with `other`, but
                        // also does not overlap with any annotations in the same set as `other` to
                        // prevent the creation of cyclic dependencies.
                        if (self.is_multiline() && other.query_rightmost() < self_beg) {
                            add_edge(other, self, /*weight=*/1);
                        }
                    }
                }
            }
        };

        AnnotationGraph const annotation_graph(annotations, label_position, first_line_height);
        // Assign label positions to all annotations that cannot be rendered inline. Since
        // `AnnotationGraph` does not own the `Annotation` objects, the results are directly written
        // back to the original `Annotation` objects.
        annotation_graph.assign_label_line_positions();

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
