#include "annotate_snippets/style.hpp"
#include "annotate_snippets/styled_string.hpp"

#include "gtest/gtest.h"

namespace {
using LineParts = std::vector<std::vector<ants::StyledStringViewPart>>;

TEST(StyledStringTest, AppendContent) {
    {
        auto str = ants::StyledString::inferred("Hello");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "Hello", .style = ants::Style::Auto } } })
        );
        str.append("World", ants::Style::Auto);
        EXPECT_EQ(str.content(), "HelloWorld");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "HelloWorld", .style = ants::Style::Auto } } })
        );
        str.append(".", ants::Style::Default);
        EXPECT_EQ(str.content(), "HelloWorld.");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "HelloWorld", .style = ants::Style::Auto },
                { .content = ".", .style = ants::Style::Default },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString();
        str.append("Hello", ants::Style::Auto);
        EXPECT_EQ(str.content(), "Hello");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "Hello", .style = ants::Style::Auto } } })
        );
    }

    {
        auto str = ants::StyledString();
        str.append("", ants::Style::Auto);
        EXPECT_EQ(str.content(), "");
        EXPECT_TRUE(str.empty());
        EXPECT_EQ(str.styled_line_parts(), LineParts {});
    }

    {
        auto str = ants::StyledString();
        str.append("", ants::Style::Auto);
        str.append("", ants::Style::Default);
        str.append("", ants::Style::Highlight);
        EXPECT_EQ(str.content(), "");
        EXPECT_TRUE(str.empty());
        EXPECT_EQ(str.styled_line_parts(), LineParts {});
        str.append("Hello", ants::Style::Addition);
        EXPECT_EQ(str.content(), "Hello");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "Hello", .style = ants::Style::Addition } } })
        );
    }

    {
        auto str = ants::StyledString::inferred("Hello").with_style(ants::Style::Default, 2);
        str.append("World", ants::Style::Auto);
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "He", .style = ants::Style::Auto },
                { .content = "llo", .style = ants::Style::Default },
                { .content = "World", .style = ants::Style::Auto },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::inferred("Hello").with_style(ants::Style::Default, 2);
        str.append("World", ants::Style::Default);
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "He", .style = ants::Style::Auto },
                { .content = "lloWorld", .style = ants::Style::Default },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::plain("Hello");
        str.append("World", ants::Style::Auto, ants::Style::Highlight);
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Default },
                { .content = "World", .style = ants::Style::Highlight },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::plain("Hello");
        str.append("World", ants::Style::Auto, ants::Style::Default);
        EXPECT_EQ(str.content(), "HelloWorld");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "HelloWorld", .style = ants::Style::Default } } })
        );
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        str.append("World", ants::Style::Auto, ants::Style::Highlight);
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = "World", .style = ants::Style::Highlight },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        str.append("World", ants::Style::Auto, ants::Style::Highlight);
        EXPECT_EQ(str.content(), "HelloWorld");
        str.set_style(ants::Style::Default, 3, 6);
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hel", .style = ants::Style::Auto },
                { .content = "loW", .style = ants::Style::Default },
                { .content = "orld", .style = ants::Style::Highlight },
            } })
        );
        // clang-format on
    }
}

