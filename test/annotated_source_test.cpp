#include "annotate_snippets/annotated_source.hpp"

#include "gtest/gtest.h"

#include <type_traits>
#include <vector>

namespace {
TEST(AnnotatedSourceTest, LineOffset) {
#define TEST_CASE_IMPL(...)                                                                        \
    ants::AnnotatedSource as(source);                                                              \
    for (unsigned const line : { __VA_ARGS__ }) {                                                  \
        if (line < line_starts.size()) {                                                           \
            EXPECT_EQ(as.line_offset(line), line_starts[line]);                                    \
        } else {                                                                                   \
            EXPECT_EQ(as.line_offset(line), line_starts.back());                                   \
        }                                                                                          \
    }

#define TEST_CASE(...) { TEST_CASE_IMPL(__VA_ARGS__) }

#define TEST_CASE_CHECK_CACHE(...)                                                                 \
    {                                                                                              \
        TEST_CASE_IMPL(__VA_ARGS__)                                                                \
        std::decay_t<decltype(as.line_offsets_cache())> expected_cache;                            \
        for (unsigned i = 0; i != static_cast<unsigned>(line_starts.size()); ++i) {                \
            expected_cache.emplace(i, line_starts[i]);                                             \
        }                                                                                          \
        EXPECT_EQ(expected_cache, as.line_offsets_cache());                                        \
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        std::vector<unsigned> const line_starts { 0, 3, 6, 8, 10 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 5, 6, 100)
        TEST_CASE_CHECK_CACHE(100, 6, 5, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(2, 0, 3, 4, 1)
        TEST_CASE(0, 2, 3)
        TEST_CASE(0, 2, 4)
        TEST_CASE(0, 3, 2)
        TEST_CASE(0, 4, 2)
        TEST_CASE(1, 3, 2, 0)
        TEST_CASE(1, 4, 2, 0)
        TEST_CASE(0, 3, 4)
        TEST_CASE_CHECK_CACHE(100, 5, 3, 1, 4, 2, 0)
        TEST_CASE(4, 1, 0)
        TEST_CASE(4, 3, 1)
        TEST_CASE(3, 1, 0)
        TEST_CASE(3, 2, 1)
        TEST_CASE(3, 0, 2)
        TEST_CASE(4, 0, 2)
        TEST_CASE(3, 1, 2, 0)
        TEST_CASE(4, 1, 2, 0)
    }

    {
        const char* const source = "abc";
        std::vector<unsigned> const line_starts { 0, 4 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 100)
        TEST_CASE_CHECK_CACHE(100, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(3, 2, 4, 1, 0)
    }

    {
        const char* const source = "abc\n";
        std::vector<unsigned> const line_starts { 0, 4 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 100)
        TEST_CASE_CHECK_CACHE(100, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(3, 2, 4, 1, 0)
        TEST_CASE(1, 0)
    }

    {
        const char* const source = "a";
        std::vector<unsigned> const line_starts { 0, 2 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 100)
        TEST_CASE_CHECK_CACHE(100, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(3, 2, 4, 1, 0)
        TEST_CASE(1, 0)
    }

    {
        const char* const source = "a\n";
        std::vector<unsigned> const line_starts { 0, 2 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 100)
        TEST_CASE_CHECK_CACHE(100, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(3, 2, 4, 1, 0)
        TEST_CASE(1, 0)
    }

    {
        const char* const source = "\n";
        std::vector<unsigned> const line_starts { 0, 1 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 100)
        TEST_CASE_CHECK_CACHE(100, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(3, 2, 4, 1, 0)
        TEST_CASE(1, 0)
    }

    {
        const char* const source = "\n\n\n\n\n";
        std::vector<unsigned> const line_starts { 0, 1, 2, 3, 4, 5 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 5, 6, 7, 100)
        TEST_CASE_CHECK_CACHE(100, 7, 6, 5, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(5, 4, 3, 2, 1, 0)
        TEST_CASE(1, 3, 5, 7)
        TEST_CASE(7, 5, 3, 1)
        TEST_CASE(0, 2, 4, 6)
        TEST_CASE(6, 4, 2, 0)
        TEST_CASE(0, 3, 6, 100)
        TEST_CASE(100, 6, 3, 0)
        TEST_CASE(1, 4, 7)
        TEST_CASE(7, 4, 1)
        TEST_CASE(0, 3, 6)
        TEST_CASE(6, 3, 0)
        TEST_CASE(0, 4)
        TEST_CASE(4, 0)
        TEST_CASE(1, 5)
        TEST_CASE(5, 1)
        TEST_CASE(2, 6)
        TEST_CASE(6, 2)
    }

    {
        const char* const source = "\n\n\n\n\na";
        std::vector<unsigned> const line_starts { 0, 1, 2, 3, 4, 5, 7 };

        TEST_CASE_CHECK_CACHE(0, 1, 2, 3, 4, 5, 6, 7, 100)
        TEST_CASE_CHECK_CACHE(100, 7, 6, 5, 4, 3, 2, 1, 0)
        TEST_CASE_CHECK_CACHE(6, 5, 4, 3, 2, 1, 0)
        TEST_CASE(1, 3, 5, 7)
        TEST_CASE(7, 5, 3, 1)
        TEST_CASE(0, 2, 4, 6)
        TEST_CASE(6, 4, 2, 0)
        TEST_CASE(0, 3, 6, 100)
        TEST_CASE(100, 6, 3, 0)
        TEST_CASE(1, 4, 7)
        TEST_CASE(7, 4, 1)
        TEST_CASE(0, 3, 6)
        TEST_CASE(6, 3, 0)
        TEST_CASE(0, 4)
        TEST_CASE(4, 0)
        TEST_CASE(1, 5)
        TEST_CASE(5, 1)
        TEST_CASE(2, 6)
        TEST_CASE(6, 2)
    }

    {
        const char* const source = "";
        std::vector<unsigned> const line_starts { 0, 1 };

        TEST_CASE_CHECK_CACHE(0, 1, 2)
        TEST_CASE_CHECK_CACHE(100, 2, 1, 0)
        TEST_CASE(0)
    }

#undef TEST_CASE_CHECK_CACHE
#undef TEST_CASE
#undef TEST_CASE_IMPL
}

TEST(AnnotatedSourceTest, ByteOffsetToLineCol) {
#define CHECK_SOURCE_LOCATION(byte, line, col)                                                     \
    EXPECT_EQ(as.byte_offset_to_line_col(byte), (ants::SourceLocation { line, col }))

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(3, 1, 0);
        CHECK_SOURCE_LOCATION(4, 1, 1);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(9, 3, 1);
        CHECK_SOURCE_LOCATION(10, 4, 0);
        CHECK_SOURCE_LOCATION(14, 4, 4);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(14, 4, 4);
        CHECK_SOURCE_LOCATION(10, 4, 0);
        CHECK_SOURCE_LOCATION(9, 3, 1);
        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(4, 1, 1);
        CHECK_SOURCE_LOCATION(3, 1, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(14, 4, 4);
        CHECK_SOURCE_LOCATION(10, 4, 0);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(4, 1, 1);
        CHECK_SOURCE_LOCATION(9, 3, 1);
        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(3, 1, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(14, 4, 4);
        CHECK_SOURCE_LOCATION(7, 2, 1);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(2, 0, 2);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(3, 1, 0);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        // Add cache by `line_offset()`.
        as.line_offset(100);

        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(3, 1, 0);
        CHECK_SOURCE_LOCATION(4, 1, 1);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(9, 3, 1);
        CHECK_SOURCE_LOCATION(10, 4, 0);
        CHECK_SOURCE_LOCATION(14, 4, 4);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        // Add cache by `line_offset()`.
        as.line_offset(100);

        CHECK_SOURCE_LOCATION(14, 4, 4);
        CHECK_SOURCE_LOCATION(10, 4, 0);
        CHECK_SOURCE_LOCATION(9, 3, 1);
        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(4, 1, 1);
        CHECK_SOURCE_LOCATION(3, 1, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "ab\ncd\ne\nf";
        ants::AnnotatedSource as(source);

        // Add cache by `line_offset()`.
        as.line_offset(4);

        CHECK_SOURCE_LOCATION(14, 4, 4);
        CHECK_SOURCE_LOCATION(10, 4, 0);
        CHECK_SOURCE_LOCATION(9, 3, 1);
        CHECK_SOURCE_LOCATION(8, 3, 0);
        CHECK_SOURCE_LOCATION(5, 1, 2);
        CHECK_SOURCE_LOCATION(4, 1, 1);
        CHECK_SOURCE_LOCATION(3, 1, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(1, 0, 1);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "abc";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(3, 0, 3);
        CHECK_SOURCE_LOCATION(4, 1, 0);
    }

    {
        const char* const source = "abc";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(4, 1, 0);
        CHECK_SOURCE_LOCATION(3, 0, 3);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "abc";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(1, 0, 1);
    }

    {
        const char* const source = "abc";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(4, 1, 0);
        CHECK_SOURCE_LOCATION(6, 1, 2);
        CHECK_SOURCE_LOCATION(5, 1, 1);
    }

    {
        const char* const source = "abc";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(1, 0, 1);
    }

    {
        const char* const source = "abc\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(4, 1, 0);
        CHECK_SOURCE_LOCATION(6, 1, 2);
    }

    {
        const char* const source = "abc\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "abc\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(6, 1, 2);
        CHECK_SOURCE_LOCATION(4, 1, 0);
        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(2, 0, 2);
    }

    {
        const char* const source = "abc\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(4, 1, 0);
        CHECK_SOURCE_LOCATION(6, 1, 2);
        CHECK_SOURCE_LOCATION(5, 1, 1);
    }

    {
        const char* const source = "abc\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(2, 0, 2);
        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(1, 0, 1);
    }

    {
        const char* const source = "\n\n\n\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(1, 1, 0);
        CHECK_SOURCE_LOCATION(2, 2, 0);
        CHECK_SOURCE_LOCATION(3, 3, 0);
        CHECK_SOURCE_LOCATION(4, 4, 0);
        CHECK_SOURCE_LOCATION(5, 4, 1);
        CHECK_SOURCE_LOCATION(6, 4, 2);
    }

    {
        const char* const source = "\n\n\n\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(6, 4, 2);
        CHECK_SOURCE_LOCATION(5, 4, 1);
        CHECK_SOURCE_LOCATION(4, 4, 0);
        CHECK_SOURCE_LOCATION(3, 3, 0);
        CHECK_SOURCE_LOCATION(2, 2, 0);
        CHECK_SOURCE_LOCATION(1, 1, 0);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

    {
        const char* const source = "\n\n\n\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(6, 4, 2);
        CHECK_SOURCE_LOCATION(1, 1, 0);
        CHECK_SOURCE_LOCATION(5, 4, 1);
        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(2, 2, 0);
        CHECK_SOURCE_LOCATION(4, 4, 0);
        CHECK_SOURCE_LOCATION(3, 3, 0);
    }

    {
        const char* const source = "\n\n\n\n";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(6, 4, 2);
        CHECK_SOURCE_LOCATION(7, 4, 3);
        CHECK_SOURCE_LOCATION(8, 4, 4);
    }

    {
        const char* const source = "";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(0, 0, 0);
        CHECK_SOURCE_LOCATION(1, 1, 0);
        CHECK_SOURCE_LOCATION(2, 1, 1);
    }

    {
        const char* const source = "";
        ants::AnnotatedSource as(source);

        CHECK_SOURCE_LOCATION(2, 1, 1);
        CHECK_SOURCE_LOCATION(1, 1, 0);
        CHECK_SOURCE_LOCATION(0, 0, 0);
    }

#undef CHECK_SOURCE_LOCATION
}

TEST(AnnotatedSourceTest, LineContent) {
    {
        ants::AnnotatedSource source("abc");
        EXPECT_EQ(source.line_content(0), "abc");
        EXPECT_EQ(source.line_content(1), "");
        EXPECT_EQ(source.line_content(2), "");
        EXPECT_EQ(source.line_content(100), "");
        EXPECT_EQ(source.line_content(0), "abc");
    }

    {
        ants::AnnotatedSource source("abc\n");
        EXPECT_EQ(source.line_content(0), "abc");
        EXPECT_EQ(source.line_content(1), "");
        EXPECT_EQ(source.line_content(2), "");
        EXPECT_EQ(source.line_content(3), "");
    }

    {
        ants::AnnotatedSource source("abc\r\n");
        EXPECT_EQ(source.line_content(0), "abc");
        EXPECT_EQ(source.line_content(1), "");
        EXPECT_EQ(source.line_content(2), "");
        EXPECT_EQ(source.line_content(3), "");
    }

    {
        ants::AnnotatedSource source("abc\r");
        EXPECT_EQ(source.line_content(0), "abc\r");
        EXPECT_EQ(source.line_content(1), "");
        EXPECT_EQ(source.line_content(2), "");
    }

    {
        ants::AnnotatedSource source("ab\ncd\r\ne\nf");
        EXPECT_EQ(source.line_content(0), "ab");
        EXPECT_EQ(source.line_content(3), "f");
        EXPECT_EQ(source.line_content(2), "e");
        EXPECT_EQ(source.line_content(1), "cd");
    }

    {
        ants::AnnotatedSource source("");
        EXPECT_EQ(source.line_content(0), "");
        EXPECT_EQ(source.line_content(1), "");
        EXPECT_EQ(source.line_content(2), "");
    }

    {
        ants::AnnotatedSource source("\r\n");
        EXPECT_EQ(source.line_content(3), "");
        EXPECT_EQ(source.line_content(2), "");
        EXPECT_EQ(source.line_content(1), "");
        EXPECT_EQ(source.line_content(0), "");
    }

    {
        ants::AnnotatedSource source("\r\n1\n2\n\n\n");
        EXPECT_EQ(source.line_content(0), "");
        EXPECT_EQ(source.line_content(1), "1");
        EXPECT_EQ(source.line_content(2), "2");
        EXPECT_EQ(source.line_content(3), "");
        EXPECT_EQ(source.line_content(4), "");
        EXPECT_EQ(source.line_content(5), "");
        EXPECT_EQ(source.line_content(6), "");
        EXPECT_EQ(source.line_content(7), "");
    }
}

TEST(AnnotatedSourceTest, NormalizeLocation) {
#define CHECK_BYTE_OFFSET(byte_offset, line, loc)                                                  \
    EXPECT_EQ(as.normalize_location(byte_offset), (ants::SourceLocation { line, loc }))
#define CHECK_LINE_COL(input_line, input_col, expected_line, expected_col)                         \
    EXPECT_EQ(                                                                                     \
        as.normalize_location(ants::SourceLocation { input_line, input_col }),                     \
        (ants::SourceLocation { expected_line, expected_col })                                     \
    )

    {
        ants::AnnotatedSource as("ab\ncd\ne");
        CHECK_BYTE_OFFSET(0, 0, 0);
        CHECK_BYTE_OFFSET(1, 0, 1);
        CHECK_BYTE_OFFSET(2, 0, 2);
        CHECK_BYTE_OFFSET(3, 1, 0);
        CHECK_BYTE_OFFSET(4, 1, 1);
        CHECK_BYTE_OFFSET(5, 1, 2);
        CHECK_BYTE_OFFSET(6, 2, 0);
        CHECK_BYTE_OFFSET(7, 2, 1);
        CHECK_BYTE_OFFSET(8, 3, 0);
        CHECK_BYTE_OFFSET(9, 3, 0);
        CHECK_BYTE_OFFSET(42, 3, 0);

        CHECK_LINE_COL(0, 0, 0, 0);
        CHECK_LINE_COL(1, 1, 1, 1);
        CHECK_LINE_COL(0, 2, 0, 2);
        CHECK_LINE_COL(0, 3, 1, 0);
        CHECK_LINE_COL(0, 4, 1, 0);
        CHECK_LINE_COL(1, 2, 1, 2);
        CHECK_LINE_COL(1, 3, 2, 0);
        CHECK_LINE_COL(1, 4, 2, 0);
        CHECK_LINE_COL(2, 0, 2, 0);
        CHECK_LINE_COL(2, 1, 2, 1);
        CHECK_LINE_COL(2, 2, 3, 0);
        CHECK_LINE_COL(3, 0, 3, 0);
        CHECK_LINE_COL(4, 2, 3, 0);
    }

    {
        ants::AnnotatedSource as("");
        CHECK_BYTE_OFFSET(0, 0, 0);
        CHECK_BYTE_OFFSET(1, 1, 0);
        CHECK_LINE_COL(0, 0, 0, 0);
        CHECK_LINE_COL(1, 0, 1, 0);
        CHECK_LINE_COL(0, 1, 1, 0);
    }

    {
        ants::AnnotatedSource as("abc");
        CHECK_BYTE_OFFSET(0, 0, 0);
        CHECK_BYTE_OFFSET(1, 0, 1);
        CHECK_BYTE_OFFSET(2, 0, 2);
        CHECK_BYTE_OFFSET(3, 0, 3);
        CHECK_BYTE_OFFSET(4, 1, 0);

        CHECK_LINE_COL(0, 0, 0, 0);
        CHECK_LINE_COL(0, 1, 0, 1);
        CHECK_LINE_COL(0, 2, 0, 2);
        CHECK_LINE_COL(0, 3, 0, 3);
        CHECK_LINE_COL(0, 4, 1, 0);
        CHECK_LINE_COL(1, 0, 1, 0);
        CHECK_LINE_COL(2, 0, 1, 0);
    }

    {
        ants::AnnotatedSource as("ab\n");
        CHECK_BYTE_OFFSET(0, 0, 0);
        CHECK_BYTE_OFFSET(1, 0, 1);
        CHECK_BYTE_OFFSET(2, 0, 2);
        CHECK_BYTE_OFFSET(3, 1, 0);
        CHECK_BYTE_OFFSET(4, 1, 0);

        CHECK_LINE_COL(0, 0, 0, 0);
        CHECK_LINE_COL(0, 1, 0, 1);
        CHECK_LINE_COL(0, 2, 0, 2);
        CHECK_LINE_COL(0, 3, 1, 0);
        CHECK_LINE_COL(0, 4, 1, 0);
        CHECK_LINE_COL(1, 0, 1, 0);
        CHECK_LINE_COL(2, 0, 1, 0);
    }

#undef CHECK_LINE_COL
#undef CHECK_BYTE_OFFSET
}
}  // namespace
