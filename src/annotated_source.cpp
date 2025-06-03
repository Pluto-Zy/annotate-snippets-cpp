#include "annotate_snippets/annotated_source.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <map>
#include <string_view>
#include <utility>

namespace ants {
namespace {
/// Calculates and returns the position of the first byte of line `target_line` in the source code
/// `source`. This function assumes that this position is not stored in the cache
/// `line_offset_cache` and will add the result to the cache. This function also utilizes existing
/// information in the cache to reduce the range of characters that need to be traversed.
///
/// If `target_line` exceeds the actual number of lines in `source`, it returns `source.size() + 1`.
/// Since `AnnotatedSource` removes the trailing newline character from `source`, we need to add 1
/// to skip this newline character.
auto compute_line_offset(
    std::map<unsigned, std::size_t>& line_offset_cache,
    unsigned target_line,
    std::string_view source
) -> std::size_t {
    if (target_line == 0) {
        // If the target line is 0, we return the start of the source code.
        //
        // We handle the case where `target_line` is 0 in advance to simplify the code later.
        line_offset_cache.emplace(0, 0);
        return 0;
    }

    // Searches forward from the starting position `start_offset` of `start_line` to find the
    // position of the target line `target_line`. If `target_line` exceeds the actual number of
    // lines, returns `source.size() + 1` and adds the line immediately following the actual last
    // line to the cache.
    auto const find_forward = [&](unsigned start_line, std::size_t start_offset) {
        if (start_offset > source.size()) {
            // If `start_offset` exceeds the size of `source`, we are trying to find a line from the
            // added hypothetical line. We don't add the target line to the cache in this case, and
            // return `source.size() + 1` to indicate that the target line is beyond the actual
            // number of lines.
            return source.size() + 1;
        }

        for (unsigned cur_line = start_line; cur_line != target_line; ++cur_line) {
            std::size_t const pos = source.find('\n', start_offset);
            if (pos == std::string_view::npos) {
                // There are not enough lines, indicating that `target_line` exceeds the actual
                // number of lines. At this point, `cur_line` is the line number of the last line,
                // and we store the line immediately following the last line in the cache.
                //
                // Note that the next line starts at `source.size() + 1`, which is the position
                // immediately after the removed trailing newline character.
                line_offset_cache.emplace(cur_line + 1, source.size() + 1);
                return source.size() + 1;
            }
            // FIXME: Should we also cache the information about the lines we pass through during
            // traversal?
            start_offset = pos + 1;
        }

        line_offset_cache.emplace(target_line, start_offset);
        return start_offset;
    };

    // Finds the index of the first byte of `target_line` by searching backwards from `start_line`.
    auto const find_backward = [&](unsigned start_line, std::size_t start_offset) {
        // Now `start_offset` is the start position of line `start_line`, and we need to search from
        // this position. However, we already know that the character at `start_offset - 1` is the
        // newline character of the previous line, so we start searching from `start_offset - 2` to
        // reduce the number of `rfind()` calls.
        //
        // We also need to consider the possibility of `start_offset` wrapping around to the maximum
        // value. Since we will not start searching backward from line 0, `start_offset` will not be
        // 0. It will also not be 1, because this would only happen when we try to search backward
        // from line 1 (where `start_line` is 1) to find the start position of line 0 (where
        // `target_line` is 0), and we have already handled the case in advance. Therefore, we can
        // safely calculate `start_offset - 2`.
        --start_offset;

        for (unsigned cur_line = start_line; cur_line != target_line; --cur_line) {
            // `start_offset` now points to the newline character at the end of the previous line of
            // `cur_line`. To continue searching backward, we decrement `start_offset` by 1 to skip
            // this newline character. This is safe according to the previous description.
            --start_offset;
            // FIXME: Should we also cache the information about the lines we pass through during
            // traversal?
            start_offset = source.rfind('\n', start_offset);
        }

        line_offset_cache.emplace(target_line, start_offset + 1);
        return start_offset + 1;
    };

    // Points to the line closest to and immediately following the target line.
    auto const closest_next_iter = line_offset_cache.upper_bound(target_line);

    // Points to the line closest to and immediately preceding the target line.
    auto const closest_prev_iter = closest_next_iter == line_offset_cache.begin()
        ? closest_next_iter
        : std::prev(closest_next_iter, 1);

    if (closest_next_iter != line_offset_cache.end()
        && closest_prev_iter != line_offset_cache.end())
    {
        // If there are calculated lines before and after `target_line`, we traverse from the
        // closest one.
        if (closest_next_iter->first - target_line < target_line - closest_prev_iter->first) {
            return find_backward(closest_next_iter->first, closest_next_iter->second);
        } else {
            return find_forward(closest_prev_iter->first, closest_prev_iter->second);
        }
    } else if (closest_prev_iter != line_offset_cache.end()) {
        // Only the line closest to and preceding the target line have been calculated, so we start
        // calculating from this line.
        return find_forward(closest_prev_iter->first, closest_prev_iter->second);
    } else if (closest_next_iter != line_offset_cache.end()) {
        // Only the line closest to and following the target line have been calculated. But since we
        // can always start computing from line 0, we check if line 0 is closer.
        if (closest_next_iter->first - target_line < target_line) {
            return find_backward(closest_next_iter->first, closest_next_iter->second);
        } else {
            return find_forward(0, 0);
        }
    } else {
        // There are no other results in the cache, so we start calculating from line 0.
        return find_forward(0, 0);
    }
}

/// Calculates the line on which the byte at `byte_offset` is located, returns the line number and
/// the offset of the first byte of that line. This function also adds this line to the cache. It
/// attempts to use the existing cache `line_offset_cache` to compute the result more quickly.
///
/// Note that since we implicitly add a newline character at the end of the source code, if
/// `byte_offset` is equal to `source.size()`, it is considered to be on the last line. If it is
/// greater than `source.size()`, it is considered to be on the line immediately following the last
/// line.
auto byte_offset_to_line(
    std::map<unsigned, std::size_t>& line_offset_cache,
    std::size_t byte_offset,
    std::string_view source
) -> std::pair<unsigned, std::size_t> {
    // Searches backward from `byte_offset` to find the position of the first newline character to
    // determine the start of the line containing `byte_offset`.
    auto const find_line_start = [&] {
        if (byte_offset == 0) {
            // Our search starts at `byte_offset - 1`, because we cannot include the newline
            // character exactly at `byte_offset`. Therefore, we need to check if `byte_offset` is
            // 0.
            return static_cast<std::size_t>(0);
        } else if (byte_offset > source.size()) {
            // If the requested position is greater than the size of `source`, it is considered to
            // be on the line immediately following the last line. Note that the line starts at
            // `source.size() + 1`, which is the position immediately after the removed trailing
            // newline character.
            return source.size() + 1;
        } else {
            std::size_t const pos = source.rfind('\n', byte_offset - 1);
            // If `pos` is `std::string_view::npos`, since `npos` is defined as `size_type(-1)`,
            // adding 1 to `npos` still yields 0, which is our expected result.
            return pos + 1;
        }
    };

    // Searches forward from `start_offset` to determine the line number containing `byte_offset`,
    // returning its line number and the start position of this line. If `byte_offset` is greater
    // than `source.size()`, it is considered to be on the line immediately following the actual
    // last line. This function does not modify the cache.
    auto const find_forward = [&](unsigned start_line, std::size_t start_offset) {
        if (start_offset > source.size()) {
            // If `start_offset` is greater than the size of `source`, it is considered to be on the
            // line immediately following the last line. Note that the line starts at `source.size()
            // + 1`, which is the position immediately after the removed trailing newline character.
            return std::make_pair(start_line, source.size() + 1);
        }

        // Counts the number of newline characters between [start_offset, byte_offset).
        std::string_view const substr = source.substr(start_offset, byte_offset - start_offset);
        auto lines = static_cast<unsigned>(std::count(substr.begin(), substr.end(), '\n'));
        // If `byte_offset` exceeds the valid range of `source` but `start_offset` is still within
        // the valid range, we need to consider the hypothetical line where `byte_offset` is
        // located.
        if (byte_offset > source.size()) {
            ++lines;
        }

        return std::make_pair(start_line + lines, find_line_start());
    };
    // Calls `find_forward()` and caches the result.
    auto const find_forward_and_cache = [&](unsigned start_line, std::size_t start_offset) {
        auto res = find_forward(start_line, start_offset);
        line_offset_cache.insert(res);
        return res;
    };

    // Searches backward from `start_offset` to determine the line number of `byte_offset`,
    // returning its line number and the start position of this line. The result is cached.
    auto const find_backward_and_cache = [&](unsigned start_line, std::size_t start_offset) {
        // Now `start_offset` is the start position of line `start_line`, and we need to move
        // `start_offset` to the end of the previous line. We assume `start_offset` cannot be 0 (as
        // we do not search backward from the first byte). This allows us to handle actual lines and
        // hypothetical lines in a uniform manner.
        --start_offset;

        // Counts the number of newline characters between [byte_offset, start_offset).
        //
        // Note that in this function, `byte_offset` will not exceed `source.size()`, because the
        // starting position of the hypothetical last line in the cache is `source.size() + 1`. In
        // this case, `closest_next_iter` will be the end iterator, and we will only calculate the
        // line number using `find_forward()` rather than `find_backward()`.
        std::string_view const substr = source.substr(byte_offset, start_offset - byte_offset);
        auto const lines = static_cast<unsigned>(std::count(substr.begin(), substr.end(), '\n'));

        // Since we skipped a line, we need to decrement by an additional line.
        std::pair res(start_line - lines - 1, find_line_start());
        line_offset_cache.insert(res);
        return res;
    };

    // Checks if the cache already contains the starting position of the line near `byte_offset`; if
    // so, the search can start from a closer position. For this, we perform a binary search on the
    // values of `line_offset_cache`, which are also sorted.

    // Points to the cached line closest to and following `byte_offset`.
    // NOLINTBEGIN(performance-inefficient-algorithm): Since we compare `byte_offset` with the value
    // rather than the key in the map, we use `upper_bound` rather than `map::upper_bound`.
    auto const closest_next_iter = std::upper_bound(
        line_offset_cache.begin(),
        line_offset_cache.end(),
        byte_offset,
        // We search using the values associated with the keys in the map.
        [](std::size_t lhs, auto const& rhs) { return lhs < rhs.second; }
    );
    // NOLINTEND(performance-inefficient-algorithm)

    // Points to the cached line closest to and preceding `byte_offset`. If the actual line
    // containing `byte_offset` is already in the cache, it points to that line.
    auto const closest_prev_iter = closest_next_iter == line_offset_cache.begin()
        ? closest_next_iter
        : std::prev(closest_next_iter, 1);

    if (closest_prev_iter != line_offset_cache.end()
        && closest_next_iter != line_offset_cache.end())
    {
        // If the lines before and after are adjacent, then we have already found the line
        // containing the target byte, so we return the result without modifying the cache.
        if (closest_prev_iter->first + 1 == closest_next_iter->first) {
            return *closest_prev_iter;
        }

        // Otherwise, we start the search from the closest line.
        if (closest_next_iter->second - byte_offset < byte_offset - closest_prev_iter->second) {
            return find_backward_and_cache(closest_next_iter->first, closest_next_iter->second);
        } else {
            return find_forward_and_cache(closest_prev_iter->first, closest_prev_iter->second);
        }
    } else if (closest_prev_iter != line_offset_cache.end()) {
        // Only lines before the target position are cached, so we start the search from the nearest
        // line.
        return find_forward_and_cache(closest_prev_iter->first, closest_prev_iter->second);
    } else if (closest_next_iter != line_offset_cache.end()) {
        // Only lines after the target position are cached. However, since we can always start from
        // the beginning, we check if starting from the beginning is closer.
        if (closest_next_iter->second - byte_offset < byte_offset) {
            return find_backward_and_cache(closest_next_iter->first, closest_next_iter->second);
        } else {
            return find_forward_and_cache(0, 0);
        }
    } else {
        // No results are in the cache, so we start the search from the beginning.
        return find_forward_and_cache(0, 0);
    }
}
}  // namespace

auto AnnotatedSource::line_offset(unsigned line) -> std::size_t {
    if (auto const cache_iter = line_offsets_.find(line); cache_iter != line_offsets_.end()) {
        // If the result is already cached in the map, returns it directly.
        return cache_iter->second;
    } else {
        return compute_line_offset(line_offsets_, line, source_);
    }
}

auto AnnotatedSource::byte_offset_to_line_col(std::size_t byte_offset) -> SourceLocation {
    auto const [line, line_start] = byte_offset_to_line(line_offsets_, byte_offset, source_);
    return {
        /*line=*/line,
        /*col=*/static_cast<unsigned>(byte_offset - line_start),
    };
}

auto AnnotatedSource::normalize_location(SourceLocation loc) -> SourceLocation {
    // The location of the first byte of the specified line.
    std::size_t const line_start = line_offset(loc.line);
    // The location of the first byte of the next line.
    std::size_t const next_line_start = line_offset(loc.line + 1);

    if (loc.col >= next_line_start - line_start) {
        // If the column exceeds the number of characters in the line, we return the start of the
        // next line.
        //
        // We use `byte_offset_to_line_col()` to generate the result, rather than returning
        // `SourceLocation { loc.line + 1, 0 }`, because the next line may not exist, and
        // `byte_offset_to_line_col()` will return the position of the end of the source code in
        // this case.
        return byte_offset_to_line_col(next_line_start);
    } else {
        return loc;
    }
}

auto AnnotatedSource::normalize_location(std::size_t byte_offset) -> SourceLocation {
    return byte_offset_to_line_col(std::min(byte_offset, source_.size()));
}

auto AnnotatedSource::line_content(unsigned line) -> std::string_view {
    std::size_t const line_start = line_offset(line);
    std::size_t const line_end = line_offset(line + 1);

    if (line_start > source_.size()) {
        return {};
    } else {
        std::string_view const result = source_.substr(line_start, line_end - line_start);
        // Remove the trailing '\n' or '\r\n'.
        return remove_final_newline(result);
    }
}

auto LabeledSpan::adjust(AnnotatedSource& source) const -> LabeledSpan {
    LabeledSpan result = *this;

    // We handle empty annotation ranges specially. In some cases, a user may want to annotate a
    // single character but provides an empty range (i.e., `result.beg` and `result.end` are equal),
    // for example, when attempting to annotate EOF, the front end may not provide a position like
    // `EOF + 1`. Therefore, we modify empty ranges here to annotate a single character.
    if (result.beg == result.end) {
        ++result.end.col;
    }

    // Sometimes we will extend the annotation to the end of a line. In the user interface, since we
    // allow users to specify the range of bytes annotated (rather than line and column numbers),
    // `result.end` will be set to the position right after the last character of this line. This
    // causes `result.end` to actually point to the first character of the next line, rather than a
    // non-existent character right after the newline character of the current line. Similarly,
    // since we always consider EOF (or any position beyond the valid byte range of the source code)
    // to belong to a hypothetical line after the last line, the same situation can occur: the user
    // intends to annotate EOF, but `result.end` points to some position in a hypothetical line.
    //
    // Therefore, when `result.end` points to the start of a line, we adjust it to point to a
    // non-existent character right after the last character of the previous line. This does not
    // affect the rendering result but allows us to correctly determine the properties of the
    // annotation, such as preventing us from incorrectly judging a single-line annotation as a
    // multi-line annotation.
    if (result.end.col == 0) {
        // To get the end position of the previous line, we calculate the offsets of the first
        // characters of the previous line and the current line respectively. This may involve
        // caching, but it does not introduce unnecessary calculations, as our results will also be
        // used again when rendering actual code lines.
        std::size_t const prev_line_start = source.line_offset(result.end.line - 1);
        std::size_t const cur_line_start = source.line_offset(result.end.line);

        result.end.col = static_cast<unsigned>(cur_line_start - prev_line_start);
        --result.end.line;
    }

    return result;
}
}  // namespace ants
