#include "annotate_snippets/styled_string_view.hpp"

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
void StyledStringView::set_style(Style style, std::size_t start_index, std::size_t end_index) {
    if (start_index == end_index) {
        // Don't modify anything if the range is empty.
        return;
    }

    // We assume that styled_parts_ is non-empty and styled_parts_.front().start_index is 0.
    auto const beg_iter = std::ranges::lower_bound(
        styled_parts_,
        start_index,
        std::ranges::less(),
        [](StyledPart const& part) { return part.start_index; }
    );
    auto const end_iter = std::ranges::upper_bound(
        styled_parts_,
        end_index,
        std::ranges::less(),
        [](StyledPart const& part) { return part.start_index; }
    );
    // This iterator points to the last element to be removed. We must save the style of the
    // element. Note that since the first element of styled_parts_ is 0, we can find that end_iter
    // won't be styled_parts_.begin(), so that we can call prev() on it safely.
    auto const last_iter = std::ranges::prev(end_iter);
    Style const end_style = last_iter->style;

    // Replace the existing StyledParts in the range with updated StyledParts.
    auto const insert_pos = styled_parts_.erase(beg_iter, end_iter);
    // clang-format off
    styled_parts_.insert(
        insert_pos,
        {
            { .start_index = start_index, .style = style },
            { .start_index = end_index, .style = end_style },
        }
    );
    // clang-format on
}

auto StyledStringView::styled_line_parts() const -> std::vector<std::vector<StyledStringPart>> {
    std::vector<std::vector<StyledStringPart>> lines;
    // Find the '\n' and split the content_ into multiple lines. We don't use std::ranges::split
    // because we need to preserve the '\n' characters.
    for (std::size_t start = 0; start != content_.size();) {
        std::size_t const pos = content_.find('\n', start);
        if (pos == std::string_view::npos) {
            // We cannot find '\n', so we add the rest of the string as a new line to the result and
            // exit the loop.
            lines.push_back({
                { .content = content_.substr(start), .style {} }
            });
            break;
        } else {
            lines.push_back({
                // Note that the substring contains newline characters here, which will be removed
                // in subsequent operations.
                { .content = content_.substr(start, pos - start + 1), .style {} }
            });
            start = pos + 1;
        }
    }

    // Now we need to further split each line into substrings consisting of consecutive characters
    // of the same style. We process line by line, and for each line, we always set the unprocessed
    // string at the last element of the vector.

    // The index of the line we are currently processing.
    std::size_t cur_line_index = 0;
    for (std::size_t part_index = 0; part_index != styled_parts_.size() - 1; ++part_index) {
        // We must use two adjacent StyledPart objects to determine a substring, as stated in the
        // documentation comment for the StyledPart class.
        std::size_t part_beg = styled_parts_[part_index].start_index;
        std::size_t const part_end = styled_parts_[part_index + 1].start_index;
        Style const part_style = styled_parts_[part_index].style;

        // The current part covers the entire unprocessed string of this line, so we process the
        // remaining string and move to the next line.
        for (; cur_line_index != lines.size()
             && part_end - part_beg >= lines[cur_line_index].back().content.size();
             ++cur_line_index) {
            StyledStringPart& unprocessed_part = lines[cur_line_index].back();
            std::string_view& unprocessed_content = unprocessed_part.content;

            // Update the style of the unprocessed part.
            unprocessed_part.style = part_style;

            // We must update part_beg here, because we may modify unprocessed_content later.
            part_beg += unprocessed_content.size();

            // Remove '\n' and '\r\n'.
            if (!unprocessed_content.empty() && unprocessed_content.back() == '\n') {
                unprocessed_content = unprocessed_content.substr(0, unprocessed_content.size() - 1);

                if (!unprocessed_content.empty() && unprocessed_content.back() == '\r') {
                    unprocessed_content =
                        unprocessed_content.substr(0, unprocessed_content.size() - 1);
                }
            }

            // When a style ends at the position of a newline character, an extra empty string may
            // be generated, which is unexpected.
            //
            // Fox example, we have a string "abc\n" of style Style::Auto and set the style of its
            // substring "abc" as Style::Highlight. During processing, we will pack substring "abc"
            // and its style Style::Highlight into a single StyledStringPart, and leave the string
            // "\n" to be processed. When we process "\n", we will insert an empty string with
            // Style::Auto into the result and generate unexpected result: { { "abc",
            // Style::Highlight }, { "", Style::Auto } }. So we should remove the empty string here.
            //
            // Note that if a line is empty, we keep it. So we remove the unneeded elements at the
            // end only if lines[cur_line_index].size() > 1 (which means the line is not empty).
            if (unprocessed_content.empty() && lines[cur_line_index].size() > 1) {
                lines[cur_line_index].pop_back();
            }
        }

        // The current style ends exactly at the end of this line, so we continue to process the
        // next line.
        if (part_beg == part_end) {
            continue;
        }

        // The current style ends in the middle of the string to be processed. We split the string
        // into two parts and insert the latter part as a new unprocessed string into the end of the
        // container.
        StyledStringPart& old_part = lines[cur_line_index].back();
        old_part.style = part_style;

        std::string_view const rest_content =
            std::exchange(old_part.content, old_part.content.substr(0, part_end - part_beg))
                .substr(part_end - part_beg);
        lines[cur_line_index].push_back(StyledStringPart { .content = rest_content, .style {} });
    }

    return lines;
}
}  // namespace ants
