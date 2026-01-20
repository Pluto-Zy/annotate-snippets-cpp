#include "annotate_snippets/source_file.hpp"
#include "annotate_snippets/source_location.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace ants {
inline namespace {
// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(SourceLocation const& loc, std::ostream* os) {
    *os << "SourceLocation(line=" << loc.line() << ", col=" << loc.col()
        << ", byte_offset=" << loc.byte_offset() << ")";
}

TEST(SourceFileTest, Basic) {
    {
        SourceFile const source;
        EXPECT_FALSE(source.valid());
        EXPECT_TRUE(source.invalid());
    }

    {
        SourceFile const source("abc");
        EXPECT_TRUE(source.valid());
        EXPECT_FALSE(source.invalid());
        EXPECT_EQ(source.name(), "");
        EXPECT_EQ(source.content(), "abc");
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source.first_line_number(), 1u);
    }

    {
        SourceFile const source("abc", "name");
        EXPECT_TRUE(source.valid());
        EXPECT_FALSE(source.invalid());
        EXPECT_EQ(source.name(), "name");
        EXPECT_EQ(source.content(), "abc");
        EXPECT_EQ(source.size(), 3u);
        EXPECT_EQ(source.first_line_number(), 1u);
    }

    {
        SourceFile source("abc", "name");
        source.set_name("new_name");
        source.set_first_line_number(5);
        EXPECT_EQ(source.name(), "new_name");
        EXPECT_EQ(source.first_line_number(), 5u);
    }

    {
        auto const source =
            SourceFile("abc", "name").with_name("chained_name").with_first_line_number(10);
        EXPECT_EQ(source.name(), "chained_name");
        EXPECT_EQ(source.first_line_number(), 10u);
    }
}

TEST(SourceFileDeathTest, InvalidSourceFileAccess) {
    {
        SourceFile const source;
        EXPECT_DEBUG_DEATH(source.name(), "");
        EXPECT_DEBUG_DEATH(source.content(), "");
        EXPECT_DEBUG_DEATH(source.size(), "");
        EXPECT_DEBUG_DEATH(source.first_line_number(), "");
    }

    {
        SourceFile src1("abc", "name");
        SourceFile const src = std::move(src1);
        EXPECT_FALSE(src1.valid());  // NOLINT(bugprone-use-after-move)
        EXPECT_TRUE(src.valid());
        EXPECT_DEBUG_DEATH(src1.name(), "");
        EXPECT_DEBUG_DEATH(src1.content(), "");
        EXPECT_DEBUG_DEATH(src1.size(), "");
        EXPECT_DEBUG_DEATH(src1.first_line_number(), "");
    }
}

