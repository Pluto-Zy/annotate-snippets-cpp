#include "annotate_snippets/renderer/human_renderer.hpp"

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "annotate_snippets/detail/unicode_display_width.hpp"
#include "annotate_snippets/style.hpp"
#include "annotate_snippets/styled_string.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
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

        return result;
    };

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
            if (cur_line_no - prev_line_no > max_unannotated_line_num) {
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
                    annotated_line.source_line.substr(chunk_begin, byte),
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

    AnnotatedLines const annotated_lines = AnnotatedLines::from_source(source, renderer);
}
}  // namespace

void HumanRenderer::render_annotated_sources(
    StyledString& render_target,
    std::vector<AnnotatedSource>& sources,
    unsigned max_line_num_len
) const {
    // clang-format off
    for (unsigned source_idx = 0;
         AnnotatedSource& source : sources | std::views::filter([](AnnotatedSource const& source) {
             return !source.primary_spans().empty() || !source.secondary_spans().empty();
        }))
    // clang-format on
    {
        render_target.append_newline();
        render_file_line_col(render_target, source, max_line_num_len, source_idx == 0);

        ++source_idx;
    }
}
}  // namespace ants
