#ifndef ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP
#define ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP

#include "styled_string_view.hpp"

#include <concepts>
#include <cstddef>
#include <map>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
/// Represents the location of a *byte* in the source code.
// FIXME: unsigned or std::size_t?
struct SourceLocation {
    /// The (0-indexed) line number of the location.
    unsigned line;
    /// The (0-indexed) column number of the location.
    unsigned col;

    auto operator==(SourceLocation const& other) const -> bool = default;
};

/// Represents a single annotation span in the source code, with an optional label. When rendering
/// diagnostic information, a non-empty label will be rendered together with the corresponding span.
struct LabeledSpan {
    SourceLocation beg;
    SourceLocation end;
    /// The label attached to this span. If label.empty() is true, we consider the annotation to
    /// have no label attached.
    StyledStringView label;
};

/// Represents source code with some annotations.
///
/// Note that `AnnotatedSource` assumes that once constructed, the code it refers to will not be
/// changed, since `AnnotatedSource` only stores the relative location of annotations and does not
/// own the code.
class AnnotatedSource {
public:
    /// Creates an `AnnotatedSource` object that is not associated with any code snippet.
    AnnotatedSource() = default;
    /// Creates an `AnnotatedSource` object that is associated with code snippet `source`.
    explicit AnnotatedSource(std::string_view source) : source_(source) { }
    /// Create an `AnnotatedSource` object associated with the code snippet `source` and specify its
    /// origin as `origin`.
    explicit AnnotatedSource(std::string_view source, std::string_view origin) :
        source_(source), origin_(origin) { }

    auto source() const -> std::string_view {
        return source_;
    }

    auto origin() const -> std::string_view {
        return origin_;
    }

    void set_origin(std::string_view origin) {
        origin_ = origin;
    }

    auto with_origin(std::string_view origin) & -> AnnotatedSource& {
        set_origin(origin);
        return *this;
    }

    auto with_origin(std::string_view origin) && -> AnnotatedSource&& {
        set_origin(origin);
        return std::move(*this);
    }

    auto first_line_number() const -> unsigned {
        return first_line_number_;
    }

    void set_first_line_number(unsigned number) {
        first_line_number_ = number;
    }

    auto with_first_line_number(unsigned number) & -> AnnotatedSource& {
        set_first_line_number(number);
        return *this;
    }

    auto with_first_line_number(unsigned number) && -> AnnotatedSource&& {
        set_first_line_number(number);
        return std::move(*this);
    }

    auto line_offsets_cache() const -> auto& {
        return line_offsets_;
    }

    auto line_offsets_cache() -> auto& {
        return line_offsets_;
    }

    /// Returns the offset of the first byte of line `line`. If this information is already cached
    /// in `line_offsets_` then the cached result is returned, otherwise the result will be
    /// calculated in place and cached.
    auto line_offset(unsigned line) -> std::size_t;

    void set_line_offset(unsigned line, std::size_t offset) {
        line_offsets_[line] = offset;
    }

    template <std::ranges::input_range Range>
        requires std::convertible_to<
            std::ranges::range_reference_t<Range>,
            std::map<unsigned, std::size_t>::value_type>
    void set_line_offsets(Range&& r) {
#ifdef __cpp_lib_containers_ranges
        line_offsets_.insert_range(std::forward<Range>(r));
#else
        auto&& cr = r | std::views::common;
        line_offsets_.insert(std::ranges::begin(cr), std::ranges::end(cr));
#endif
    }

    /// Returns the line and column number of the byte at offset `byte_offset` in the source code.
    ///
    /// This method caches the position of the first character of the line where `byte_offset` is
    /// located into the cache `line_offsets_`.
    auto byte_offset_to_line_col(std::size_t byte_offset) -> SourceLocation;

    /// Returns the content of the line `line`. If the line does not exist, returns an empty string.
    ///
    /// Note that the returned string does not include the trailing newline character, whether it is
    /// '\n' or '\r\n'.
    auto line_content(unsigned line) -> std::string_view;

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
            .beg = beg,
            .end = end,
            .label = std::move(label),
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
            byte_offset_to_line_col(byte_beg),
            byte_offset_to_line_col(byte_end),
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

    void add_secondary_labeled_annotation(
        SourceLocation beg,
        SourceLocation end,
        StyledStringView label
    ) {
        secondary_spans_.push_back(LabeledSpan {
            .beg = beg,
            .end = end,
            .label = std::move(label),
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
            byte_offset_to_line_col(byte_beg),
            byte_offset_to_line_col(byte_end),
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

private:
    /// The source code to be annotated.
    std::string_view source_;
    /// The origin of the source code (the file name).
    std::string_view origin_;
    /// A collection of primary spans, which are the locus of the error. They will be rendered with
    /// specific symbol (e.g. ^^^).
    std::vector<LabeledSpan> primary_spans_;
    /// A collection of secondary spans. They will be rendered with specific symbol (e.g. ---).
    std::vector<LabeledSpan> secondary_spans_;
    /// Caches the offset of the first byte of each line in the entire source code (`source_`). It
    /// is used to quickly find a line of source code when rendering diagnostic information.
    ///
    /// There are several ways to modify the cache:
    ///     1. The user can explicitly specify the offset of the starting byte of a line by
    ///     `set_line_offset()` and `set_line_offsets()`, because this information is usually known
    ///     in other compilation stages, for example, the source code may have been scanned to
    ///     obtain the offset of each line. Explicitly setting the cache will improve the
    ///     performance of rendering diagnostic information, because the renderer does not need to
    ///     find the starting position of a line separately.
    ///     2. If there is no information about the first byte position of a line, when the code of
    ///     this line needs to be accessed, the information will be calculated and cached. We try to
    ///     iterate over as few bytes as possible to find the information we need, for example we
    ///     might process a new line from an already calculated line.
    std::map<unsigned, std::size_t> line_offsets_;
    /// The (1-indexed) line number of the first line in the source code. The line numbers of
    /// subsequent lines will be calculated based on this, which allows us to provide a portion of
    /// the source code and explicitly specify the actual line number of the first line to display
    /// the correct line number in the rendered result.
    ///
    /// Note that the line numbers of the spans stored in primary_spans_ and secondary_spans_ are
    /// relative to the current code snippet, regardless of the value of this member: they always
    /// store relative line numbers starting with 0.
    unsigned first_line_number_ = 1;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP
