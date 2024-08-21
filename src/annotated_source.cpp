#include "annotate_snippets/annotated_source.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <string_view>
#include <utility>

namespace ants {
namespace {
/// Calculates and returns the position of the first byte of line `target_line` in the source code
/// `source`. This function assumes that this position is not stored in the cache
/// `line_offset_cache` and will add the result to the cache. This function also utilizes existing
/// information in the cache to reduce the range of characters that need to be traversed. If
/// `target_line` exceeds the actual number of lines in `source`, it returns `source.size()`, and
/// this result is not added to the cache.
auto compute_line_offset(
    std::map<unsigned, std::size_t>& line_offset_cache,
    unsigned target_line,
    std::string_view source
) -> std::size_t {
    // Searches forward from the starting position `start_offset` of `start_line` to find the
    // position of the target line `target_line`. If `target_line` exceeds the actual number of
    // lines, returns `source.size()` and adds the line immediately following the actual last line
    // to the cache.
    auto const find_forward = [&](unsigned start_line, std::size_t start_offset) -> std::size_t {
        for (unsigned cur_line = start_line; cur_line != target_line; ++cur_line) {
            std::size_t const pos = source.find('\n', start_offset);
            if (pos == std::string_view::npos) {
                // There are not enough lines, indicating that `target_line` exceeds the actual
                // number of lines. At this point, `cur_line` is the line number of the last line,
                // and we store the line immediately following the last line in the cache.
                //
                // Special case: If `source` ends with '\n', then the actual last line is empty,
                // which causes the last line and the line following the last line to have the same
                // starting position. In this case, we do not add the hypothetical end line.
                unsigned const line = start_offset == source.size() ? cur_line : cur_line + 1;
                line_offset_cache[line] = source.size();
                return source.size();
            }
            // FIXME: Should we also cache the information about the lines we pass through during
            // traversal?
            start_offset = pos + 1;
        }

        line_offset_cache[target_line] = start_offset;
        return start_offset;
    };

    // Finds the index of the first byte of `target_line` by searching backwards from `start_line`.
    auto const find_backward = [&](unsigned start_line, std::size_t start_offset) -> std::size_t {
        // Now `start_offset` is the start position of line `start_line`, and we need to move
        // `start_offset` to the end of the previous line. We assume `start_line` cannot be 0 (as we
        // do not search backward from line 0), so `start_offset` will not be 0.
        --start_offset;

        for (unsigned cur_line = start_line; cur_line != target_line; --cur_line) {
            // `start_offset` now points to the newline character at the end of the previous line of
            // `cur_line`. To continue searching backward, we decrement `start_offset` by 1 to skip
            // this newline character. However, if the content of the current line is only one
            // newline character, `start_offset - 1` would jump directly to an even earlier line.
            // Generally, this is not an issue, except when we are already at line 0: further
            // decrementing would cause `start_offset` to underflow, resulting in `rfind()`
            // searching the entire string. Therefore, when we are already at the start, we no
            // longer continue to search and return 0 directly.
            --start_offset;
            if (start_offset == std::string_view::npos) {
                break;
            }
            // FIXME: Should we also cache the information about the lines we pass through during
            // traversal?
            start_offset = source.rfind('\n', start_offset);
        }

        // If `rfind()` returns `npos`, then since `npos` is defined as `size_type(-1)`, `npos + 1`
        // will yield 0, which is what we expect.
        line_offset_cache[target_line] = start_offset + 1;
        return start_offset + 1;
    };

    // Points to the line closest to and immediately following the target line.
    auto const closest_next_iter = line_offset_cache.upper_bound(target_line);

    // Points to the line closest to and immediately preceding the target line.
    auto const closest_prev_iter =
        std::ranges::prev(closest_next_iter, 1, line_offset_cache.begin());

    if (closest_next_iter != line_offset_cache.end()
        && closest_prev_iter != line_offset_cache.end()) {
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
        } else if (byte_offset >= source.size()) {
            // If the requested position exceeds the valid range of `source`, we cannot use
            // `rfind()` to locate the start of this line, as there may not necessarily be a newline
            // character between the hypothetical end line and the actual last line of the file.
            return source.size();
        } else {
            std::size_t const pos = source.rfind('\n', byte_offset - 1);
            // If `pos` is `std::string_view::npos`, since `npos` is defined as `size_type(-1)`,
            // adding 1 to `npos` still yields 0, which is our expected result.
            return pos + 1;
        }
    };

    // Searches forward from `start_offset` to determine the line number containing `byte_offset`,
    // returning its line number and the start position of this line. If `byte_offset` exceeds the
    // valid range of `source`, it is considered to be on the line immediately following the actual
    // last line. This function does not modify the cache.
    auto const find_forward = [&](unsigned start_line, std::size_t start_offset) {
        // Counts the number of newline characters between [start_offset, byte_offset).
        auto lines = static_cast<unsigned>(
            std::ranges::count(source.substr(start_offset, byte_offset - start_offset), '\n')
        );
        // If `byte_offset` exceeds the valid range of `source` but `start_offset` is still within
        // the valid range, we need to consider the hypothetical line where `byte_offset` is
        // located. We only add this hypothetical line if `source` does not end with '\n'.
        if (byte_offset >= source.size() && start_offset != source.size()) {
            if (!source.empty() && source.back() != '\n') {
                ++lines;
            }
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
        auto const lines = static_cast<unsigned>(
            std::ranges::count(source.substr(byte_offset, start_offset - byte_offset), '\n')
        );

        // Since we skipped a line, we need to decrement by an additional line.
        std::pair res(start_line - lines - 1, find_line_start());
        line_offset_cache.insert(res);
        return res;
    };

    // Checks if the cache already contains the starting position of the line near `byte_offset`; if
    // so, the search can start from a closer position. For this, we perform a binary search on the
    // values of `line_offset_cache`, which are also sorted.

    // Points to the cached line closest to and following `byte_offset`.
    // NOLINTBEGIN(misc-include-cleaner): The include cleaner mistakenly assumes that `<algorithm>`
    // is not the header for `std::ranges::upper_bound`, resulting in the warning. See
    // https://github.com/llvm/llvm-project/issues/94459.
    auto const closest_next_iter = std::ranges::upper_bound(
        line_offset_cache,
        byte_offset,
        std::ranges::less(),
        // We search using the values associated with the keys in the map.
        [](auto const& pair) { return pair.second; }
    );
    // NOLINTEND(misc-include-cleaner)

    // Points to the cached line closest to and preceding `byte_offset`. If the actual line
    // containing `byte_offset` is already in the cache, it points to that line.
    auto const closest_prev_iter =
        std::ranges::prev(closest_next_iter, 1, line_offset_cache.begin());

    if (closest_prev_iter != line_offset_cache.end()
        && closest_next_iter != line_offset_cache.end()) {
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
        .line = line,
        .col = static_cast<unsigned>(byte_offset - line_start),
    };
}

auto AnnotatedSource::line_content(unsigned line) -> std::string_view {
    std::size_t const line_start = line_offset(line);
    std::size_t const line_end = line_offset(line + 1);

    if (line_start >= source_.size()) {
        return {};
    } else {
        std::string_view result = source_.substr(line_start, line_end - line_start);

        // Remove the trailing '\n'.
        if (!result.empty() && result.back() == '\n') {
            result = result.substr(0, result.size() - 1);

            // If the end is "\r\n", remove both characters.
            if (!result.empty() && result.back() == '\r') {
                result = result.substr(0, result.size() - 1);
            }
        }

        return result;
    }
}
}  // namespace ants
