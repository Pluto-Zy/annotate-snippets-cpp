#ifndef ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP
#define ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP

#include "annotate_snippets/patch.hpp"
#include "annotate_snippets/source_file.hpp"
#include "annotate_snippets/source_location.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
/// Represents source code with some annotations and fixes.
///
/// Note that `AnnotatedSource` assumes that once constructed, the code it refers to will not be
/// changed, since `AnnotatedSource` only stores the relative location of annotations and does not
/// own the code.
class AnnotatedSource : public SourceFile {
public:
    using SourceFile::SourceFile;

    /// Creates an `AnnotatedSource` object that is not associated with any code snippet.
    AnnotatedSource() = default;
    /// Creates an `AnnotatedSource` object that is associated with code snippet `source`.
    AnnotatedSource(SourceFile source) : SourceFile(std::move(source)) { }

    auto with_name(std::string_view name) & -> AnnotatedSource& {
        set_name(name);
        return *this;
    }

    auto with_name(std::string_view name) && -> AnnotatedSource&& {
        set_name(name);
        return std::move(*this);
    }

    auto with_first_line_number(unsigned number) & -> AnnotatedSource& {
        set_first_line_number(number);
        return *this;
    }

    auto with_first_line_number(unsigned number) && -> AnnotatedSource&& {
        set_first_line_number(number);
        return std::move(*this);
    }

    auto primary_spans() const -> std::vector<LabeledSpan> const& {
        return primary_spans_;
    }

    auto primary_spans() -> std::vector<LabeledSpan>& {
        return primary_spans_;
    }

    auto secondary_spans() const -> std::vector<LabeledSpan> const& {
        return secondary_spans_;
    }

    auto secondary_spans() -> std::vector<LabeledSpan>& {
        return secondary_spans_;
    }

