#ifndef ANNOTATE_SNIPPETS_RENDERER_HUMAN_RENDERER_HPP
#define ANNOTATE_SNIPPETS_RENDERER_HUMAN_RENDERER_HPP

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/diag/diag_entry_impl.hpp"
#include "annotate_snippets/detail/diag/level.hpp"
#include "annotate_snippets/detail/styled_string_impl.hpp"
#include "annotate_snippets/diag.hpp"
#include "annotate_snippets/style.hpp"
#include "annotate_snippets/style_spec.hpp"
#include "annotate_snippets/styled_string.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ants {
class HumanRenderer {
public:
    /// Default column width, used in tests and when terminal dimensions cannot be determined.
    static constexpr unsigned default_column_width = 140;

    /// Maximum width for diagnostic messages. When code lines exceed this width, the renderer will
    /// attempt to reduce the lines to fit within this constraint.
    unsigned diagnostic_width = default_column_width;
    /// Character used for primary annotations underline. This character's display width must be 1.
    char primary_underline = '^';
    /// Character used for secondary annotations underline. This character's display width must
    /// be 1.
    char secondary_underline = '-';
    /// Indicates whether to render a simplified diagnostic message.
    bool short_message = false;
    /// Indicates whether to render anonymized line numbers.
    bool ui_testing = false;
    /// Content displayed at the line number position when rendering anonymized line numbers.
    std::string_view anonymized_line_num = "LL";
    /// Represents the number of spaces that a '\t' should be replaced with when rendering *source
    /// code* (rather than labels) on the screen. By specifying this value, we can make the tabs in
    /// the rendering appear more uniform.
    ///
    /// If set to 0, it means that tabs should not be replaced with spaces.
    std::uint8_t display_tab_width = 4;
    /// The maximum number of unannotated lines allowed. If the number of unannotated lines between
    /// two annotated lines exceeds this value, all such lines are collectively replaced by an
    /// ellipsis line (represented by "..."). Otherwise, all these unannotated lines will be fully
    /// rendered.
    ///
    /// If this value is set to 0, it means that no unannotated lines are allowed.
    std::uint8_t max_unannotated_line_num = 2;
    /// The maximum number of lines allowed for a multi-line annotation. If the number of lines
    /// covered by a multi-line annotation exceeds this value, the lines exceeding this count from
    /// the middle onwards are collectively replaced by an ellipsis line (represented by "...").
    /// Otherwise, the multi-line annotation will be fully rendered.
    ///
    /// Note that since multiple multi-line annotations may overlap and intersect, it is necessary
    /// to ensure all multi-line annotations are correctly rendered before attempting to omit any
    /// lines to meet this value. For example, when multiple multi-line annotations are nested, the
    /// outermost annotations cannot be omitted because it is necessary for the inner annotations to
    /// be rendered.
    ///
    /// Valid values for this parameter must *not* be less than 2, as at least two lines need to be
    /// rendered. If a value less than 2 is specified, all multi-line annotations will be fully
    /// rendered.
    std::uint8_t max_multiline_annotation_line_num = 4;
    /// Rendering position of single-line annotation labels.
    ///
    /// Typically, we aim to render the label and the underline on the same line to minimize the
    /// vertical length of diagnostic messages. For example:
    ///
    ///     foo(abc + def)
    ///         ^^^ label   <-- on the same line as the underline.
    ///
    /// However, sometimes we cannot render them on the same line, as shown below:
    ///
    ///     foo(abc + def)
    ///         ^^^   ^^^ label   <-- this label can be on the same line as the underline.
    ///         |
    ///         label   <-- This label cannot be on the same line, otherwise it would obscure
    ///                     subsequent information.
    ///
    /// This member controls the placement of the label when such situations occur.
    ///
    /// Note:
    /// 1. Regardless of what value `label_position` is set to, as long as the label can be rendered
    ///    on the same line as the underline, we will do so. In this case, `label_position` has no
    ///    effect.
    /// 2. This member does not affect *multi-line annotations*: if a label cannot be rendered on
    ///    the same line as the end of a multi-line annotation, it will always be rendered at the
    ///    end of the multi-line annotation's tail, similar to the `Right` effect.
    enum LabelPosition : std::uint8_t {
        /// Indicates that the label should be rendered at the far left of the annotated range, for
        /// example:
        ///
        ///     foo(variable + def)
        ///         ^^^^^^^^   ^^^
        ///         |
        ///         This label is rendered at the far left of the annotated "variable" word.
        Left,
        /// Indicates that the label should be rendered at the far right of the annotated range, for
        /// example:
        ///
        ///     foo(variable + def)
        ///         ^^^^^^^^   ^^^
        ///                |
        ///                This label is rendered at the far right of the annotated "variable" word.
        Right,
    } label_position = Left;
    /// Represents the alignment of line numbers.
    enum LineNumAlignment : std::uint8_t {
        /// Aligns line numbers to the left.
        ///
        /// For example:
        ///
        ///     1   | foo(abc + def)
        ///     ...
        ///     100 | bar(abc + def)
        AlignLeft,
        /// Aligns line numbers to the right.
        ///
        /// For example:
        ///
        ///       1 | foo(abc + def)
        ///     ...
        ///     100 | bar(abc + def)
        AlignRight,
    } line_num_alignment = AlignRight;
    /// Represents whether to render the line numbers before or after applying the patch.
    ///
    /// When a patch is applied, it may change the line numbers of subsequent patches. For example,
    /// consider the following code:
    ///
    ///     int main() {
    ///         Type var = 3;
    ///         Type var2 = val;
    ///     }
    ///
    /// Suppose the user wants to add two patches to the above code: the first patch adds a line
    /// `using Type = int;` before the second line, and the second patch modifies `val` to `var` in
    /// the third line. The first patch introduces a new line, which causes the line numbers of the
    /// second patch to change. The explicit line numbers before and after applying the patches are
    /// as follows:
    ///
    ///     0 0 | int main() {
    ///       1 +     using Type = int;
    ///     1 2 |     Type var = 3;
    ///     2 3 ~     Type var2 = var;
    ///     3 4 | }
    ///
    /// When rendering the second patch in `Inline` style, we only render the modified content and
    /// underline the modified part:
    ///
    ///     3 ~     Type var2 = var;
    ///       |
    ///
    /// This option controls which line numbers to display in such cases. If `BeforePatch` is used,
    /// the line number before applying the patch (2 in this case) will be displayed; otherwise, the
    /// line number after applying the patch (3 in this case) will be displayed.
    ///
    /// Note that this option only affects the rendering of line numbers in `Inline` style and for
    /// lines that have not been modified. For `Diff` style rendering, the deleted lines always use
    /// the line numbers before the patch, and the added lines always use the line numbers after the
    /// patch.
    enum PatchLineNumMode : std::uint8_t {
        /// Renders the line numbers before applying the patch.
        BeforePatch,
        /// Renders the line numbers after applying the patch.
        AfterPatch,
    } line_num_patch_mode = AfterPatch;
    /// Represents the maximum length of a single-line replacement patch that will be rendered as
    /// `Inline` style. If the length of the replacement string exceeds this value, the patch will
    /// be rendered as `Diff` style, otherwise it will be rendered as `Inline` style.
    ///
    /// This parameter only applies to single-line replacement patches, i.e., patches that satisfy
    /// the following three conditions:
    /// 1. `patch.is_replacement()` returns `true`;
    /// 2. `patch.location_begin().line == patch.location_end().line`;
    /// 3. `patch.replacement()` does not contain any newline characters.
    ///
    /// The length `l` is defined as `max(patch.replacement().size(), patch.location_end().col -
    /// patch.location_begin().col)`. If `l` is greater than this value, the patch will be rendered
    /// as `Diff` style, otherwise it will be rendered as `Inline` style.
    unsigned max_inline_style_single_line_replacement_length = 5;
    /// Represents the character used to form the underline for an addition patch when rendered in
    /// `Inline` style. When rendered in `Diff` style, this character is used as the line number
    /// separator for added lines. This character's display width must be 1.
    char addition_marker = '+';
    /// Represents the character used to form the underline for a deletion patch when rendered in
    /// `Inline` style. When rendered in `Diff` style, this character is used as the line number
    /// separator for deleted lines. This character's display width must be 1.
    char deletion_marker = '-';
    /// Represents the character used to form the underline for a replacement patch when rendered in
    /// `Inline` style. This character's display width must be 1.
    char replacement_marker = '~';

