#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/diag.hpp"
#include "annotate_snippets/patch.hpp"
#include "annotate_snippets/renderer/human_renderer.hpp"
#include "level_for_test.hpp"

#include "gtest/gtest.h"

#include <string_view>
#include <utility>

namespace {
TEST(HumanRendererSuggestionTest, SingleLineSinglePatch) {
    std::string_view const source = "Hello World";
    ants::HumanRenderer const renderer;

    auto const render_diag = [&](ants::Patch patch) {
        return renderer.render_diag(
            ants::Diag(Level::Error, "message")
                .with_source(ants::AnnotatedSource(source, "name").with_patch(patch))
        );
    };

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, ",")).content(),
        R"(error: message
 --> name
  |
1 | Hello, World
  |      +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, ", ")).content(),
        R"(error: message
 --> name
  |
1 | Hello,  World
  |      ++)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(0, "\"")).content(),
        R"(error: message
 --> name
  |
1 | "Hello World
  | +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(0, "Say ")).content(),
        R"(error: message
 --> name
  |
1 | Say Hello World
  | ++++)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(11, ".")).content(),
        R"(error: message
 --> name
  |
1 | Hello World.
  |            +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(11, "!!!")).content(),
        R"(error: message
 --> name
  |
1 | Hello World!!!
  |            +++)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(12, "??")).content(),
        R"(error: message
 --> name
  |
2 | ??
  | ++)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(5, 6)).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 + HelloWorld)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 2)).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 + llo World)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(10, 11)).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 + Hello Worl)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(11, 12)).content(),
        R"(error: message
 --> name
  |
2 -)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(10, 12)).content(),
        R"(error: message
 --> name
  |
1 - Hello World
2 -
1 + Hello Worl)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 11)).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 12)).content(),
        R"(error: message
 --> name
  |
1 - Hello World)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(5, 6, ",")).content(),
        R"(error: message
 --> name
  |
1 | Hello,World
  |      ~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(5, 6, ", ")).content(),
        R"(error: message
 --> name
  |
1 | Hello, World
  |      ~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(6, 11, "C++")).content(),
        R"(error: message
 --> name
  |
1 | Hello C++
  |       ~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(6, 11, "Ants!!")).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 + Hello Ants!!)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 11, "Good Morning")).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 + Good Morning)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 12, "Good Morning\n")).content(),
        R"(error: message
 --> name
  |
1 - Hello World
1 + Good Morning)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 12, "Good Morning")).content(),
        R"(error: message
 --> name
  |
1 - Hello World
2 -
1 + Good Morning)"
    );
}

TEST(HumanRendererSuggestionTest, MultiLineSinglePatch) {
    std::string_view const source = "int a = 0;\nint b = 1;";
    ants::HumanRenderer const renderer;

    auto const render_diag = [&](ants::Patch patch) {
        return renderer.render_diag(
            ants::Diag(Level::Error, "message")
                .with_source(ants::AnnotatedSource(source, "name").with_patch(patch))
        );
    };

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, "1")).content(),
        R"(error: message
 --> name
  |
1 | int a1 = 0;
  |      +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(14, "64_t")).content(),
        R"(error: message
 --> name
  |
2 | int64_t b = 1;
  |    ++++)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(10, "\n")).content(),
        R"(error: message
 --> name
  |
2 +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(11, "\n")).content(),
        R"(error: message
 --> name
  |
2 +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(10, "\nint c = 3;")).content(),
        R"(error: message
 --> name
  |
2 + int c = 3;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(11, "int c = 3;\n")).content(),
        R"(error: message
 --> name
  |
2 + int c = 3;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(0, "int c = 3;\n")).content(),
        R"(error: message
 --> name
  |
1 + int c = 3;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(0, "int c = 3;\nint d = 4;\n")).content(),
        R"(error: message
 --> name
  |
