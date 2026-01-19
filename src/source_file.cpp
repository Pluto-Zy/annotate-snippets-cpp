#include "annotate_snippets/source_file.hpp"

#include "annotate_snippets/source_location.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
auto SourceFile::remove_final_newline(std::string_view str) -> std::string_view {
    if (!str.empty() && str.back() == '\n') {
        str.remove_suffix(1);

        if (!str.empty() && str.back() == '\r') {
            // If the end is "\r\n", remove both characters.
            str.remove_suffix(1);
        }
    }

    return str;
}

namespace {
/// Scans through `content` to find the positions of newline characters and records the byte offsets
/// of the beginning of each line in `line_offsets`. The scanning continues until `pred()` returns
/// true or the end of `content` is reached. If the end of `content` is reached, the offset of the
/// hypothetical line after the last line (i.e., `content_end`) is also recorded.
template <class Pred>
void scan_lines_until(
    std::string_view content,
    std::size_t content_end,
    std::vector<std::size_t>& line_offsets,
    Pred pred
) {
    if (line_offsets.empty()) {
        line_offsets.push_back(0);
    }

    std::size_t current_offset = line_offsets.back();
    while (!pred()) {
        std::size_t const pos = content.find('\n', current_offset);
        if (pos == std::string_view::npos) {
            // No more newline characters found; we have reached the end of the content. Mark the
            // start of the last line.
            line_offsets.push_back(content_end);
            break;
        } else {
            // The start of the next line is the position after the newline character.
            current_offset = pos + 1;
            line_offsets.push_back(current_offset);
        }
    }
}
}  // namespace

void SourceFile::build_lines_to_offset(std::size_t byte_offset) {
    if (is_fully_cached()) {
        return;
    }

    scan_lines_until(content(), content_end(), storage().line_offsets, [&] {
        return storage().line_offsets.back() >= byte_offset;
    });
}

void SourceFile::build_lines_to_line(unsigned line) {
    if (is_fully_cached()) {
        return;
    }

    scan_lines_until(content(), content_end(), storage().line_offsets, [&] {
        return cached_line_count() > line;
    });
}

void SourceFile::build_lines_to_loc(SourceLocation loc) {
    assert(loc.is_partially_specified() && "loc must be partially specified");
    if (loc.has_byte_offset()) {
        build_lines_to_offset(loc.byte_offset());
    } else {
        build_lines_to_line(loc.line());
    }
}

auto SourceFile::line_content(unsigned line) const -> std::string_view {
    std::size_t const line_start = line_offset(line);
    std::size_t const line_end = line_offset(line + 1);

    if (line_start > size()) {
        return {};
    } else {
        std::string_view const result = content().substr(line_start, line_end - line_start);
        // Remove the trailing '\n' or '\r\n'.
        return remove_final_newline(result);
    }
}

auto SourceFile::byte_offset_to_line_col(  //
    std::size_t byte_offset
) const -> std::pair<unsigned, unsigned> {
    // Binary search to find the line containing `byte_offset`.
    auto iter = std::upper_bound(cached_lines().begin(), cached_lines().end(), byte_offset);

    // `iter` can never be `cached_lines().begin()` here because the first entry is always 0.
    assert(iter != cached_lines().begin());
    iter = std::prev(iter);

    return {
        static_cast<unsigned>(std::distance(cached_lines().begin(), iter)),
        static_cast<unsigned>(byte_offset - *iter),
    };
}

auto SourceFile::line_col_to_byte_offset(unsigned line, unsigned col) const -> std::size_t {
    std::size_t const line_start = line_offset(line);
    return line_start + col;
}

auto SourceFile::fill_source_location(SourceLocation loc) const -> SourceLocation {
    assert(loc.is_partially_specified() && "loc must be partially specified");
    if (loc.has_byte_offset()) {
        auto const [line, col] = byte_offset_to_line_col(loc.byte_offset());
        return loc.with_line_col(line, col);
    } else {
        return loc.with_byte_offset(line_col_to_byte_offset(loc.line(), loc.col()));
    }
}

