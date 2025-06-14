#ifndef ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP
#define ANNOTATE_SNIPPETS_ANNOTATED_SOURCE_HPP

#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
class AnnotatedSource;

/// Represents the location of a *byte* in the source code.
// FIXME: unsigned or std::size_t?
struct SourceLocation {
    /// The (0-indexed) line number of the location.
    unsigned line;
    /// The (0-indexed) column number of the location.
    unsigned col;

    friend constexpr auto operator==(SourceLocation lhs, SourceLocation rhs) -> bool {
        return lhs.line == rhs.line && lhs.col == rhs.col;
    }

    friend constexpr auto operator!=(SourceLocation lhs, SourceLocation rhs) -> bool {
        return !(lhs == rhs);
    }

    friend constexpr auto operator<(SourceLocation lhs, SourceLocation rhs) -> bool {
        return lhs.line < rhs.line || (lhs.line == rhs.line && lhs.col < rhs.col);
    }

    friend constexpr auto operator<=(SourceLocation lhs, SourceLocation rhs) -> bool {
        return !(rhs < lhs);
    }

    friend constexpr auto operator>(SourceLocation lhs, SourceLocation rhs) -> bool {
        return rhs < lhs;
    }

    friend constexpr auto operator>=(SourceLocation lhs, SourceLocation rhs) -> bool {
        return !(lhs < rhs);
    }
};

/// Represents a single annotation span in the source code, with an optional label. When rendering
/// diagnostic information, a non-empty label will be rendered together with the corresponding span.
struct LabeledSpan {
    SourceLocation beg;
    SourceLocation end;
    /// The label attached to this span. If label.empty() is true, we consider the annotation to
    /// have no label attached.
    StyledStringView label;

    /// Adjusts the range of the span. Returns the adjusted span (does not modify `*this`).
    ///
    /// This function adjusts the span in two ways:
    /// 1. If the current span points to an empty range (`beg == end`), it adjusts it to include 1
    ///    byte.
    /// 2. If the current span's `end` points to the start of a line, it adjusts it to point to the
    ///    end of the previous line. Since the character pointed to by `end` is not included in the
    ///    span, this ensures that we render the correct result (e.g., it does not treat a
    ///    single-line span as a multi-line span). Consider the following example:
    ///
    ///        "hello"
    ///        ^^^^^^^ We want to annotate this word.
    ///        "world"
    ///        ^ However, `end` points to the start of the next line. We cannot render this line.
    ///
    /// This function requires an `AnnotatedSource` object to correctly calculate the length of
    /// lines.
    [[nodiscard]] auto adjust(AnnotatedSource& source) const -> LabeledSpan;
};

/// Represents a modification to the source code.
///
/// `Patch` uses a pair of locations to represent the start and end positions of the modification,
/// and stores the content of the source code after the modification.
///
/// `Patch` supports 3 different types of modifications:
/// 1. Addition: Indicates that new content is inserted at the specified position. In this case,
///    `beg_ == end_`, and the new string will be inserted before the character pointed to by
///    `beg_`.
/// 2. Deletion: Indicates that the content at the specified position is deleted. In this case,
///    `replacement_` is empty.
/// 3. Replacement: Indicates that the content at the specified position is replaced with new
///    content. In this case, `beg_ != end_`, and `replacement_` is non-empty.
///
/// Note that `Patch` supports specifying locations either as `SourceLocation` or byte offsets.
/// However, we can only convert one representation to another when we have an `AnnotatedSource`
/// object. Inside `AnnotatedSource`, we convert all `Patch` objects to the `SourceLocation`
/// representation.
struct Patch {
    Patch() : Patch(SourceLocation(), SourceLocation(), {}) { }

    Patch(SourceLocation beg, SourceLocation end, std::string_view replacement) :
        beg_(beg),
        end_(end),
        replacement_(replacement),
        replacement_lines_(
            static_cast<unsigned>(std::count(replacement.begin(), replacement.end(), '\n') + 1)
        ),
        location_type_(LineColumn) { }

    Patch(std::size_t beg, std::size_t end, std::string_view replacement) :
        beg_(beg),
        end_(end),
        replacement_(replacement),
        replacement_lines_(
            static_cast<unsigned>(std::count(replacement.begin(), replacement.end(), '\n') + 1)
        ),
        location_type_(ByteOffset) { }

    auto is_addition() const -> bool {
        switch (location_type_) {
        case LineColumn:
            return beg_.loc_ == end_.loc_;

        case ByteOffset:
        default:
            return beg_.byte_offset_ == end_.byte_offset_;
        }
    }

    auto is_deletion() const -> bool {
        return replacement_.empty();
    }

    auto is_replacement() const -> bool {
        return !is_addition() && !is_deletion();
    }

    auto location_begin() const -> SourceLocation {
        assert(location_type_ == LineColumn);
        return beg_.loc_;
    }