1 + int c = 3;
2 + int d = 4;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(7, " 100;\nint c =")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int a = 100;
2 + int c = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(7, " 100;\nint c =\n")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int a = 100;
2 + int c =
3 +  0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(22, "int c = 2;\n")).content(),
        R"(error: message
 --> name
  |
3 + int c = 2;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(3, 5)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(16, 19)).content(),
        R"(error: message
 --> name
  |
2 - int b = 1;
2 + int b1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(10, 11)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
1 + int a = 0;int b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(6, 17)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
1 + int a = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 11)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 10)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 22)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(11, 22)).content(),
        R"(error: message
 --> name
  |
2 - int b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 3, "float")).content(),
        R"(error: message
 --> name
  |
1 | float a = 0;
  | ~~~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(19, 20, "1.0f")).content(),
        R"(error: message
 --> name
  |
2 | int b = 1.0f;
  |         ~~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(10, 11, "int c = 3;\n")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int a = 0;int c = 3;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(10, 11, "\nint c = 3;")).content(),
        R"(error: message
 --> name
  |
2 - int b = 1;
2 + int c = 3;int b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(10, 11, "int c = 3;\nint d = 4;")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
1 + int a = 0;int c = 3;
2 + int d = 4;int b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(10, 11, "int c = 3;\nint d = 4;\n")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int a = 0;int c = 3;
2 + int d = 4;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(10, 11, "\nint c = 3;\nint d = 4;")).content(),
        R"(error: message
 --> name
  |
2 - int b = 1;
2 + int c = 3;
3 + int d = 4;int b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(8, 11, "1;\n")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int a = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(8, 14, "1;\nfloat")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
1 + int a = 1;
2 + float b = 1;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 11, "float a = 1.0f;\n")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + float a = 1.0f;)"
    );
}

TEST(HumanRendererSuggestionTest, SingleLineMultiPatch) {
    std::string_view const source = "int a = 0;";
    ants::HumanRenderer const renderer;

    auto const render_diag = [&](auto... patches) {
        ants::AnnotatedSource s(source, "name");
        (s.add_patch(patches), ...);

        return renderer.render_diag(ants::Diag(Level::Error, "message").with_source(std::move(s)));
    };

    EXPECT_EQ(
        render_diag(ants::Patch::addition(3, "64_t"), ants::Patch::addition(9, "L")).content(),
        R"(error: message
 --> name
  |
1 | int64_t a = 0L;
  |    ++++      +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(9, "L"), ants::Patch::addition(3, "64_t")).content(),
        R"(error: message
 --> name
  |
1 | int64_t a = 0L;
  |    ++++      +)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, "1"), ants::Patch::deletion(9, 10)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + int a1 = 0)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, "1"), ants::Patch::deletion(0, 4)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + a1 = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, "1"), ants::Patch::replacement(8, 9, "123")).content(),
        R"(error: message
 --> name
  |
1 | int a1 = 123;
  |      +   ~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(9, ".f"), ants::Patch::replacement(0, 3, "float"))
            .content(),
        R"(error: message
 --> name
  |
1 | float a = 0.f;
  | ~~~~~      ++)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 2), ants::Patch::deletion(2, 4)).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + a = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 4), ants::Patch::replacement(4, 5, "abc")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + abc = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(5, 9), ants::Patch::replacement(0, 3, "float")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + float a;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 3, "float"), ants::Patch::replacement(8, 9, "1.f"))
            .content(),
        R"(error: message
 --> name
  |
1 | float a = 1.f;
  | ~~~~~     ~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 3, "double"), ants::Patch::replacement(8, 9, "1."))
            .content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + double a = 1.;)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(0, "std::"),
            ants::Patch::addition(3, "64_t"),
            ants::Patch::addition(9, "L")
        )
            .content(),
        R"(error: message
 --> name
  |
1 | std::int64_t a = 0L;
  | +++++   ++++      +)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(0, "std::"),
            ants::Patch::replacement(0, 3, "uint"),
            ants::Patch::addition(3, "64_t"),
            ants::Patch::addition(9, "L")
        )
            .content(),
        R"(error: message
 --> name
  |
