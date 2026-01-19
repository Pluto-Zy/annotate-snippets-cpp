#ifndef ANNOTATE_SNIPPETS_PATCH_HPP
#define ANNOTATE_SNIPPETS_PATCH_HPP

#include "annotate_snippets/source_location.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace ants {
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
class Patch {
public:
    Patch() : replacement_lines_(0) { }

    Patch(SourceLocation beg, SourceLocation end, std::string_view replacement) :
        beg_(beg), end_(end), replacement_(replacement), replacement_lines_(0) { }

    Patch(std::size_t beg, std::size_t end, std::string_view replacement) :
        Patch(SourceLocation(beg), SourceLocation(end), replacement) { }

    Patch(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view replacement
    ) :
        Patch(SourceLocation(line_beg, col_beg), SourceLocation(line_end, col_end), replacement) { }

    auto location_begin() const -> SourceLocation {
        return beg_;
    }

    void set_location_begin(SourceLocation loc) {
        beg_ = loc;
    }

    auto location_end() const -> SourceLocation {
        return end_;
    }

    void set_location_end(SourceLocation loc) {
        end_ = loc;
    }

    auto replacement() const -> std::string_view {
        return replacement_;
    }

    void set_replacement(std::string_view replacement) {
        replacement_ = replacement;
        // Invalidate the cached replacement lines.
        replacement_lines_ = 0;
    }

    /// Computes and caches the number of lines in `replacement_`.
    void compute_replacement_lines() {
        replacement_lines_ =
            static_cast<unsigned>(std::count(replacement_.begin(), replacement_.end(), '\n') + 1);
    }

    /// Returns the number of lines of `replacement_`. If not already computed, it computes and
    /// caches the value first.
    auto replacement_lines() -> unsigned {
        // Compute and cache the number of lines if not already done.
        if (replacement_lines_ == 0) {
            compute_replacement_lines();
        }

        return replacement_lines_;
    }

    /// Returns the number of lines of `replacement_`. This function assumes that the value has
    /// already been computed and cached.
    auto replacement_lines() const -> unsigned {
        return replacement_lines_;
    }

    auto is_addition() const -> bool {
        return beg_ == end_;
    }

    auto is_deletion() const -> bool {
        return replacement_.empty();
    }

    auto is_replacement() const -> bool {
        return !is_addition() && !is_deletion();
    }

    /// Returns a `Patch` that inserts `replacement` before the character at `loc`.
    static auto addition(SourceLocation loc, std::string_view replacement) -> Patch {
        return { /*beg=*/loc, /*end=*/loc, replacement };
    }

    /// Returns a `Patch` that inserts `replacement` before the character at the specified byte
    /// offset.
    static auto addition(std::size_t byte_offset, std::string_view replacement) -> Patch {
        return addition(SourceLocation(byte_offset), replacement);
    }

    /// Returns a `Patch` that inserts `replacement` before the character at the specified line and
    /// column.
    static auto addition(unsigned line, unsigned col, std::string_view replacement) -> Patch {
        return addition(SourceLocation(line, col), replacement);
    }

    /// Returns a `Patch` that deletes the content within the specified range.
    static auto deletion(SourceLocation beg, SourceLocation end) -> Patch {
        return { beg, end, /*replacement=*/ {} };
    }

    /// Returns a `Patch` that deletes the content within the range specified by byte offsets.
    static auto deletion(std::size_t beg, std::size_t end) -> Patch {
        return deletion(SourceLocation(beg), SourceLocation(end));
    }

    /// Returns a `Patch` that deletes the content within the range specified by line and column
    /// numbers.
    static auto deletion(unsigned line_beg, unsigned col_beg, unsigned line_end, unsigned col_end)
        -> Patch  //
    {
        return deletion(SourceLocation(line_beg, col_beg), SourceLocation(line_end, col_end));
    }

    /// Returns a `Patch` that replaces the content at the specified range with `content`.
    static auto replacement(SourceLocation beg, SourceLocation end, std::string_view content)
        -> Patch  //
    {
        return { beg, end, content };
    }

    /// Returns a `Patch` that replaces the content at the specified byte offsets with `content`.
    static auto replacement(std::size_t beg, std::size_t end, std::string_view content) -> Patch {
        return replacement(SourceLocation(beg), SourceLocation(end), content);
    }

    /// Returns a `Patch` that replaces the content at the specified line and column numbers with
    /// `content`.
    static auto replacement(
        unsigned line_beg,
        unsigned col_beg,
        unsigned line_end,
        unsigned col_end,
        std::string_view content
    ) -> Patch {
        return replacement(
            SourceLocation(line_beg, col_beg),
            SourceLocation(line_end, col_end),
            content
        );
    }

private:
    /// Represents the range of the modification.
    SourceLocation beg_, end_;
    /// The content of the source code after the modification. If this is an addition, it contains
    /// the content to be inserted. If this is a deletion, it is empty. If this is a replacement, it
    /// contains the content to replace the original content.
    std::string_view replacement_;
    /// The number of lines of `replacement_`. Note that the value is computed on demand and cached
    /// for future use.
    unsigned replacement_lines_;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_PATCH_HPP