    auto location_end() const -> SourceLocation {
        assert(location_type_ == LineColumn);
        return end_.loc_;
    }

    auto replacement() const -> std::string_view {
        return replacement_;
    }

    auto replacement_lines() const -> unsigned {
        return replacement_lines_;
    }

    /// Returns a `Patch` that inserts `replacement` before the character at `loc`.
    static auto addition(SourceLocation loc, std::string_view replacement) -> Patch {
        return { /*beg=*/loc, /*end=*/loc, replacement };
    }

    /// Returns a `Patch` that inserts `replacement` before the character at the specified byte
    /// offset.
    static auto addition(std::size_t byte_offset, std::string_view replacement) -> Patch {
        return { /*beg=*/byte_offset, /*end=*/byte_offset, replacement };
    }

    /// Returns a `Patch` that deletes the content at the specified range.
    static auto deletion(SourceLocation beg, SourceLocation end) -> Patch {
        return { beg, end, /*replacement=*/ {} };
    }

    /// Returns a `Patch` that deletes the content at the specified byte offsets.
    static auto deletion(std::size_t beg, std::size_t end) -> Patch {
        return { beg, end, /*replacement=*/ {} };
    }

    /// Returns a `Patch` that replaces the content at the specified range with `replacement`.
    static auto replacement(  //
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) -> Patch {
        return { beg, end, replacement };
    }

    /// Returns a `Patch` that replaces the content at the specified byte offsets with
    /// `replacement`.
    static auto replacement(  //
        std::size_t beg,
        std::size_t end,
        std::string_view replacement
    ) -> Patch {
        return { beg, end, replacement };
    }

private:
    union Location {
        SourceLocation loc_;
        std::size_t byte_offset_;

        Location(SourceLocation loc) : loc_(loc) { }
        Location(std::size_t byte_offset) : byte_offset_(byte_offset) { }
    } beg_, end_;
    /// The content of the source code after the modification. If this is an addition, it contains
    /// the content to be inserted. If this is a deletion, it is empty. If this is a replacement, it
    /// contains the content to replace the original content.
    std::string_view replacement_;
    /// The number of lines of `replacement_`. The value is computed and cached here to avoid
    /// recalculating.
    unsigned replacement_lines_;
    /// Represents the type of the `beg_` and `end_` locations. This is used to determine how to
    /// interpret the `beg_` and `end_` locations when applying the patch.
    enum : std::uint8_t {
        /// Indicates that the `beg_` and `end_` locations are specified as `SourceLocation`.
        LineColumn,
        /// Indicates that the `beg_` and `end_` locations are specified as byte offsets.
        ByteOffset,
    } location_type_;

    friend AnnotatedSource;
};

/// Represents source code with some annotations and fixes.
///
/// Note that `AnnotatedSource` assumes that once constructed, the code it refers to will not be
/// changed, since `AnnotatedSource` only stores the relative location of annotations and does not
/// own the code.
class AnnotatedSource {
    /// Removes the trailing newline character from `str` if it exists.
    ///
    /// Our implementation requires that the source always ends with a newline character. If it does
    /// not, we append a newline character at the end. However, we cannot append characters to
    /// `string_view`, so we choose to remove the newline character at the end of `str` to ensure
    /// that we can handle the source code in a consistent manner.
    static auto remove_final_newline(std::string_view str) -> std::string_view {
        if (!str.empty() && str.back() == '\n') {
            str.remove_suffix(1);

            if (!str.empty() && str.back() == '\r') {
                // If the end is "\r\n", remove both characters.
                str.remove_suffix(1);
            }
        }

        return str;
    }

public:
    /// Creates an `AnnotatedSource` object that is not associated with any code snippet.
    AnnotatedSource() = default;
    /// Creates an `AnnotatedSource` object that is associated with code snippet `source`.
    explicit AnnotatedSource(std::string_view source) : source_(remove_final_newline(source)) { }
    /// Create an `AnnotatedSource` object associated with the code snippet `source` and specify its
    /// origin as `origin`.
    explicit AnnotatedSource(std::string_view source, std::string_view origin) :
        source_(remove_final_newline(source)), origin_(origin) { }

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
    ///
    /// If `line` is greater than the number of lines in the source code, the offset of the last
    /// line is returned. Note that if the provided source code does not end with a newline
    /// character, `AnnotatedSource` will append a newline character at the end. In this case, the
    /// offset of the last line will be `len + 1`, where `len` is the length of the provided source.
    auto line_offset(unsigned line) -> std::size_t;

    void set_line_offset(unsigned line, std::size_t offset) {
        line_offsets_[line] = offset;
    }

    template <class Iter>
    void set_line_offsets(Iter begin, Iter end) {
        line_offsets_.insert(std::make_move_iterator(begin), std::make_move_iterator(end));
    }

    /// Returns the line and column number of the byte at offset `byte_offset` in the source code.
    ///
    /// This method caches the position of the first character of the line where `byte_offset` is
    /// located into the cache `line_offsets_`.
    ///
    /// If `byte_offset` exceeds the length of the source code, the last line and column are
    /// returned. Note that if the provided source code does not end with a newline character,
    /// `AnnotatedSource` will append a newline character at the end. In this case, there will be
    /// one more line than the actual number of lines in the source code, and the length of the
    /// source code will also be increased by one character.
    auto byte_offset_to_line_col(std::size_t byte_offset) -> SourceLocation;

    /// Adjusts `loc` to ensure it is within the valid range of the source code.
    ///
    /// This function checks if `loc` is within the range of the source code. It handles the
    /// following two scenarios:
    /// - If the column of `loc` exceeds the number of valid characters in that line, it adjusts
    ///   `loc` to point to the start of the next line.
    /// - If the line of `loc` exceeds the actual number of lines in the source code, it adjusts
    ///   `loc` to point to the end of the source code (`source.size()`).
    ///
    /// Returns the adjusted `SourceLocation`.
    auto normalize_location(SourceLocation loc) -> SourceLocation;
    auto normalize_location(std::size_t byte_offset) -> SourceLocation;

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
        // clang-format off
        primary_spans_.push_back(
            LabeledSpan {
                /*beg=*/beg,
                /*end=*/end,
                /*label=*/std::move(label),
            }
            .adjust(*this)
        );
        // clang-format on
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
        // clang-format off
        secondary_spans_.push_back(
            LabeledSpan {
                /*beg=*/beg,
                /*end=*/end,
                /*label=*/std::move(label),
            }
            .adjust(*this)
        );
        // clang-format on
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

    auto patches() const -> std::vector<Patch> const& {
        return patches_;
    }

    auto patches() -> std::vector<Patch>& {
        return patches_;
    }

    void add_patch(Patch patch) {
        switch (patch.location_type_) {
        case Patch::LineColumn:
            patch.beg_ = normalize_location(patch.beg_.loc_);
            patch.end_ = normalize_location(patch.end_.loc_);
            break;

        case Patch::ByteOffset:
        default:
            patch.beg_ = normalize_location(patch.beg_.byte_offset_);
            patch.end_ = normalize_location(patch.end_.byte_offset_);
            patch.location_type_ = Patch::LineColumn;
            break;
        }

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
        patches_.push_back(
            Patch::replacement(normalize_location(beg), normalize_location(end), replacement)
        );
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
        patches_.push_back(Patch::replacement(
            normalize_location(byte_beg),
            normalize_location(byte_end),
            replacement
        ));
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

    void add_addition_patch(SourceLocation loc, std::string_view replacement) {
        patches_.push_back(Patch::addition(normalize_location(loc), replacement));
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
        patches_.push_back(Patch::addition(normalize_location(byte_loc), replacement));
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

    void add_deletion_patch(SourceLocation beg, SourceLocation end) {
        patches_.push_back(Patch::deletion(normalize_location(beg), normalize_location(end)));
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
        patches_.push_back(
            Patch::deletion(normalize_location(byte_beg), normalize_location(byte_end))
        );
    }

    auto with_deletion_patch(std::size_t byte_beg, std::size_t byte_end) & -> AnnotatedSource& {
        add_deletion_patch(byte_beg, byte_end);
        return *this;
    }

    auto with_deletion_patch(std::size_t byte_beg, std::size_t byte_end) && -> AnnotatedSource&& {
        add_deletion_patch(byte_beg, byte_end);
        return std::move(*this);
    }

    void add_replacement_patch(
        SourceLocation beg,
        SourceLocation end,
        std::string_view replacement
    ) {
        patches_.push_back(
            Patch::replacement(normalize_location(beg), normalize_location(end), replacement)
        );
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
        patches_.push_back(Patch::replacement(
            normalize_location(byte_beg),
            normalize_location(byte_end),
            replacement
        ));
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
    /// A collection of patches, which represent suggested modifications to the source code.
    std::vector<Patch> patches_;
    /// Caches the offset of the first byte of each line in the entire source code (`source_`). It
    /// is used to quickly find a line of source code when rendering diagnostic information.
    ///
    /// There are several ways to modify the cache:
    /// 1. The user can explicitly specify the offset of the starting byte of a line by
    ///    `set_line_offset()` and `set_line_offsets()`, because this information is usually known
    ///    in other compilation stages, for example, the source code may have been scanned to obtain
    ///    the offset of each line. Explicitly setting the cache will improve the performance of
    ///    rendering diagnostic information, because the renderer does not need to find the starting
    ///    position of a line separately.
    /// 2. If there is no information about the first byte position of a line, when the code of this
    ///    line needs to be accessed, the information will be calculated and cached. We try to
    ///    iterate over as few bytes as possible to find the information we need, for example we
    ///    might process a new line from an already calculated line.
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