    void add_primary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) {
        primary_spans_.push_back(LabeledSpan {
            /*beg=*/beg,
            /*end=*/end,
            /*label=*/std::move(label),
        });
    }

    auto with_primary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_primary_labeled_annotation(beg, end, std::move(label));
        return *this;
    }

    auto with_primary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_primary_labeled_annotation(beg, end, std::move(label));
        return std::move(*this);
    }

    void add_primary_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) {
        add_primary_labeled_annotation(
            SourceLocation::from_byte_offset(byte_beg),
            SourceLocation::from_byte_offset(byte_end),
            std::move(label)
        );
    }

    auto with_primary_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_primary_labeled_annotation(byte_beg, byte_end, std::move(label));
        return *this;
    }

    auto with_primary_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_primary_labeled_annotation(byte_beg, byte_end, std::move(label));
        return std::move(*this);
    }

    void add_primary_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) {
        add_primary_labeled_annotation(
            SourceLocation::from_line_col(line_beg, col_beg),
            SourceLocation::from_line_col(line_end, col_end),
            std::move(label)
        );
    }

    auto with_primary_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_primary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return *this;
    }

    auto with_primary_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_primary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return std::move(*this);
    }

    void add_primary_annotation(SourceLocation beg, SourceLocation end, StyledStringView label) {
        add_primary_labeled_annotation(beg, end, std::move(label));
    }

    auto with_primary_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_primary_annotation(beg, end, std::move(label));
        return *this;
    }

    auto with_primary_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_primary_annotation(beg, end, std::move(label));
        return std::move(*this);
    }

    void add_primary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) {
        add_primary_labeled_annotation(byte_beg, byte_end, std::move(label));
    }

    auto with_primary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_primary_annotation(byte_beg, byte_end, std::move(label));
        return *this;
    }

    auto with_primary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_primary_annotation(byte_beg, byte_end, std::move(label));
        return std::move(*this);
    }

    void add_primary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) {
        add_primary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
    }

    auto with_primary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_primary_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return *this;
    }

    auto with_primary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_primary_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return std::move(*this);
    }

    void add_primary_annotation(SourceLocation beg, SourceLocation end) {
        add_primary_labeled_annotation(beg, end, StyledStringView());
    }

    auto with_primary_annotation(SourceLocation beg, SourceLocation end) & -> AnnotatedSource& {
        add_primary_annotation(beg, end);
        return *this;
    }

    auto with_primary_annotation(SourceLocation beg, SourceLocation end) && -> AnnotatedSource&& {
        add_primary_annotation(beg, end);
        return std::move(*this);
    }

    void add_primary_annotation(std::size_t byte_beg, std::size_t byte_end) {
        add_primary_labeled_annotation(byte_beg, byte_end, StyledStringView());
    }

    auto with_primary_annotation(std::size_t byte_beg, std::size_t byte_end) & -> AnnotatedSource& {
        add_primary_annotation(byte_beg, byte_end);
        return *this;
    }

    auto with_primary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end
    ) && -> AnnotatedSource&& {
        add_primary_annotation(byte_beg, byte_end);
        return std::move(*this);
    }

    void add_primary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) {
        add_primary_labeled_annotation(line_beg, col_beg, line_end, col_end, StyledStringView());
    }

    auto with_primary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) & -> AnnotatedSource& {
        add_primary_annotation(line_beg, col_beg, line_end, col_end);
        return *this;
    }

    auto with_primary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) && -> AnnotatedSource&& {
        add_primary_annotation(line_beg, col_beg, line_end, col_end);
        return std::move(*this);
    }

    void add_secondary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) {
        secondary_spans_.push_back(LabeledSpan {
            /*beg=*/beg,
            /*end=*/end,
            /*label=*/std::move(label),
        });
    }

    auto with_secondary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_secondary_labeled_annotation(beg, end, std::move(label));
        return *this;
    }

    auto with_secondary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_secondary_labeled_annotation(beg, end, std::move(label));
        return std::move(*this);
    }

    void add_secondary_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) {
        add_secondary_labeled_annotation(
            SourceLocation::from_byte_offset(byte_beg),
            SourceLocation::from_byte_offset(byte_end),
            std::move(label)
        );
    }

    auto with_secondary_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_secondary_labeled_annotation(byte_beg, byte_end, std::move(label));
        return *this;
    }

    auto with_secondary_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_secondary_labeled_annotation(byte_beg, byte_end, std::move(label));
        return std::move(*this);
    }

    void add_secondary_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) {
        add_secondary_labeled_annotation(
            SourceLocation::from_line_col(line_beg, col_beg),
            SourceLocation::from_line_col(line_end, col_end),
            std::move(label)
        );
    }

    auto with_secondary_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_secondary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return *this;
    }

    auto with_secondary_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_secondary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return std::move(*this);
    }

    void add_secondary_annotation(SourceLocation beg, SourceLocation end, StyledStringView label) {
        add_secondary_labeled_annotation(beg, end, std::move(label));
    }

    auto with_secondary_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_secondary_annotation(beg, end, std::move(label));
        return *this;
    }

    auto with_secondary_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_secondary_annotation(beg, end, std::move(label));
        return std::move(*this);
    }

    void add_secondary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) {
        add_secondary_labeled_annotation(byte_beg, byte_end, std::move(label));
    }

    auto with_secondary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_secondary_annotation(byte_beg, byte_end, std::move(label));
        return *this;
    }

    auto with_secondary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_secondary_annotation(byte_beg, byte_end, std::move(label));
        return std::move(*this);
    }

    void add_secondary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) {
        add_secondary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
    }

    auto with_secondary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_secondary_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return *this;
    }

    auto with_secondary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_secondary_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return std::move(*this);
    }

    void add_secondary_annotation(SourceLocation beg, SourceLocation end) {
        add_secondary_labeled_annotation(beg, end, StyledStringView());
    }

    auto with_secondary_annotation(SourceLocation beg, SourceLocation end) & -> AnnotatedSource& {
        add_secondary_annotation(beg, end);
        return *this;
    }

    auto with_secondary_annotation(SourceLocation beg, SourceLocation end) && -> AnnotatedSource&& {
        add_secondary_annotation(beg, end);
        return std::move(*this);
    }

    void add_secondary_annotation(std::size_t byte_beg, std::size_t byte_end) {
        add_secondary_labeled_annotation(byte_beg, byte_end, StyledStringView());
    }

    auto with_secondary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end
    ) & -> AnnotatedSource& {
        add_secondary_annotation(byte_beg, byte_end);
        return *this;
    }

    auto with_secondary_annotation(
        std::size_t byte_beg,
        std::size_t byte_end
    ) && -> AnnotatedSource&& {
        add_secondary_annotation(byte_beg, byte_end);
        return std::move(*this);
    }

    void add_secondary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) {
        add_secondary_labeled_annotation(line_beg, col_beg, line_end, col_end, StyledStringView());
    }

    auto with_secondary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) & -> AnnotatedSource& {
        add_secondary_annotation(line_beg, col_beg, line_end, col_end);
        return *this;
    }

    auto with_secondary_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) && -> AnnotatedSource&& {
        add_secondary_annotation(line_beg, col_beg, line_end, col_end);
        return std::move(*this);
    }

    void add_labeled_annotation(SourceLocation beg, SourceLocation end, StyledStringView label) {
        add_primary_labeled_annotation(beg, end, std::move(label));
    }

    auto with_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_labeled_annotation(beg, end, std::move(label));
        return *this;
    }

    auto with_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_labeled_annotation(beg, end, std::move(label));
        return std::move(*this);
    }

    void add_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) {
        add_primary_labeled_annotation(byte_beg, byte_end, std::move(label));
    }

    auto with_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_labeled_annotation(byte_beg, byte_end, std::move(label));
        return *this;
    }

    auto with_labeled_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_labeled_annotation(byte_beg, byte_end, std::move(label));
        return std::move(*this);
    }

    void add_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) {
        add_primary_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
    }

    auto with_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return *this;
    }

    auto with_labeled_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_labeled_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return std::move(*this);
    }

    void add_annotation(SourceLocation beg, SourceLocation end, StyledStringView label) {
        add_primary_annotation(beg, end, std::move(label));
    }

    auto with_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_annotation(beg, end, std::move(label));
        return *this;
    }

    auto with_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_annotation(beg, end, std::move(label));
        return std::move(*this);
    }

    void add_annotation(std::size_t byte_beg, std::size_t byte_end, StyledStringView label) {
        add_primary_annotation(byte_beg, byte_end, std::move(label));
    }

    auto with_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_annotation(byte_beg, byte_end, std::move(label));
        return *this;
    }

    auto with_annotation(
        std::size_t byte_beg,
        std::size_t byte_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_annotation(byte_beg, byte_end, std::move(label));
        return std::move(*this);
    }

    void add_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) {
        add_primary_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
    }

    auto with_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) & -> AnnotatedSource& {
        add_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return *this;
    }

    auto with_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        StyledStringView label
    ) && -> AnnotatedSource&& {
        add_annotation(line_beg, col_beg, line_end, col_end, std::move(label));
        return std::move(*this);
    }

    void add_annotation(SourceLocation beg, SourceLocation end) {
        add_primary_annotation(beg, end);
    }

    auto with_annotation(SourceLocation beg, SourceLocation end) & -> AnnotatedSource& {
        add_annotation(beg, end);
        return *this;
    }

    auto with_annotation(SourceLocation beg, SourceLocation end) && -> AnnotatedSource&& {
        add_annotation(beg, end);
        return std::move(*this);
    }

    void add_annotation(std::size_t byte_beg, std::size_t byte_end) {
        add_primary_annotation(byte_beg, byte_end);
    }

    auto with_annotation(std::size_t byte_beg, std::size_t byte_end) & -> AnnotatedSource& {
        add_annotation(byte_beg, byte_end);
        return *this;
    }

    auto with_annotation(std::size_t byte_beg, std::size_t byte_end) && -> AnnotatedSource&& {
        add_annotation(byte_beg, byte_end);
        return std::move(*this);
    }

    void add_annotation(unsigned line_beg, unsigned col_beg, unsigned line_end, unsigned col_end) {
        add_primary_annotation(line_beg, col_beg, line_end, col_end);
    }

    auto with_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) & -> AnnotatedSource& {
        add_annotation(line_beg, col_beg, line_end, col_end);
        return *this;
    }

    auto with_annotation(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) && -> AnnotatedSource&& {
        add_annotation(line_beg, col_beg, line_end, col_end);
        return std::move(*this);
    }

    auto patches() const -> std::vector<Patch> const& {
        return patches_;
    }

    auto patches() -> std::vector<Patch>& {
        return patches_;
    }

    void add_patch(Patch patch) {
        patches_.push_back(patch);
    }

    auto with_patch(Patch patch) & -> AnnotatedSource& {
        add_patch(patch);
        return *this;
    }

    auto with_patch(Patch patch) && -> AnnotatedSource&& {
        add_patch(patch);
        return std::move(*this);
    }

    void add_patch(SourceLocation beg, SourceLocation end, std::string_view replacement) {
        patches_.emplace_back(beg, end, replacement);
    }

    auto with_patch(
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_patch(beg, end, replacement);
        return *this;
    }

    auto with_patch(
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_patch(beg, end, replacement);
        return std::move(*this);
    }

    void add_patch(std::size_t byte_beg, std::size_t byte_end, std::string_view replacement) {
        patches_.emplace_back(byte_beg, byte_end, replacement);
    }

    auto with_patch(
        std::size_t byte_beg,
        std::size_t byte_end,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_patch(byte_beg, byte_end, replacement);
        return *this;
    }

    auto with_patch(
        std::size_t byte_beg,
        std::size_t byte_end,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_patch(byte_beg, byte_end, replacement);
        return std::move(*this);
    }

    void add_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) {
        patches_.emplace_back(line_beg, col_beg, line_end, col_end, replacement);
    }

    auto with_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_patch(line_beg, col_beg, line_end, col_end, replacement);
        return *this;
    }

    auto with_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_patch(line_beg, col_beg, line_end, col_end, replacement);
        return std::move(*this);
    }

    void add_addition_patch(SourceLocation loc, std::string_view replacement) {
        patches_.push_back(Patch::addition(loc, replacement));
    }

    auto with_addition_patch(
        SourceLocation loc,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_addition_patch(loc, replacement);
        return *this;
    }

    auto with_addition_patch(
        SourceLocation loc,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_addition_patch(loc, replacement);
        return std::move(*this);
    }

    void add_addition_patch(std::size_t byte_loc, std::string_view replacement) {
        patches_.push_back(Patch::addition(byte_loc, replacement));
    }

    auto with_addition_patch(
        std::size_t byte_loc,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_addition_patch(byte_loc, replacement);
        return *this;
    }

    auto with_addition_patch(
        std::size_t byte_loc,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_addition_patch(byte_loc, replacement);
        return std::move(*this);
    }

    void add_addition_patch(unsigned line, unsigned col, std::string_view replacement) {
        patches_.push_back(Patch::addition(line, col, replacement));
    }

    auto with_addition_patch(
        unsigned line,
        unsigned col,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_addition_patch(line, col, replacement);
        return *this;
    }

    auto with_addition_patch(
        unsigned line,
        unsigned col,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_addition_patch(line, col, replacement);
        return std::move(*this);
    }

    void add_deletion_patch(SourceLocation beg, SourceLocation end) {
        patches_.push_back(Patch::deletion(beg, end));
    }

    auto with_deletion_patch(SourceLocation beg, SourceLocation end) & -> AnnotatedSource& {
        add_deletion_patch(beg, end);
        return *this;
    }

    auto with_deletion_patch(SourceLocation beg, SourceLocation end) && -> AnnotatedSource&& {
        add_deletion_patch(beg, end);
        return std::move(*this);
    }

    void add_deletion_patch(std::size_t byte_beg, std::size_t byte_end) {
        patches_.push_back(Patch::deletion(byte_beg, byte_end));
    }

    auto with_deletion_patch(std::size_t byte_beg, std::size_t byte_end) & -> AnnotatedSource& {
        add_deletion_patch(byte_beg, byte_end);
        return *this;
    }

    auto with_deletion_patch(std::size_t byte_beg, std::size_t byte_end) && -> AnnotatedSource&& {
        add_deletion_patch(byte_beg, byte_end);
        return std::move(*this);
    }

    void add_deletion_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) {
        patches_.push_back(Patch::deletion(line_beg, col_beg, line_end, col_end));
    }

    auto with_deletion_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) & -> AnnotatedSource& {
        add_deletion_patch(line_beg, col_beg, line_end, col_end);
        return *this;
    }

    auto with_deletion_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end
    ) && -> AnnotatedSource&& {
        add_deletion_patch(line_beg, col_beg, line_end, col_end);
        return std::move(*this);
    }

    void add_replacement_patch(
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) {
        patches_.push_back(Patch::replacement(beg, end, replacement));
    }

    auto with_replacement_patch(
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_replacement_patch(beg, end, replacement);
        return *this;
    }

    auto with_replacement_patch(
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_replacement_patch(beg, end, replacement);
        return std::move(*this);
    }

    void add_replacement_patch(
        std::size_t byte_beg,
        std::size_t byte_end,
        std::string_view replacement
    ) {
        patches_.push_back(Patch::replacement(byte_beg, byte_end, replacement));
    }

    auto with_replacement_patch(
        std::size_t byte_beg,
        std::size_t byte_end,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_replacement_patch(byte_beg, byte_end, replacement);
        return *this;
    }

    auto with_replacement_patch(
        std::size_t byte_beg,
        std::size_t byte_end,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_replacement_patch(byte_beg, byte_end, replacement);
        return std::move(*this);
    }

    void add_replacement_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) {
        patches_.push_back(Patch::replacement(line_beg, col_beg, line_end, col_end, replacement));
    }

    auto with_replacement_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) & -> AnnotatedSource& {
        add_replacement_patch(line_beg, col_beg, line_end, col_end, replacement);
        return *this;
    }

    auto with_replacement_patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) && -> AnnotatedSource&& {
        add_replacement_patch(line_beg, col_beg, line_end, col_end, replacement);
        return std::move(*this);
    }

    /// Resolves all `SourceLocation` objects in the annotations and patches, filling in any missing
    /// location information. For example, if a `SourceLocation` only contains line and column
    /// information, the corresponding byte offset is calculated by looking up the source code
    /// content.
    ///
    /// Additionally, the function adjusts certain `SourceLocation` objects and spans, such as
    /// ensuring that non-empty spans cover at least one byte, and normalizing locations that exceed
    /// the bounds of the source code.
    ///
    /// The function utilizes the content and line cache provided by `SourceFile` to perform these
    /// operations. It builds the necessary line cache during execution. All `SourceFile` objects
    /// sharing the same source code can share and modify the same line cache, avoiding redundant
    /// computations.
    void resolve_and_normalize_locations();

private:
    /// A collection of primary spans, which are the locus of the error. They will be rendered with
    /// specific symbol (e.g. ^^^).
    std::vector<LabeledSpan> primary_spans_;
    /// A collection of secondary spans. They will be rendered with specific symbol (e.g. ---).
    std::vector<LabeledSpan> secondary_spans_;
    /// A collection of patches, which represent suggested modifications to the source code.
    std::vector<Patch> patches_;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP
