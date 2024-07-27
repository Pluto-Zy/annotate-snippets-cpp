#include "annotate_snippets/style.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include "gtest/gtest.h"

namespace {
using LineParts = std::vector<std::vector<ants::StyledStringViewPart>>;

TEST(StyledStringViewTest, SingleLineWithStyle) {
    {
        auto const lines = ants::StyledStringView::inferred("abcdefg").styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Auto } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("").styled_line_parts();
        LineParts const expected {};
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView().styled_line_parts();
        LineParts const expected {};
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Auto, 2, 4)
                               .styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Auto } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 7)
                               .styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Default } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abc", .style = ants::Style::Default },
            { .content = "defg", .style = ants::Style::Auto },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Default, 3, 7)
                               .styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Default } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Default, 2, 7)
                               .styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Default } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 3, 7)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abc", .style = ants::Style::Auto },
            { .content = "defg", .style = ants::Style::Default },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 3, 7)
                               .with_style(ants::Style::Default, 2, 3)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "ab", .style = ants::Style::Auto },
            { .content = "cdefg", .style = ants::Style::Default },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 2, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "ab", .style = ants::Style::Auto },
            { .content = "cde", .style = ants::Style::Default },
            { .content = "fg", .style = ants::Style::Auto },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 5, 7)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abc", .style = ants::Style::Default },
            { .content = "de", .style = ants::Style::Auto },
            { .content = "fg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 5, 7)
                               .with_style(ants::Style::Highlight, 2, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "ab", .style = ants::Style::Default },
            { .content = "cdefg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 5, 7)
                               .with_style(ants::Style::Highlight, 3, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abc", .style = ants::Style::Default },
            { .content = "defg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 5, 7)
                               .with_style(ants::Style::Highlight, 0, 3)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abc", .style = ants::Style::Highlight },
            { .content = "de", .style = ants::Style::Auto },
            { .content = "fg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 4, 7)
                               .with_style(ants::Style::PrimaryTitle, 2, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "ab", .style = ants::Style::Default },
            { .content = "cde", .style = ants::Style::PrimaryTitle },
            { .content = "fg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 4, 7)
                               .with_style(ants::Style::PrimaryTitle, 2, 7)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "ab", .style = ants::Style::Default },
            { .content = "cdefg", .style = ants::Style::PrimaryTitle },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 4, 7)
                               .with_style(ants::Style::PrimaryTitle, 0, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abcde", .style = ants::Style::PrimaryTitle },
            { .content = "fg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 4, 7)
                               .with_style(ants::Style::PrimaryTitle, 0, 4)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abcd", .style = ants::Style::PrimaryTitle },
            { .content = "efg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 4, 7)
                               .with_style(ants::Style::PrimaryTitle, 0, 3)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "abc", .style = ants::Style::PrimaryTitle },
            { .content = "d", .style = ants::Style::Auto },
            { .content = "efg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg")
                               .with_style(ants::Style::Default, 0, 3)
                               .with_style(ants::Style::Highlight, 4, 7)
                               .with_style(ants::Style::PrimaryTitle, 0, 1)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected { {
            { .content = "a", .style = ants::Style::PrimaryTitle },
            { .content = "bc", .style = ants::Style::Default },
            { .content = "d", .style = ants::Style::Auto },
            { .content = "efg", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }
}

TEST(StyledStringViewTest, MultiLineWithStyle) {
    {
        auto const lines = ants::StyledStringView::inferred("abcdefg\n").styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Auto } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg\n")
                               .with_style(ants::Style::Default, 0, 7)
                               .styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Default } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg\r\n").styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Auto } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg\r\n")
                               .with_style(ants::Style::Default, 0, 7)
                               .styled_line_parts();
        LineParts const expected { { { .content = "abcdefg", .style = ants::Style::Default } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abcdefg\n\n").styled_line_parts();
        LineParts const expected {
            { { .content = "abcdefg", .style = ants::Style::Auto } },
            { { .content = "", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("\nabcdefg").styled_line_parts();
        LineParts const expected {
            { { .content = "", .style = ants::Style::Auto } },
            { { .content = "abcdefg", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng").styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\r\ndef\ng").styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng\n").styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("ab\rc\ndef\ng").styled_line_parts();
        LineParts const expected {
            { { .content = "ab\rc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines =
            ants::StyledStringView::inferred("abc\ndef\r\n\r\ng").styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 0, 2)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "ab", .style = ants::Style::Default },
              { .content = "c", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 0, 3)
                               .styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Default } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 0, 4)
                               .styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Default } },
            { { .content = "def", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 0, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Default } },
            { { .content = "d", .style = ants::Style::Default },
              { .content = "ef", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 2, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "ab", .style = ants::Style::Auto },
              { .content = "c", .style = ants::Style::Default } },
            { { .content = "d", .style = ants::Style::Default },
              { .content = "ef", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 3, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "d", .style = ants::Style::Default },
              { .content = "ef", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 4, 5)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "d", .style = ants::Style::Default },
              { .content = "ef", .style = ants::Style::Auto } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 3, 8)
                               .styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Default } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 4, 8)
                               .styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Default } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 3, 7)
                               .styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Default } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("abc\ndef\ng")
                               .with_style(ants::Style::Default, 4, 7)
                               .styled_line_parts();
        LineParts const expected {
            { { .content = "abc", .style = ants::Style::Auto } },
            { { .content = "def", .style = ants::Style::Default } },
            { { .content = "g", .style = ants::Style::Auto } },
        };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("ab\r\nc\nde\nfgh\r\ni")
                               .with_style(ants::Style::Default, 2, 10)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "ab", .style = ants::Style::Auto } },
            { { .content = "c", .style = ants::Style::Default } },
            { { .content = "de", .style = ants::Style::Default } },
            { { .content = "f", .style = ants::Style::Default },
              { .content = "gh", .style = ants::Style::Auto } },
            { { .content = "i", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::inferred("ab\r\nc\nde\nfgh\r\ni")
                               .with_style(ants::Style::Default, 1, 10)
                               .styled_line_parts();
        // clang-format off
        LineParts const expected {
            { { .content = "a", .style = ants::Style::Auto },
              { .content = "b", .style = ants::Style::Default } },
            { { .content = "c", .style = ants::Style::Default } },
            { { .content = "de", .style = ants::Style::Default } },
            { { .content = "f", .style = ants::Style::Default },
              { .content = "gh", .style = ants::Style::Auto } },
            { { .content = "i", .style = ants::Style::Auto } },
        };
        // clang-format on
        EXPECT_EQ(lines, expected);
    }
}

TEST(StyledStringViewTest, Constructor) {
    {
        auto const lines = ants::StyledStringView::inferred("abc").styled_line_parts();
        LineParts const expected { { { .content = "abc", .style = ants::Style::Auto } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines = ants::StyledStringView::plain("abc").styled_line_parts();
        LineParts const expected { { { .content = "abc", .style = ants::Style::Default } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines =
            ants::StyledStringView::styled("abc", ants::Style::Highlight).styled_line_parts();
        LineParts const expected { { { .content = "abc", .style = ants::Style::Highlight } } };
        EXPECT_EQ(lines, expected);
    }

    {
        auto const lines =
            ants::StyledStringView::styled("abc", ants::Style::custom(1)).styled_line_parts();
        LineParts const expected { { { .content = "abc", .style = ants::Style::custom(1) } } };
        EXPECT_EQ(lines, expected);
        EXPECT_NE(lines.front().front().style, ants::Style::Default);
    }

    {
        ants::StyledStringView const str = "abc";
        LineParts const expected { { { .content = "abc", .style = ants::Style::Auto } } };
        EXPECT_EQ(str.styled_line_parts(), expected);
    }

    {
        ants::StyledStringView const str("abc");
        LineParts const expected { { { .content = "abc", .style = ants::Style::Auto } } };
        EXPECT_EQ(str.styled_line_parts(), expected);
    }

    {
        ants::StyledStringView const str(std::string_view("abc"));
        LineParts const expected { { { .content = "abc", .style = ants::Style::Auto } } };
        EXPECT_EQ(str.styled_line_parts(), expected);
    }
}

TEST(StyledStringViewTest, Setter) {
    {
        auto view = ants::StyledStringView::inferred("abcd");
        LineParts expected { { { .content = "abcd", .style = ants::Style::Auto } } };
        EXPECT_EQ(view.styled_line_parts(), expected);

        view.set_style(ants::Style::Default);
        expected = { { { .content = "abcd", .style = ants::Style::Default } } };
        EXPECT_EQ(view.styled_line_parts(), expected);

        view.set_style(ants::Style::Highlight, 3);
        // clang-format off
        expected = { {
            { .content = "abc", .style = ants::Style::Default },
            { .content = "d", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(view.styled_line_parts(), expected);

        view.set_style(ants::Style::PrimaryUnderline, 1, 2);
        // clang-format off
        expected = { {
            { .content = "a", .style = ants::Style::Default },
            { .content = "b", .style = ants::Style::PrimaryUnderline },
            { .content = "c", .style = ants::Style::Default },
            { .content = "d", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(view.styled_line_parts(), expected);

        view.set_style(ants::Style::custom(3), 1, 2);
        // clang-format off
        expected = { {
            { .content = "a", .style = ants::Style::Default },
            { .content = "b", .style = ants::Style::custom(3) },
            { .content = "c", .style = ants::Style::Default },
            { .content = "d", .style = ants::Style::Highlight },
        } };
        // clang-format on
        EXPECT_EQ(view.styled_line_parts(), expected);

        view.set_style(ants::Style::custom(1));
        view.set_style(ants::Style::custom(2), 2, 2);
        expected = { { { .content = "abcd", .style = ants::Style::custom(1) } } };
        EXPECT_EQ(view.styled_line_parts(), expected);

        view.set_style(ants::Style::custom(2), 2);
        view.set_style(ants::Style::custom(3), 2, 2);
        // clang-format off
        expected = { {
            { .content = "ab", .style = ants::Style::custom(1) },
            { .content = "cd", .style = ants::Style::custom(2) },
        } };
        // clang-format on
        EXPECT_EQ(view.styled_line_parts(), expected);
    }
}
}  // namespace
