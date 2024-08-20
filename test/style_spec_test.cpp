#include "annotate_snippets/style_spec.hpp"

#include "gtest/gtest.h"

namespace {
TEST(StyleSpecTest, Constructor) {
    {
        constexpr ants::StyleSpec spec(ants::StyleSpec::Red);
        static_assert(spec.foreground_color() == ants::StyleSpec::Red);
        static_assert(spec.background_color() == ants::StyleSpec::Default);
        static_assert(spec.text_styles() == 0);
    }

    {
        constexpr ants::StyleSpec spec = ants::StyleSpec::Red;
        static_assert(spec.foreground_color() == ants::StyleSpec::Red);
        static_assert(spec.background_color() == ants::StyleSpec::Default);
        static_assert(spec.text_styles() == 0);
    }

    {
        constexpr ants::StyleSpec spec(ants::StyleSpec::Red, ants::StyleSpec::Blue);
        static_assert(spec.foreground_color() == ants::StyleSpec::Red);
        static_assert(spec.background_color() == ants::StyleSpec::Blue);
        static_assert(spec.text_styles() == 0);
    }

    {
        constexpr ants::StyleSpec spec(  //
            ants::StyleSpec::Red,
            ants::StyleSpec::Blue,
            ants::StyleSpec::Bold
        );
        static_assert(spec.foreground_color() == ants::StyleSpec::Red);
        static_assert(spec.background_color() == ants::StyleSpec::Blue);
        static_assert(spec.text_styles() == ants::StyleSpec::Bold);
    }
}

TEST(StyleSpecTest, ForegroundColor) {
    ants::StyleSpec spec;
    EXPECT_EQ(spec.foreground_color(), ants::StyleSpec::Default);

    spec.set_foreground_color(ants::StyleSpec::Blue);
    EXPECT_EQ(spec.foreground_color(), ants::StyleSpec::Blue);

    spec.set_foreground_color(ants::StyleSpec::Red);
    EXPECT_EQ(spec.foreground_color(), ants::StyleSpec::Red);

    spec.reset_foreground_color();
    EXPECT_EQ(spec.foreground_color(), ants::StyleSpec::Default);
}

TEST(StyleSpecTest, BackgroundColor) {
    ants::StyleSpec spec;
    EXPECT_EQ(spec.background_color(), ants::StyleSpec::Default);

    spec.set_background_color(ants::StyleSpec::Blue);
    EXPECT_EQ(spec.background_color(), ants::StyleSpec::Blue);

    spec.set_background_color(ants::StyleSpec::Red);
    EXPECT_EQ(spec.background_color(), ants::StyleSpec::Red);

    spec.reset_background_color();
    EXPECT_EQ(spec.background_color(), ants::StyleSpec::Default);
}

TEST(StyleSpecTest, TextStyle) {
    ants::StyleSpec spec;
    EXPECT_EQ(spec.text_styles(), 0);

    spec.add_text_style(ants::StyleSpec::Bold);
    EXPECT_EQ(spec.text_styles(), ants::StyleSpec::Bold);
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Bold));

    spec.add_text_style(ants::StyleSpec::Italic);
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Bold));
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Italic));
    EXPECT_EQ(spec.text_styles(), ants::StyleSpec::Bold | ants::StyleSpec::Italic);

    spec.remove_text_style(ants::StyleSpec::Bold);
    EXPECT_FALSE(spec.has_text_style(ants::StyleSpec::Bold));
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Italic));
    EXPECT_EQ(spec.text_styles(), ants::StyleSpec::Italic);

    spec.add_text_style(ants::StyleSpec::Dim | ants::StyleSpec::Underline);
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Dim));
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Underline));
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Italic));

    spec.remove_text_style(ants::StyleSpec::Italic | ants::StyleSpec::Dim | ants::StyleSpec::Bold);
    EXPECT_TRUE(spec.has_text_style(ants::StyleSpec::Underline));
    EXPECT_FALSE(spec.has_text_style(ants::StyleSpec::Italic));
    EXPECT_FALSE(spec.has_text_style(ants::StyleSpec::Dim));
    EXPECT_FALSE(spec.has_text_style(ants::StyleSpec::Bold));

    spec.clear_text_styles();
    EXPECT_EQ(spec.text_styles(), 0);
}

TEST(StyleSpecTest, Operator) {
    static_assert(
        ants::StyleSpec(ants::StyleSpec::Red) + ants::StyleSpec::Bold
        == ants::StyleSpec(ants::StyleSpec::Red, ants::StyleSpec::Default, ants::StyleSpec::Bold)
    );
    static_assert(
        ants::StyleSpec(ants::StyleSpec::Red) + (ants::StyleSpec::Bold | ants::StyleSpec::Underline)
        == ants::StyleSpec(
            ants::StyleSpec::Red,
            ants::StyleSpec::Default,
            ants::StyleSpec::Bold | ants::StyleSpec::Underline
        )
    );
    static_assert(
        ants::StyleSpec::Bold + ants::StyleSpec(ants::StyleSpec::Red)
        == ants::StyleSpec(ants::StyleSpec::Red, ants::StyleSpec::Default, ants::StyleSpec::Bold)
    );

    static_assert(
        ants::StyleSpec::Red + ants::StyleSpec::Bold
        == ants::StyleSpec(ants::StyleSpec::Red, ants::StyleSpec::Default, ants::StyleSpec::Bold)
    );
    static_assert(
        ants::StyleSpec::Bold + ants::StyleSpec::Red == ants::StyleSpec::Red + ants::StyleSpec::Bold
    );
    static_assert(
        ants::StyleSpec::Red + (ants::StyleSpec::Bold | ants::StyleSpec::Underline)
        == ants::StyleSpec::Red + ants::StyleSpec::Bold + ants::StyleSpec::Underline
    );

    static_assert(
        ants::StyleSpec(ants::StyleSpec::Red) - ants::StyleSpec::Bold
        == ants::StyleSpec(ants::StyleSpec::Red)
    );
    static_assert(
        ants::StyleSpec(ants::StyleSpec::Red) + ants::StyleSpec::Bold - ants::StyleSpec::Bold
        == ants::StyleSpec(ants::StyleSpec::Red)
    );
    static_assert(
        ants::StyleSpec(ants::StyleSpec::Red) + ants::StyleSpec::Bold + ants::StyleSpec::Underline
            - ants::StyleSpec::Bold
        == ants::StyleSpec(ants::StyleSpec::Red) + ants::StyleSpec::Underline
    );
}
}  // namespace
