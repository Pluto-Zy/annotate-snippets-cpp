#ifndef ANNOTATE_SNIPPETS_RENDERER_HUMAN_RENDERER_HPP
#define ANNOTATE_SNIPPETS_RENDERER_HUMAN_RENDERER_HPP

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/diag/diag_entry_impl.hpp"
#include "annotate_snippets/detail/diag/level.hpp"
#include "annotate_snippets/diag.hpp"
#include "annotate_snippets/styled_string.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <ranges>
#include <string_view>

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

    template <class Level>
    auto render_diag(Diag<Level> diag) const -> StyledString {
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

    template <class Level, class Derived>
    void render_diag_entry(
        StyledString& render_target,
        detail::DiagEntryImpl<Level, Derived>& diag_entry,
        unsigned max_line_num_len,
        bool is_secondary
    ) const {
        // If all associated source codes of the current diagnostic entry have no annotations, or if
        // it is not associated with any source code (if `diag_entry.associated_source()` is empty,
        // then `std::ranges::any_of` returns `false`), then the current diagnostic entry does not
        // need to render annotations.
        bool const has_annotation =
            std::ranges::any_of(diag_entry.associated_sources(), [](AnnotatedSource const& source) {
                return !source.primary_spans().empty() || !source.secondary_spans().empty();
            });

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
            detail::level_display_string(diag_entry.level()),
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

private:
    /// Calculates the maximum space required to display all annotated line numbers contained in
    /// `source`.
    auto compute_max_line_num_len(AnnotatedSource const& source) const -> unsigned;

    /// Calculates the maximum space required to display the line numbers for rendering `diag`.
    template <class Level>
    auto compute_max_line_num_len(Diag<Level> const& diag) const -> unsigned {
        if (ui_testing) {
            return anonymized_line_num.size();
        }

        auto const source_trans = [&](AnnotatedSource const& source) {
            return compute_max_line_num_len(source);
        };

        unsigned const primary = [&] {
            if (diag.primary_diag_entry().associated_sources().empty()) {
                return 0u;
            } else {
                return std::ranges::max(
                    diag.primary_diag_entry().associated_sources()
                    | std::views::transform(source_trans)
                );
            }
        }();

        unsigned const secondary = [&] {
            if (std::ranges::all_of(
                    diag.secondary_diag_entries(),
                    [](DiagEntry<Level> const& entry) { return entry.associated_sources().empty(); }
                )) {
                return 0u;
            } else {
                return std::ranges::max(
                    diag.secondary_diag_entries()
                    | std::views::transform([](DiagEntry<Level> const& entry) -> auto& {
                          return entry.associated_sources();
                      })
                    | std::views::join | std::views::transform(source_trans)
                );
            }
        }();

        return std::ranges::max(primary, secondary);
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