TEST(SourceFileTest, BuildCache) {
#define EXPECT_CACHE(...)                                                                          \
    EXPECT_EQ(source.cached_lines(), (std::vector<std::size_t> { __VA_ARGS__ }))

    {
        SourceFile source("abc\nxyz\r\n123");
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines();
        EXPECT_CACHE(0, 4, 9, 13);
        EXPECT_TRUE(source.is_fully_cached());
    }

    {
        SourceFile source("abc");
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines();
        EXPECT_CACHE(0, 4);
        EXPECT_TRUE(source.is_fully_cached());
    }

    {
        SourceFile source("abc\n");
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines();
        EXPECT_CACHE(0, 4);
        EXPECT_TRUE(source.is_fully_cached());
    }

    {
        SourceFile source("");
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines();
        EXPECT_CACHE(0, 1);
        EXPECT_TRUE(source.is_fully_cached());
    }

    {
        SourceFile source("\n");
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines();
        EXPECT_CACHE(0, 1);
        EXPECT_TRUE(source.is_fully_cached());
    }

    {
        SourceFile source("\r\n");
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines();
        EXPECT_CACHE(0, 1);
        EXPECT_TRUE(source.is_fully_cached());
    }

    {
        SourceFile source("a\nb\nc\nde");
        source.build_lines_to_offset(0);
        EXPECT_CACHE(0);
        source.build_lines_to_offset(1);
        EXPECT_CACHE(0, 2);
        source.build_lines_to_offset(2);
        EXPECT_CACHE(0, 2);
        source.build_lines_to_offset(3);
        EXPECT_CACHE(0, 2, 4);
        source.build_lines_to_offset(3);
        EXPECT_CACHE(0, 2, 4);
        source.build_lines_to_offset(0);
        EXPECT_CACHE(0, 2, 4);
        source.build_lines_to_offset(6);
        EXPECT_CACHE(0, 2, 4, 6);
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines_to_offset(7);
        EXPECT_CACHE(0, 2, 4, 6, 9);
        EXPECT_TRUE(source.is_fully_cached());
        source.build_lines_to_offset(9);
        EXPECT_CACHE(0, 2, 4, 6, 9);
        source.build_lines_to_offset(100);
        EXPECT_CACHE(0, 2, 4, 6, 9);
    }

    {
        for (unsigned const i : { 4, 5, 6, 100 }) {
            SourceFile source("ab\nc");
            source.build_lines_to_offset(i);
            EXPECT_CACHE(0, 3, 5) << " when building to offset " << i;
        }
    }

    {
        SourceFile source("a\nb\nc\nde");
        source.build_lines_to_line(0);
        EXPECT_CACHE(0);
        source.build_lines_to_line(1);
        EXPECT_CACHE(0, 2);
        source.build_lines_to_line(1);
        EXPECT_CACHE(0, 2);
        source.build_lines_to_line(2);
        EXPECT_CACHE(0, 2, 4);
        source.build_lines_to_line(2);
        EXPECT_CACHE(0, 2, 4);
        source.build_lines_to_line(0);
        EXPECT_CACHE(0, 2, 4);
        source.build_lines_to_line(3);
        EXPECT_CACHE(0, 2, 4, 6);
        EXPECT_FALSE(source.is_fully_cached());
        source.build_lines_to_line(4);
        EXPECT_CACHE(0, 2, 4, 6, 9);
        EXPECT_TRUE(source.is_fully_cached());
        source.build_lines_to_line(5);
        EXPECT_CACHE(0, 2, 4, 6, 9);
        source.build_lines_to_line(100);
        EXPECT_CACHE(0, 2, 4, 6, 9);
    }

    {
        for (unsigned const i : { 2, 3, 4, 100 }) {
            SourceFile source("ab\nc");
            source.build_lines_to_line(i);
            EXPECT_CACHE(0, 3, 5) << " when building to line " << i;
        }
    }

#undef EXPECT_CACHE
}

TEST(SourceFileTest, LineCount) {
#define EXPECT_LINE_COUNT(expected)                                                                \
    EXPECT_EQ(source.total_line_count(), expected);                                                \
    EXPECT_EQ(std::as_const(source).total_line_count(), expected)

    {
        SourceFile source("a\nb\nc\nde");
        EXPECT_LINE_COUNT(5u);
    }

    {
        SourceFile source("abc");
        EXPECT_LINE_COUNT(2u);
    }

    {
        SourceFile source("abc\n");
        EXPECT_LINE_COUNT(2u);
    }

    {
        SourceFile source("");
        EXPECT_LINE_COUNT(2u);
    }

    {
        SourceFile source("\n");
        EXPECT_LINE_COUNT(2u);
    }

    {
        SourceFile source("\r\n");
        EXPECT_LINE_COUNT(2u);
    }

    {
        SourceFile source("\r\n");
        EXPECT_LINE_COUNT(2u);
    }

    {
        SourceFile source("\n\n\n");
        EXPECT_LINE_COUNT(4u);
    }

#undef EXPECT_LINE_COUNT
}

TEST(SourceFileTest, LineOffset) {
#define EXPECT_LINE_OFFSET(line, expected_offset)                                                  \
    EXPECT_EQ(source.line_offset(line), expected_offset);                                          \
    EXPECT_EQ(std::as_const(source).line_offset(line), expected_offset)

    {
        SourceFile source("a\nb\nc\nde");
        EXPECT_LINE_OFFSET(0, 0u);
        EXPECT_LINE_OFFSET(1, 2u);
        EXPECT_LINE_OFFSET(2, 4u);
        EXPECT_LINE_OFFSET(3, 6u);
        EXPECT_LINE_OFFSET(4, 9u);
        EXPECT_LINE_OFFSET(5, 9u);
        EXPECT_LINE_OFFSET(10, 9u);
    }

    {
        SourceFile source("a\nb\nc\nde");
        EXPECT_LINE_OFFSET(3, 6u);
        EXPECT_LINE_OFFSET(0, 0u);
        EXPECT_LINE_OFFSET(2, 4u);
        EXPECT_LINE_OFFSET(5, 9u);
        EXPECT_LINE_OFFSET(10, 9u);
        EXPECT_LINE_OFFSET(1, 2u);
        EXPECT_LINE_OFFSET(4, 9u);
    }

    {
        SourceFile source("abc");
        EXPECT_LINE_OFFSET(0, 0u);
        EXPECT_LINE_OFFSET(1, 4u);
        EXPECT_LINE_OFFSET(2, 4u);
        EXPECT_LINE_OFFSET(10, 4u);
    }

    {
        SourceFile source("abc");
        EXPECT_LINE_OFFSET(10, 4u);
        EXPECT_LINE_OFFSET(2, 4u);
        EXPECT_LINE_OFFSET(0, 0u);
        EXPECT_LINE_OFFSET(1, 4u);
    }

#undef EXPECT_LINE_OFFSET
}

TEST(SourceFileTest, LineContent) {
#define EXPECT_LINE_CONTENT(line, expected_content)                                                \
    EXPECT_EQ(source.line_content(line), expected_content);                                        \
    EXPECT_EQ(std::as_const(source).line_content(line), expected_content)

    {
        SourceFile source("a\nb\nc\r\nde");
        EXPECT_LINE_CONTENT(0, "a");
        EXPECT_LINE_CONTENT(1, "b");
        EXPECT_LINE_CONTENT(2, "c");
        EXPECT_LINE_CONTENT(3, "de");
        EXPECT_LINE_CONTENT(4, "");
        EXPECT_LINE_CONTENT(10, "");
    }

    {
        SourceFile source("");
        EXPECT_LINE_CONTENT(0, "");
        EXPECT_LINE_CONTENT(1, "");
        EXPECT_LINE_CONTENT(2, "");
    }

#undef EXPECT_LINE_CONTENT
}

TEST(SourceFileTest, ByteOffsetToLineCol) {
#define EXPECT_SOURCE_LOCATION(byte_offset, line, col)                                             \
    EXPECT_EQ(source.byte_offset_to_line_col(byte_offset), std::make_pair(line, col));             \
    EXPECT_EQ(std::as_const(source).byte_offset_to_line_col(byte_offset), std::make_pair(line, col))

    {
        SourceFile source("ab\ncd\ne\nf");
        EXPECT_SOURCE_LOCATION(0, 0u, 0u);
        EXPECT_SOURCE_LOCATION(1, 0u, 1u);
        EXPECT_SOURCE_LOCATION(2, 0u, 2u);
        EXPECT_SOURCE_LOCATION(3, 1u, 0u);
        EXPECT_SOURCE_LOCATION(4, 1u, 1u);
        EXPECT_SOURCE_LOCATION(5, 1u, 2u);
        EXPECT_SOURCE_LOCATION(8, 3u, 0u);
        EXPECT_SOURCE_LOCATION(9, 3u, 1u);
        EXPECT_SOURCE_LOCATION(10, 4u, 0u);
        EXPECT_SOURCE_LOCATION(11, 4u, 1u);
        EXPECT_SOURCE_LOCATION(12, 4u, 2u);
    }

    {
        SourceFile source("ab\ncd\ne\nf");
        EXPECT_SOURCE_LOCATION(14, 4u, 4u);
        EXPECT_SOURCE_LOCATION(10, 4u, 0u);
        EXPECT_SOURCE_LOCATION(9, 3u, 1u);
        EXPECT_SOURCE_LOCATION(8, 3u, 0u);
        EXPECT_SOURCE_LOCATION(5, 1u, 2u);
        EXPECT_SOURCE_LOCATION(4, 1u, 1u);
        EXPECT_SOURCE_LOCATION(3, 1u, 0u);
        EXPECT_SOURCE_LOCATION(2, 0u, 2u);
        EXPECT_SOURCE_LOCATION(1, 0u, 1u);
        EXPECT_SOURCE_LOCATION(0, 0u, 0u);
    }

    {
        SourceFile source("ab\ncd\ne\nf");
        EXPECT_SOURCE_LOCATION(8, 3u, 0u);
        EXPECT_SOURCE_LOCATION(3, 1u, 0u);
        EXPECT_SOURCE_LOCATION(0, 0u, 0u);
        EXPECT_SOURCE_LOCATION(9, 3u, 1u);
        EXPECT_SOURCE_LOCATION(4, 1u, 1u);
        EXPECT_SOURCE_LOCATION(2, 0u, 2u);
        EXPECT_SOURCE_LOCATION(10, 4u, 0u);
        EXPECT_SOURCE_LOCATION(14, 4u, 4u);
        EXPECT_SOURCE_LOCATION(5, 1u, 2u);
        EXPECT_SOURCE_LOCATION(1, 0u, 1u);
    }

    {
        SourceFile source("ab\ncd\ne\nf");
        EXPECT_SOURCE_LOCATION(10, 4u, 0u);
        EXPECT_SOURCE_LOCATION(5, 1u, 2u);
        EXPECT_SOURCE_LOCATION(1, 0u, 1u);
        EXPECT_SOURCE_LOCATION(0, 0u, 0u);
    }

    {
        SourceFile source("");
        EXPECT_SOURCE_LOCATION(0, 0u, 0u);
        EXPECT_SOURCE_LOCATION(1, 1u, 0u);
        EXPECT_SOURCE_LOCATION(2, 1u, 1u);
        EXPECT_SOURCE_LOCATION(3, 1u, 2u);
    }

#undef EXPECT_SOURCE_LOCATION
}

