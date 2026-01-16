#include "annotate_snippets/source_location.hpp"

#include "gtest/gtest.h"

#include <cstddef>
#include <ostream>

namespace ants {
inline namespace {
// NOLINTNEXTLINE(readability-identifier-naming)
void PrintTo(SourceLocation const& loc, std::ostream* os) {
    *os << "SourceLocation(line=" << loc.line() << ", col=" << loc.col()
        << ", byte_offset=" << loc.byte_offset() << ")";
}

TEST(SourceLocationTest, ConstructionAndAccessors) {
    {
        SourceLocation const loc;
        EXPECT_EQ(loc.line(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.col(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.byte_offset(), static_cast<std::size_t>(-1));
    }

    {
        SourceLocation const loc(3, 5);
        EXPECT_EQ(loc.line(), 3u);
        EXPECT_EQ(loc.col(), 5u);
        EXPECT_EQ(loc.byte_offset(), static_cast<std::size_t>(-1));
    }

    {
        SourceLocation const loc(42u);
        EXPECT_EQ(loc.line(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.col(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.byte_offset(), 42u);
    }

    {
        SourceLocation const loc = SourceLocation::from_line_col(7, 9);
        EXPECT_EQ(loc.line(), 7u);
        EXPECT_EQ(loc.col(), 9u);
        EXPECT_EQ(loc.byte_offset(), static_cast<std::size_t>(-1));
    }

    {
        SourceLocation const loc = SourceLocation::from_byte_offset(99);
        EXPECT_EQ(loc.line(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.col(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.byte_offset(), 99u);
    }
}

TEST(SourceLocationTest, Setters) {
    {
        SourceLocation loc;
        loc.set_line(2);
        loc.set_col(4);
        EXPECT_EQ(loc.line(), 2u);
        EXPECT_EQ(loc.col(), 4u);
        EXPECT_EQ(loc.byte_offset(), static_cast<std::size_t>(-1));

        loc.set_line_col(6, 8);
        EXPECT_EQ(loc.line(), 6u);
        EXPECT_EQ(loc.col(), 8u);
        EXPECT_EQ(loc.byte_offset(), static_cast<std::size_t>(-1));
    }

    {
        SourceLocation loc;
        loc.set_byte_offset(100);
        EXPECT_EQ(loc.line(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.col(), static_cast<unsigned>(-1));
        EXPECT_EQ(loc.byte_offset(), 100u);
    }
}

TEST(SourceLocationTest, StatusChecks) {
    {
        SourceLocation const loc;
        EXPECT_FALSE(loc.has_line_col());
        EXPECT_FALSE(loc.has_byte_offset());
        EXPECT_FALSE(loc.is_fully_specified());
        EXPECT_TRUE(loc.is_unspecified());
        EXPECT_FALSE(loc.is_partially_specified());
    }

    {
        SourceLocation const loc(1, 2);
        EXPECT_TRUE(loc.has_line_col());
        EXPECT_FALSE(loc.has_byte_offset());
        EXPECT_FALSE(loc.is_fully_specified());
        EXPECT_FALSE(loc.is_unspecified());
        EXPECT_TRUE(loc.is_partially_specified());
    }

    {
        SourceLocation const loc(42);
        EXPECT_FALSE(loc.has_line_col());
        EXPECT_TRUE(loc.has_byte_offset());
        EXPECT_FALSE(loc.is_fully_specified());
        EXPECT_FALSE(loc.is_unspecified());
        EXPECT_TRUE(loc.is_partially_specified());
    }
}

TEST(SourceLocationTest, Comparison) {
    SourceLocation const loc1(2, 3);
    SourceLocation const loc2(2, 3);
    SourceLocation const loc3(0, 3);
    SourceLocation const loc4(3);
    SourceLocation const loc5(3);
    SourceLocation const loc6(5);

    EXPECT_EQ(loc1, loc2);
    EXPECT_NE(loc1, loc3);
    EXPECT_NE(loc3, loc4);
    EXPECT_EQ(loc4, loc5);
    EXPECT_NE(loc4, loc6);
}

TEST(SourceLocationDeathTest, InvalidSetLineCol) {
    SourceLocation loc(10u);
    EXPECT_DEBUG_DEATH(loc.set_line(3), "");
    EXPECT_DEBUG_DEATH(loc.set_col(3), "");
    EXPECT_DEBUG_DEATH(loc.set_line_col(2, 4), "");
}

TEST(SourceLocationDeathTest, InvalidSetByteOffset) {
    SourceLocation loc(1, 2);
    EXPECT_DEBUG_DEATH(loc.set_byte_offset(20), "");
}
}  // namespace
}  // namespace ants