1 | std::uint64_t a = 0L;
  | +++++~~~~++++      +)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(0, "std::"),
            ants::Patch::addition(3, "64_t"),
            ants::Patch::addition(9, "L"),
            ants::Patch::deletion(4, 6)
        )
            .content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + std::int64_t = 0L;)"
    );
}

TEST(HumanRendererSuggestionTest, MultiLineMultiPatch) {
    std::string_view const source = "int a = 0;\nint b = 1;\nint c = 2;";
    ants::HumanRenderer const renderer;

    auto const render_diag = [&](auto... patches) {
        ants::AnnotatedSource s(source, "name");
        (s.add_patch(patches), ...);

        return renderer.render_diag(ants::Diag(Level::Error, "message").with_source(std::move(s)));
    };

    EXPECT_EQ(
        render_diag(ants::Patch::addition(3, "64_t"), ants::Patch::addition(10, "\nint d = 2;"))
            .content(),
        R"(error: message
 --> name
  |
1 | int64_t a = 0;
  |    ++++
2 + int d = 2;)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(3, "64_t"),
            ants::Patch::deletion(8, 19),
            ants::Patch::deletion(21, 26)
        )
            .content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
3 - int c = 2;
1 + int64_t a = 1;c = 2;)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(3, "64_t"),
            ants::Patch::replacement(8, 14, "1;\nfloat"),
            ants::Patch::replacement(15, 16, "bcd"),
            ants::Patch::deletion(16, 31)
        )
            .content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
3 - int c = 2;
1 + int64_t a = 1;
2 + float bcd;)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(3, "64_t"),
            ants::Patch::replacement(8, 14, "1;\nfloat"),
            ants::Patch::replacement(15, 16, "bcd"),
            ants::Patch::replacement(19, 25, "3.0f;\nlong")
        )
            .content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
2 - int b = 1;
3 - int c = 2;
1 + int64_t a = 1;
2 + float bcd = 3.0f;
3 + long c = 2;)"
    );

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(11, "float d = 1.0f;\n"),
            ants::Patch::replacement(15, 16, "bcd")
        )
            .content(),
        R"(error: message
 --> name
  |
2 + float d = 1.0f;
3 | int bcd = 1;
  |     ~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(11, "float d = 1.0f;\n"), ants::Patch::deletion(15, 17))
            .content(),
        R"(error: message
 --> name
  |
2 - int b = 1;
2 + float d = 1.0f;
3 + int = 1;)"
    );
}

TEST(HumanRendererSuggestionTest, LineNumMode) {
    std::string_view const source = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14";
    ants::HumanRenderer renderer;

    auto const render_diag = [&](auto... patches) {
        ants::AnnotatedSource s(source, "name");
        (s.add_patch(patches), ...);

        return renderer.render_diag(ants::Diag(Level::Error, "message").with_source(std::move(s)));
    };

    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(0, "A0\nA1\nA2\n"),
            ants::Patch::replacement(6, 7, "R4"),
            ants::Patch::addition(10, "R"),
            ants::Patch::replacement(12, 14, "A7\n"),
            ants::Patch::addition(16, "R"),
            ants::Patch::deletion(18, 27),
            ants::Patch::replacement(30, 32, "R14")
        )
            .content(),
        R"(error: message
  --> name
   |
 1 + A0
 2 + A1
 3 + A2
...
 7 | R4
   | ~~
 8 | 5
 9 | R6
   | +
 7 - 7
10 + A7
11 | 8
12 | R9
   | +