TEST(SourceFileTest, LineColToByteOffset) {
#define EXPECT_SOURCE_LOCATION(line, col, expected_byte_offset)                                    \
    EXPECT_EQ(source.line_col_to_byte_offset(line, col), expected_byte_offset);                    \
    EXPECT_EQ(std::as_const(source).line_col_to_byte_offset(line, col), expected_byte_offset)

    {
        SourceFile source("ab\ncd\ne\nf");
        EXPECT_SOURCE_LOCATION(0, 0, 0u);
        EXPECT_SOURCE_LOCATION(0, 1, 1u);
        EXPECT_SOURCE_LOCATION(0, 2, 2u);
        EXPECT_SOURCE_LOCATION(0, 3, 3u);
        EXPECT_SOURCE_LOCATION(0, 4, 4u);
        EXPECT_SOURCE_LOCATION(1, 0, 3u);
        EXPECT_SOURCE_LOCATION(1, 1, 4u);
        EXPECT_SOURCE_LOCATION(1, 2, 5u);
        EXPECT_SOURCE_LOCATION(1, 3, 6u);
        EXPECT_SOURCE_LOCATION(3, 0, 8u);
        EXPECT_SOURCE_LOCATION(3, 1, 9u);
        EXPECT_SOURCE_LOCATION(4, 0, 10u);
        EXPECT_SOURCE_LOCATION(4, 1, 11u);
        EXPECT_SOURCE_LOCATION(4, 2, 12u);
        EXPECT_SOURCE_LOCATION(5, 0, 10u);
        EXPECT_SOURCE_LOCATION(5, 1, 11u);
        EXPECT_SOURCE_LOCATION(5, 2, 12u);
        EXPECT_SOURCE_LOCATION(10, 0, 10u);
    }

    {
        SourceFile source("ab\ncd\ne\nf");
        EXPECT_SOURCE_LOCATION(10, 0, 10u);
        EXPECT_SOURCE_LOCATION(5, 2, 12u);
        EXPECT_SOURCE_LOCATION(1, 2, 5u);
        EXPECT_SOURCE_LOCATION(0, 2, 2u);
    }

    {
        SourceFile source("");
        EXPECT_SOURCE_LOCATION(0, 0, 0u);
        EXPECT_SOURCE_LOCATION(0, 1, 1u);
        EXPECT_SOURCE_LOCATION(0, 2, 2u);
        EXPECT_SOURCE_LOCATION(0, 3, 3u);
        EXPECT_SOURCE_LOCATION(1, 0, 1u);
        EXPECT_SOURCE_LOCATION(1, 1, 2u);
        EXPECT_SOURCE_LOCATION(1, 2, 3u);
        EXPECT_SOURCE_LOCATION(10, 0, 1u);
    }

#undef EXPECT_SOURCE_LOCATION
}