    /// Renders `diag` to a `StyledString` and returns the rendering result.
    template <class Level>
    auto render_diag(Diag<Level> diag) const -> StyledString {
        // Sort all patches associated with the sources in `diag`. This is necessary because when we
        // compute the maximum width of line numbers, we need to rely on the sorted order of patches
        // to determine the maximum line number that may be involved.
        sort_patches(diag);

        StyledString render_target;
        unsigned const max_line_num_len = compute_max_line_num_len(diag);

        // Render the primary diagnostic entry.
        render_diag_entry(
            render_target,
            diag.primary_diag_entry(),
            max_line_num_len,
            /*is_secondary=*/false
        );

        // Render all secondary diagnostic entries.
        for (DiagEntry<Level>& entry : diag.secondary_diag_entries()) {
            render_target.append_newline();
            render_diag_entry(render_target, entry, max_line_num_len, /*is_secondary=*/true);
        }

        return render_target;
    }

    /// Renders `diag` to the output stream associated with `out`. The rendering style is specified
    /// by `style_sheet`.
    template <
        class Level,
        class StyleSheet = PlainTextStyleSheet,
        std::enable_if_t<is_style_sheet<StyleSheet, Level>, int> = 0>
    void render_diag(std::ostream& out, Diag<Level> diag, StyleSheet style_sheet = {}) const {
        // Sort all patches associated with the sources in `diag`. This is necessary because when we
        // compute the maximum width of line numbers, we need to rely on the sorted order of patches
        // to determine the maximum line number that may be involved.
        sort_patches(diag);

        unsigned const max_line_num_len = compute_max_line_num_len(diag);

        // Render the primary diagnostic entry.
        render_diag_entry(
            out,
            diag.primary_diag_entry(),
            max_line_num_len,
            /*is_secondary=*/false,
            style_sheet
        );

        // Render all secondary diagnostic entries.
        for (DiagEntry<Level>& entry : diag.secondary_diag_entries()) {
            render_diag_entry(out, entry, max_line_num_len, /*is_secondary=*/true, style_sheet);
        }
    }