10 - 10
11 - 11
12 - 12
13 | 13
14 | R14
   | ~~~)"
    );

    renderer.line_num_patch_mode = ants::HumanRenderer::BeforePatch;
    EXPECT_EQ(
        render_diag(
            ants::Patch::addition(0, "A0\nA1\nA2\n"),
            ants::Patch::replacement(6, 7, "R4"),
            ants::Patch::addition(10, "R"),
            ants::Patch::replacement(12, 14, "A7\n"),
            ants::Patch::addition(16, "R"),
            ants::Patch::deletion(18, 27),
            ants::Patch::replacement(30, 32, "R14")
        )
            .content(),
        R"(error: message
  --> name
   |
 1 + A0
 2 + A1
 3 + A2
...
 4 | R4
   | ~~
 5 | 5
 6 | R6
   | +
 7 - 7
10 + A7
 8 | 8
 9 | R9
   | +
10 - 10
11 - 11
12 - 12
13 | 13
14 | R14
   | ~~~)"
    );
}

TEST(HumanRendererSuggestionTest, ReplacementStyle) {
    std::string_view const source = "int a = 0;";
    ants::HumanRenderer renderer;

    auto const render_diag = [&](ants::Patch patch) {
        return renderer.render_diag(
            ants::Diag(Level::Error, "message")
                .with_source(ants::AnnotatedSource(source, "name").with_patch(patch))
        );
    };

    renderer.max_inline_style_single_line_replacement_length = 0;
    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 1, "a")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + ant a = 0;)"
    );

    renderer.max_inline_style_single_line_replacement_length = 1;
    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 1, "a")).content(),
        R"(error: message
 --> name
  |
1 | ant a = 0;
  | ~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 1, "aa")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + aant a = 0;)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 2, "a")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + at a = 0;)"
    );

    renderer.max_inline_style_single_line_replacement_length = 9;
    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 9, "abc = 0")).content(),
        R"(error: message
 --> name
  |
1 | abc = 0;
  | ~~~~~~~)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::replacement(0, 10, "abc = 0;")).content(),
        R"(error: message
 --> name
  |
1 - int a = 0;
1 + abc = 0;)"
    );
}

TEST(HumanRendererSuggestionTest, SourceNormalization) {
    std::string_view const source = "\tfunc(args1,\targs2)";
    ants::HumanRenderer renderer;

    auto const render_diag = [&](ants::Patch patch) {
        return renderer.render_diag(
            ants::Diag(Level::Error, "message")
                .with_source(ants::AnnotatedSource(source, "name").with_patch(patch))
        );
    };

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 1)).content(),
        R"(error: message
 --> name
  |
1 -     func(args1,    args2)
1 + func(args1,    args2))"
    );

    renderer.display_tab_width = 8;
    EXPECT_EQ(
        render_diag(ants::Patch::deletion(0, 1)).content(),
        R"(error: message
 --> name
  |
1 -         func(args1,        args2)
1 + func(args1,        args2))"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::addition(6, "\t")).content(),
        R"(error: message
 --> name
  |
1 |         func(        args1,        args2)
  |              ++++++++)"
    );

    renderer.display_tab_width = 2;
    EXPECT_EQ(
        render_diag(ants::Patch::replacement(12, 13, "\t\t")).content(),
        R"(error: message
 --> name
  |
1 |   func(args1,    args2)
  |              ~~~~)"
    );
}