void check_fill_location(SourceFile& source, std::size_t byte_offset, unsigned line, unsigned col) {
    // Construct message for debugging.
    std::string const message =
        "The function is called with byte_offset=" + std::to_string(byte_offset)
        + ", line=" + std::to_string(line) + ", col=" + std::to_string(col) + '.';
    SCOPED_TRACE(message);

    {
        SourceLocation const loc = source.fill_source_location(SourceLocation(line, col));
        EXPECT_TRUE(loc.is_fully_specified());
        EXPECT_EQ(loc.line(), line);
        EXPECT_EQ(loc.col(), col);
        EXPECT_EQ(loc.byte_offset(), byte_offset);
    }

    {
        SourceLocation const loc = source.fill_source_location(SourceLocation(byte_offset));
        EXPECT_TRUE(loc.is_fully_specified());
        EXPECT_EQ(loc.line(), line);
        EXPECT_EQ(loc.col(), col);
        EXPECT_EQ(loc.byte_offset(), byte_offset);
    }
}

TEST(SourceFileTest, FillSourceLocation) {
    {
        SCOPED_TRACE("The `SourceFile` is defined in this scope.");
        SourceFile source("ab\ncd\ne\nf");
        check_fill_location(source, 0, 0, 0);
        check_fill_location(source, 1, 0, 1);
        check_fill_location(source, 2, 0, 2);
        check_fill_location(source, 3, 1, 0);
        check_fill_location(source, 4, 1, 1);
        check_fill_location(source, 5, 1, 2);
        check_fill_location(source, 8, 3, 0);
        check_fill_location(source, 9, 3, 1);
        check_fill_location(source, 10, 4, 0);
        check_fill_location(source, 11, 4, 1);
        check_fill_location(source, 12, 4, 2);
    }

    {
        SCOPED_TRACE("The `SourceFile` is defined in this scope.");
        SourceFile source("ab\ncd\ne\nf");
        check_fill_location(source, 14, 4, 4);
        check_fill_location(source, 10, 4, 0);
        check_fill_location(source, 9, 3, 1);
        check_fill_location(source, 8, 3, 0);
        check_fill_location(source, 5, 1, 2);
        check_fill_location(source, 4, 1, 1);
        check_fill_location(source, 3, 1, 0);
        check_fill_location(source, 2, 0, 2);
        check_fill_location(source, 1, 0, 1);
        check_fill_location(source, 0, 0, 0);
    }

    {
        SCOPED_TRACE("The `SourceFile` is defined in this scope.");
        SourceFile source("");
        check_fill_location(source, 0, 0, 0);
        check_fill_location(source, 1, 1, 0);
        check_fill_location(source, 2, 1, 1);
        check_fill_location(source, 3, 1, 2);
    }
}

TEST(SourceFileDeathTest, FillCompleteLocation) {
    SourceFile source("");
    SourceLocation const complete_loc = source.fill_source_location(SourceLocation(0, 0));
    ASSERT_TRUE(complete_loc.is_fully_specified());
    // Attempting to fill a `SourceLocation` that is already complete should trigger an assertion.
    EXPECT_DEBUG_DEATH(static_cast<void>(source.fill_source_location(complete_loc)), "");
}

void check_normalize_location(
    SourceFile& source,
    unsigned input_line,
    unsigned input_col,
    unsigned expected_line,
    unsigned expected_col
) {
    // Construct message for debugging.
    std::string const message = "The function is called with input_line="
        + std::to_string(input_line) + ", input_col=" + std::to_string(input_col) + '.';
    SCOPED_TRACE(message);

    SourceLocation const loc = source.normalize_location(SourceLocation(input_line, input_col));
    EXPECT_TRUE(loc.is_fully_specified());
    EXPECT_EQ(loc.line(), expected_line);
    EXPECT_EQ(loc.col(), expected_col);

    // Use the fully specified location to test again to check that no assertion is triggered.
    SourceLocation const loc2 = source.normalize_location(loc);
    EXPECT_TRUE(loc2.is_fully_specified());
    EXPECT_EQ(loc2.line(), expected_line);
    EXPECT_EQ(loc2.col(), expected_col);
}

TEST(SourceFileTest, NormalizeLocation) {
    {
        SCOPED_TRACE("The `SourceFile` is defined in this scope.");
        SourceFile source("ab\ncd\ne");
        check_normalize_location(source, 0, 0, 0, 0);
        check_normalize_location(source, 1, 1, 1, 1);
        check_normalize_location(source, 0, 2, 0, 2);
        check_normalize_location(source, 0, 3, 1, 0);
        check_normalize_location(source, 0, 4, 1, 0);
        check_normalize_location(source, 1, 2, 1, 2);
        check_normalize_location(source, 1, 3, 2, 0);
        check_normalize_location(source, 1, 4, 2, 0);
        check_normalize_location(source, 2, 0, 2, 0);
        check_normalize_location(source, 2, 1, 2, 1);
        check_normalize_location(source, 2, 2, 3, 0);
        check_normalize_location(source, 3, 0, 3, 0);
        check_normalize_location(source, 4, 2, 3, 0);
    }

    {
        SCOPED_TRACE("The `SourceFile` is defined in this scope.");
        SourceFile source("ab\ncd\ne");
        check_normalize_location(source, 4, 2, 3, 0);
    }

    {
        SCOPED_TRACE("The `SourceFile` is defined in this scope.");
        SourceFile source("");
        check_normalize_location(source, 0, 0, 0, 0);
        check_normalize_location(source, 1, 0, 1, 0);
        check_normalize_location(source, 0, 1, 1, 0);
    }
}