namespace {
/// Implementation of `SourceFile::normalize_location()`. It uses template to handle both const and
/// non-const `SourceFile` objects.
template <class SourceFileType>
auto normalize_location_impl(SourceFileType& source_file, SourceLocation loc) -> SourceLocation {
    assert(loc.has_line_col() && "loc must have line and column information");

    // Extract line and column numbers from `loc`.
    //
    // We do not modify `loc` directly because if `loc` contains both kinds of location information
    // (e.g., it is the return value of `SourceFile::fill_source_location()`), calling `set_line()`
    // or `set_col()` would trigger assertions.
    unsigned line = loc.line();
    unsigned col = loc.col();

    // The location of the first byte of the specified line.
    std::size_t const line_start = source_file.line_offset(line);
    // The location of the first byte of the next line.
    std::size_t const next_line_start = source_file.line_offset(line + 1);

    if (col >= next_line_start - line_start) {
        col = 0;

        if (line_start == next_line_start) {
            // If they are equal, it means they both point to the hypothetical line at the end of
            // the file. This indicates that `line` is the last line or beyond the last line. In
            // this case, the complete line offset cache has been built, and we can directly set
            // `line` to the last line.
            assert(source_file.is_fully_cached());
            line = source_file.cached_line_count() - 1;
        } else {
            // Move to the next line.
            ++line;
        }
    }

    return source_file.fill_source_location(SourceLocation(line, col));
}
}  // namespace

auto SourceFile::normalize_location(SourceLocation loc) const -> SourceLocation {
    return normalize_location_impl(*this, loc);
}

auto SourceFile::normalize_location(SourceLocation loc) -> SourceLocation {
    return normalize_location_impl(*this, loc);
}

namespace {
template <class SourceFileType>
auto adjust_span_impl(SourceFileType& source_file, LabeledSpan span) -> LabeledSpan {
    assert(
        span.beg.has_line_col() && span.end.has_line_col()
        && "span locations must have line and column info"
    );

    // Set the `beg` and `end` locations to partial locations with only line and column information.
    span.beg = SourceLocation(span.beg.line(), span.beg.col());
    span.end = SourceLocation(span.end.line(), span.end.col());

    // We handle empty annotation ranges specially. In some cases, a user may want to annotate a
    // single character but provides an empty range (i.e., `span.beg` and `span.end` are equal), for
    // example, when attempting to annotate EOF, the front end may not provide a position like `EOF
    // + 1`. Therefore, we modify empty ranges here to annotate a single character.
    if (span.beg == span.end) {
        span.end.set_col(span.end.col() + 1);
    }

    // Sometimes we will extend the annotation to the end of a line. In the user interface, since we
    // allow users to specify the range of bytes annotated (rather than line and column numbers),
    // `span.end` will be set to the position right after the last character of this line. This
    // causes `span.end` to actually point to the first character of the next line, rather than a
    // non-existent character right after the newline character of the current line. Similarly,
    // since we always consider EOF (or any position beyond the valid byte range of the source code)
    // to belong to a hypothetical line after the last line, the same situation can occur: the user
    // intends to annotate EOF, but `span.end` points to some position in a hypothetical line.
    //
    // Therefore, when `span.end` points to the start of a line, we adjust it to point to a
    // non-existent character right after the last character of the previous line. This does not
    // affect the rendering result but allows us to correctly determine the properties of the
    // annotation, such as preventing us from incorrectly judging a single-line annotation as a
    // multi-line annotation.
    if (span.end.col() == 0) {
        unsigned const line = span.end.line();

        // Note that `span.end.line()` cannot be zero here, because if it were zero, it would imply
        // that both the line and column numbers of `span.beg` are also zero, meaning that `span`
        // points to an empty location, which has already been handled above.
        assert(line != 0);

        std::size_t const prev_line_start = source_file.line_offset(line - 1);
        std::size_t const curr_line_start = source_file.line_offset(line);

        span.end.set_col(static_cast<unsigned>(curr_line_start - prev_line_start));
        span.end.set_line(line - 1);
    }

    // Fill in the byte offsets for `span.beg` and `span.end`.
    span.beg = source_file.fill_source_location(span.beg);
    span.end = source_file.fill_source_location(span.end);

    return span;
}
}  // namespace

auto SourceFile::adjust_span(LabeledSpan span) const -> LabeledSpan {
    return adjust_span_impl(*this, std::move(span));
}

auto SourceFile::adjust_span(LabeledSpan span) -> LabeledSpan {
    return adjust_span_impl(*this, std::move(span));
}
}  // namespace ants