TEST(HumanRendererSuggestionTest, LineNumWidth) {
    std::string_view const source = "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14";
    ants::HumanRenderer renderer;

    auto const render_diag = [&](unsigned first_line_num, auto... patches) {
        ants::AnnotatedSource s(source, "name");
        s.set_first_line_number(first_line_num);
        (s.add_patch(patches), ...);

        return renderer.render_diag(ants::Diag(Level::Error, "message").with_source(std::move(s)));
    };

    EXPECT_EQ(
        render_diag(
            86,
            ants::Patch::addition(0, "A0\nA1\nA2\n"),
            ants::Patch::replacement(6, 7, "R4"),
            ants::Patch::addition(10, "R"),
            ants::Patch::replacement(12, 14, "A7\n"),
            ants::Patch::addition(16, "R"),
            ants::Patch::deletion(18, 27),
            ants::Patch::replacement(30, 32, "R14")
        )
            .content(),
        R"(error: message
  --> name
   |
86 + A0
87 + A1
88 + A2
...
92 | R4
   | ~~
93 | 5
94 | R6
   | +
92 - 7
95 + A7
96 | 8
97 | R9
   | +
95 - 10
96 - 11
97 - 12
98 | 13
99 | R14
   | ~~~)"
    );

    EXPECT_EQ(
        render_diag(
            87,
            ants::Patch::addition(0, "A0\nA1\nA2\n"),
            ants::Patch::replacement(6, 7, "R4"),
            ants::Patch::addition(10, "R"),
            ants::Patch::replacement(12, 14, "A7\n"),
            ants::Patch::addition(16, "R"),
            ants::Patch::deletion(18, 27),
            ants::Patch::replacement(30, 32, "R14")
        )
            .content(),
        R"(error: message
   --> name
    |
 87 + A0
 88 + A1
 89 + A2
...
 93 | R4
    | ~~
 94 | 5
 95 | R6
    | +
 93 - 7
 96 + A7
 97 | 8
 98 | R9
    | +
 96 - 10
 97 - 11
 98 - 12
 99 | 13
100 | R14
    | ~~~)"
    );

    renderer.line_num_patch_mode = ants::HumanRenderer::BeforePatch;
    EXPECT_EQ(
        render_diag(
            86,
            ants::Patch::addition(0, "A0\nA1\nA2\n"),
            ants::Patch::replacement(6, 7, "R4"),
            ants::Patch::addition(10, "R"),
            ants::Patch::replacement(12, 14, "A7\n"),
            ants::Patch::addition(16, "R"),
            ants::Patch::deletion(18, 27),
            ants::Patch::replacement(30, 32, "R14")
        )
            .content(),
        R"(error: message
  --> name
   |
86 + A0
87 + A1
88 + A2
...
89 | R4
   | ~~
90 | 5
91 | R6
   | +
92 - 7
95 + A7
93 | 8
94 | R9
   | +
95 - 10
96 - 11
97 - 12
98 | 13
99 | R14
   | ~~~)"
    );

    EXPECT_EQ(
        render_diag(
            87,
            ants::Patch::addition(0, "A0\nA1\nA2\n"),
            ants::Patch::replacement(6, 7, "R4"),
            ants::Patch::addition(10, "R"),
            ants::Patch::replacement(12, 14, "A7\n"),
            ants::Patch::addition(16, "R"),
            ants::Patch::deletion(18, 27),
            ants::Patch::replacement(30, 32, "R14")
        )
            .content(),
        R"(error: message
   --> name
    |
 87 + A0
 88 + A1
 89 + A2
...
 90 | R4
    | ~~
 91 | 5
 92 | R6
    | +
 93 - 7
 96 + A7
 94 | 8
 95 | R9
    | +
 96 - 10
 97 - 11
 98 - 12
 99 | 13
100 | R14
    | ~~~)"
    );
}

TEST(HumanRendererSuggestionTest, Markers) {
    std::string_view const source = "int a = 0;";
    ants::HumanRenderer renderer;

    auto const render_diag = [&](auto... patches) {
        ants::AnnotatedSource s(source, "name");
        (s.add_patch(patches), ...);

        return renderer.render_diag(ants::Diag(Level::Error, "message").with_source(std::move(s)));
    };

    renderer.addition_marker = '^';
    renderer.deletion_marker = '~';
    renderer.replacement_marker = '*';

    EXPECT_EQ(
        render_diag(ants::Patch::addition(5, "1"), ants::Patch::replacement(8, 9, "123")).content(),
        R"(error: message
 --> name
  |
1 | int a1 = 123;
  |      ^   ***)"
    );

    EXPECT_EQ(
        render_diag(ants::Patch::deletion(5, 9), ants::Patch::replacement(0, 3, "float")).content(),
        R"(error: message
 --> name
  |
1 ~ int a = 0;
1 ^ float a;)"
    );
}
}  // namespace