void check_adjust_empty_span(SourceFile& source, SourceLocation loc) {
    // Construct message for debugging.
    std::string const message = "The function is called with SourceLocation(line="
        + std::to_string(loc.line()) + ", col=" + std::to_string(loc.col()) + ").";
    SCOPED_TRACE(message);

    // Call `adjust_span()` to adjust the empty span.
    LabeledSpan const span = source.adjust_span(LabeledSpan { loc, loc, {} });
    EXPECT_TRUE(span.beg.is_fully_specified());
    EXPECT_TRUE(span.end.is_fully_specified());
    EXPECT_EQ(span.beg.line(), loc.line());
    EXPECT_EQ(span.beg.col(), loc.col());
    EXPECT_EQ(span.end.line(), loc.line());
    EXPECT_EQ(span.end.col(), loc.col() + 1);

    // Use the fully specified location to test again to check that no assertion is triggered.
    LabeledSpan const span2 = source.adjust_span(LabeledSpan { span.beg, span.beg, {} });
    EXPECT_TRUE(span2.beg.is_fully_specified());
    EXPECT_TRUE(span2.end.is_fully_specified());
    EXPECT_EQ(span2.beg, span.beg);
    EXPECT_EQ(span2.end, span.end);

    // Test `adjust_span()` won't modify non-empty spans.
    LabeledSpan const span3 = source.adjust_span(span);
    EXPECT_EQ(span3.beg, span.beg);
    EXPECT_EQ(span3.end, span.end);
}

void check_adjust_span_end(SourceFile& source, SourceLocation end, SourceLocation expected_end) {
    // Construct message for debugging.
    std::string const message = "The function is called with end SourceLocation(line="
        + std::to_string(end.line()) + ", col=" + std::to_string(end.col()) + ").";
    SCOPED_TRACE(message);

    // Call `adjust_span()` to adjust the span with only `end` specified.
    LabeledSpan const span = source.adjust_span(LabeledSpan { SourceLocation(0, 0), end, {} });
    EXPECT_TRUE(span.beg.is_fully_specified());
    EXPECT_TRUE(span.end.is_fully_specified());
    EXPECT_EQ(span.end.line(), expected_end.line());
    EXPECT_EQ(span.end.col(), expected_end.col());

    // Use the fully specified location to test again to check that no assertion is triggered.
    LabeledSpan const span2 = source.adjust_span(LabeledSpan { span.beg, span.end, {} });
    EXPECT_TRUE(span2.beg.is_fully_specified());
    EXPECT_TRUE(span2.end.is_fully_specified());
    EXPECT_EQ(span2.beg, span.beg);
    EXPECT_EQ(span2.end, span.end);
}

TEST(SourceFileTest, AdjustSpan) {
    {
        SourceFile source("a\n");
        check_adjust_empty_span(source, SourceLocation(0, 0));
        check_adjust_empty_span(source, SourceLocation(0, 1));
        check_adjust_empty_span(source, SourceLocation(1, 0));
    }

    {
        SourceFile source("ab\ncd\ne");
        check_adjust_span_end(source, SourceLocation(1, 0), SourceLocation(0, 3));
        check_adjust_span_end(source, SourceLocation(1, 1), SourceLocation(1, 1));
        check_adjust_span_end(source, SourceLocation(2, 0), SourceLocation(1, 3));
        check_adjust_span_end(source, SourceLocation(2, 1), SourceLocation(2, 1));
        check_adjust_span_end(source, SourceLocation(3, 0), SourceLocation(2, 2));
        check_adjust_span_end(source, SourceLocation(3, 1), SourceLocation(3, 1));
    }
}
}  // namespace
}  // namespace ants