    /// Appends the rendering of a single `DiagEntry` to the end of a `StyledString`.
    template <class Level, class Derived>
    void render_diag_entry(
        StyledString& render_target,
        detail::DiagEntryImpl<Level, Derived>& diag_entry,
        unsigned max_line_num_len,
        bool is_secondary
    ) const {
        // If all associated source codes of the current diagnostic entry have no annotations and
        // patches, or if it is not associated with any source code (if
        // `diag_entry.associated_source()` is empty, then `std::any_of` returns `false`), then the
        // current diagnostic entry does not need to render annotations.
        bool const has_annotation = std::any_of(
            diag_entry.associated_sources().begin(),
            diag_entry.associated_sources().end(),
            [](AnnotatedSource const& source) {
                return !source.primary_spans().empty() || !source.secondary_spans().empty()
                    || !source.patches().empty();
            }
        );

        // Indentation for the title message. Usually, this indentation is 0. When `short_message`
        // is `true`, since we need to render the file name and line/column numbers before the title
        // message, the actual indentation is not 0.
        unsigned title_message_indentation = 0;

        // For short messages, we first need to render the file name and line/column numbers of the
        // diagnostic information.
        if (short_message) {
            title_message_indentation =
                render_file_line_col_short_message(render_target, diag_entry.associated_sources());
        }

        render_title_message(
            render_target,
            detail::level_title(diag_entry.level()),
            diag_entry.error_code(),
            diag_entry.diag_message(),
            max_line_num_len,
            title_message_indentation,
            is_secondary,
            // If the current diagnostic entry has no associated annotations, it can be attached
            // after the previous diagnostic entry.
            !has_annotation
        );

        // For short messages, we have now completed the rendering.
        if (short_message) {
            return;
        }

        render_annotated_sources(render_target, diag_entry.associated_sources(), max_line_num_len);
    }