TEST(StyledStringTest, AppendParts) {
    {
        auto str = ants::StyledString::inferred("Hello");
        auto const append = ants::StyledString::styled("World", ants::Style::Addition);
        str.append(append.styled_line_parts().front());
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = "World", .style = ants::Style::Addition },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        auto const append = ants::StyledString::inferred("World");
        str.append(append.styled_line_parts().front());
        EXPECT_EQ(str.content(), "HelloWorld");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "HelloWorld", .style = ants::Style::Auto } } })
        );
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        auto const append =
            ants::StyledString::inferred("World").with_style(ants::Style::Default, 3);
        str.append(append.styled_line_parts().front());
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "HelloWor", .style = ants::Style::Auto },
                { .content = "ld", .style = ants::Style::Default },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        auto const append = ants::StyledString::inferred("World")
                                .with_style(ants::Style::custom(1), 0)
                                .with_style(ants::Style::custom(2), 1)
                                .with_style(ants::Style::custom(3), 3)
                                .with_style(ants::Style::custom(4), 4);
        str.append(append.styled_line_parts().front());
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = "W", .style = ants::Style::custom(1) },
                { .content = "or", .style = ants::Style::custom(2) },
                { .content = "l", .style = ants::Style::custom(3) },
                { .content = "d", .style = ants::Style::custom(4) },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::inferred("");
        auto const append = ants::StyledString::inferred("Hello")
                                .with_style(ants::Style::custom(1), 0)
                                .with_style(ants::Style::custom(2), 1)
                                .with_style(ants::Style::custom(3), 3)
                                .with_style(ants::Style::custom(4), 4);
        str.append(append.styled_line_parts().front());
        EXPECT_EQ(str.content(), "Hello");
        EXPECT_EQ(str.styled_line_parts(), append.styled_line_parts());
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        auto const append =
            ants::StyledString::inferred("World").with_style(ants::Style::Default, 3);
        str.append(append.styled_line_parts().front(), ants::Style::Highlight);
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = "Wor", .style = ants::Style::Highlight },
                { .content = "ld", .style = ants::Style::Default },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::plain("Hello");
        auto const append =
            ants::StyledString::inferred("World").with_style(ants::Style::Highlight, 3);
        str.append(append.styled_line_parts().front(), ants::Style::Default);
        EXPECT_EQ(str.content(), "HelloWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "HelloWor", .style = ants::Style::Default },
                { .content = "ld", .style = ants::Style::Highlight },
            } })
        );
        // clang-format on
    }
}

TEST(StyledStringTest, AppendNewline) {
    {
        auto str = ants::StyledString::inferred("Hello");
        str.append_newline();
        EXPECT_EQ(str.content(), "Hello\n");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "Hello", .style = ants::Style::Auto } } })
        );

        str.append_newline();
        EXPECT_EQ(str.content(), "Hello\n\n");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { { .content = "Hello", .style = ants::Style::Auto } },
                { { .content = "", .style = ants::Style::Auto } },
            } })
        );
        // clang-format on

        str.append("World", ants::Style::Default);
        EXPECT_EQ(str.content(), "Hello\n\nWorld");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { { .content = "Hello", .style = ants::Style::Auto } },
                { { .content = "", .style = ants::Style::Auto } },
                { { .content = "World", .style = ants::Style::Default } },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString();
        str.append_newline();
        EXPECT_EQ(str.content(), "\n");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "", .style = ants::Style::Auto } } })
        );

        str.append("Hello", ants::Style::Default);
        EXPECT_EQ(str.content(), "\nHello");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { { .content = "", .style = ants::Style::Auto } },
                { { .content = "Hello", .style = ants::Style::Default } },
            } })
        );
        // clang-format on

        str.append_newline();
        str.append("World", ants::Style::Highlight);
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { { .content = "", .style = ants::Style::Auto } },
                { { .content = "Hello", .style = ants::Style::Default } },
                { { .content = "World", .style = ants::Style::Highlight } },
            } })
        );
        // clang-format on
    }
}

TEST(StyledStringTest, AppendSpace) {
    {
        auto str = ants::StyledString();
        str.append_spaces(0);
        EXPECT_TRUE(str.empty());
        EXPECT_EQ(str.styled_line_parts(), LineParts {});

        str.append_spaces(3);
        EXPECT_EQ(str.content(), "   ");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "   ", .style = ants::Style::Default } } })
        );

        str.append_spaces(2);
        EXPECT_EQ(str.content(), "     ");
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { { { .content = "     ", .style = ants::Style::Default } } })
        );

        str.append("Hello", ants::Style::Auto);
        str.append_spaces(3);
        EXPECT_EQ(str.content(), "     Hello   ");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "     ", .style = ants::Style::Default },
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = "   ", .style = ants::Style::Default },
            } })
        );
        // clang-format on
    }

    {
        auto str = ants::StyledString::inferred("Hello");
        str.append_spaces(1);
        EXPECT_EQ(str.content(), "Hello ");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = " ", .style = ants::Style::Default },
            } })
        );
        // clang-format on

        str.append("World", ants::Style::Default);
        EXPECT_EQ(str.content(), "Hello World");
        // clang-format off
        EXPECT_EQ(
            str.styled_line_parts(),
            (LineParts { {
                { .content = "Hello", .style = ants::Style::Auto },
                { .content = " World", .style = ants::Style::Default },
            } })
        );
        // clang-format on
    }
}
}  // namespace
