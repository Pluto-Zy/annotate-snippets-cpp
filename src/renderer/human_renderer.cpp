#include "annotate_snippets/renderer/human_renderer.hpp"

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "annotate_snippets/style.hpp"
#include "annotate_snippets/styled_string.hpp"
#include "annotate_snippets/styled_string_view.hpp"
#include "unicode/display_width.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
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
    indentation += unicode::display_width(std::string(level_title));

    // If there is an error code, render it.
    if (!err_code.empty()) {
        std::string rendered_err_code = "[";
        rendered_err_code.append(err_code);
        rendered_err_code.push_back(']');

        render_target.append(rendered_err_code, title_style);
        indentation += unicode::display_width(rendered_err_code);
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
        final_width =
            unicode::display_width(std::string(source.origin())) + line.size() + col.size() + 4;
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