    /// Renders a single `DiagEntry` to the output stream associated with `out`. The rendering style
    /// is specified by `style_sheet`.
    template <
        class Level,
        class Derived,
        class StyleSheet,
        std::enable_if_t<is_style_sheet<StyleSheet, Level>, int> = 0>
    void render_diag_entry(
        std::ostream& out,
        detail::DiagEntryImpl<Level, Derived>& diag_entry,
        unsigned max_line_num_len,
        bool is_secondary,
        StyleSheet const& style_sheet
    ) const {
        StyledString render_target;
        // Render the diagnostic entry to `render_target`.
        render_diag_entry(render_target, diag_entry, max_line_num_len, is_secondary);

        // Render the styled string to the output stream.
        for (std::vector<StyledStringViewPart> const& line : render_target.styled_line_parts()) {
            for (StyledStringViewPart const& part : line) {
                // The style used to render `part`. For `Style::Default`, the default style is
                // always used.
                StyleSpec const spec = part.style == Style::Default
                    ? StyleSpec()
                    : static_cast<StyleSpec>(
                          std::invoke(style_sheet, part.style, diag_entry.level())
                      );

                spec.render_string(out, part.content);
            }

            out << '\n';
        }
    }

private:
    /// Sorts all patches associated with the sources in `diag`. Patches are sorted by their
    /// starting position (`location_begin()`). This ensures that we can apply the patches in sorted
    /// order to get the correct modified result.
    template <class Level>
    void sort_patches(Diag<Level>& diag) const {
        for (AnnotatedSource& source : diag.primary_diag_entry().associated_sources()) {
            std::sort(
                source.patches().begin(),
                source.patches().end(),
                [](Patch const& lhs, Patch const& rhs) {
                    return lhs.location_begin() < rhs.location_begin();
                }
            );
        }

        for (DiagEntry<Level>& entry : diag.secondary_diag_entries()) {
            for (AnnotatedSource& source : entry.associated_sources()) {
                std::sort(
                    source.patches().begin(),
                    source.patches().end(),
                    [](Patch const& lhs, Patch const& rhs) {
                        return lhs.location_begin() < rhs.location_begin();
                    }
                );
            }
        }
    }

    /// Calculates the maximum space required to display all annotated line numbers contained in
    /// `source`.
    auto compute_max_line_num_len(AnnotatedSource& source) const -> unsigned;

    /// Calculates the maximum space required to display the line numbers for rendering `diag`.
    template <class Level>
    auto compute_max_line_num_len(Diag<Level>& diag) const -> unsigned {
        if (ui_testing) {
            return static_cast<unsigned>(anonymized_line_num.size());
        }

        auto const source_trans = [&](AnnotatedSource& source) {
            return compute_max_line_num_len(source);
        };

        unsigned result = 0;

        // Primary diag entry.
        for (AnnotatedSource& source : diag.primary_diag_entry().associated_sources()) {
            result = std::max(result, compute_max_line_num_len(source));
        }

        // Secondary diag entries.
        for (DiagEntry<Level>& entry : diag.secondary_diag_entries()) {
            for (AnnotatedSource& source : entry.associated_sources()) {
                result = std::max(result, compute_max_line_num_len(source));
            }
        }

        return result;
    }

    /// Renders the title message into `render_target`. For example:
    ///
    ///     error[E0001]: error message.
    ///
    /// Here, `level_title` is "error", `err_code` is "E0001", and `message` is "error message.".
    /// The entire title message will have an indentation of `indentation`.
    ///
    /// `is_secondary` is used to indicate whether the current diagnostic entry is a secondary
    /// diagnostic entry, as secondary diagnostics have a different rendering style and format.
    /// `is_attached` indicates whether the current diagnostic item can be attached to the previous
    /// one, such as "note: " messages. Typically, only secondary diagnostics without annotations
    /// are rendered in the attached format.
    void render_title_message(
        StyledString& render_target,
        std::string_view level_title,
        std::string_view err_code,
        StyledStringView const& message,
        unsigned max_line_num_len,
        unsigned indentation,
        bool is_secondary,
        bool is_attached
    ) const;

    /// Renders the file name, line number, and column number triple in short message mode. `source`
    /// represents all the source codes associated with the current diagnostic entry. This method
    /// selects sources where `primary_spans()` is not empty, rendering them line by line in the
    /// format "filename:line:column: ". Returns the text display width of the last line.
    static auto render_file_line_col_short_message(
        StyledString& render_target,
        std::vector<AnnotatedSource> const& sources
    ) -> unsigned;

    void render_annotated_sources(
        StyledString& render_target,
        std::vector<AnnotatedSource>& sources,
        unsigned max_line_num_len
    ) const;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_RENDERER_HUMAN_RENDERER_HPP
