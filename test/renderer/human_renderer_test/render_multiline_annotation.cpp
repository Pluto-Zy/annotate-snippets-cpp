#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/diag.hpp"
#include "annotate_snippets/renderer/human_renderer.hpp"
#include "annotate_snippets/source_location.hpp"
#include "annotate_snippets/styled_string_view.hpp"
#include "level_for_test.hpp"

#include "gtest/gtest.h"

#include <string>
#include <string_view>

namespace {
TEST(HumanRendererMultilineAnnotationTest, BasicFormat) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello World" << '\n';
})";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 1 }
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:20
  |
1 |   auto main() -> int {
  |  ____________________^
2 | |     std::cout << "Hello World" << '\n';
3 | | }
  | |_^)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 1 },
                                ants::StyledStringView::inferred("label")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:20
  |
1 |   auto main() -> int {
  |  ____________________^
2 | |     std::cout << "Hello World" << '\n';
3 | | }
  | |_^ label)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 1, 4 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:20
  |
1 |    auto main() -> int {
  |  _______-             ^
  | | ____________________|
2 | ||     std::cout << "Hello World" << '\n';
  | ||____^ label1 -
  | |______________|
  |                label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 1, 4 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             -
  | | ____________________|
2 | ||     std::cout << "Hello World" << '\n';
  | ||____^ label1 -
  |  |_____________|
  |                label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 1, 4 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 9 },
                                ants::SourceLocation { 1, 18 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |     auto main() -> int {
  |  ________^   ^         -
  | | ___________|         |
  | || ____________________|
2 | |||     std::cout << "Hello World" << '\n';
  | |||____^ label1 -    ^
  |  ||_____________|____|
  |   |_____________|    label3
  |                 label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 1, 4 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 9 },
                                ants::SourceLocation { 1, 18 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:20
  |
1 |     auto main() -> int {
  |  ________-   ^         ^
  | | ___________|         |
  | || ____________________|
2 | |||     std::cout << "Hello World" << '\n';
  | |||____^ label1 -    ^
  | ||______________|____|
  | |_______________|    label3
  |                 label2)"
    );
}

TEST(HumanRendererMultilineAnnotationTest, Depth) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello";
    std::cout << "World";
})";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 1, 14 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |     auto main() -> int {
  |   _______^             ^
  |  |_____________________|
2 | ||      std::cout << "Hello";
  | ||              ^ -
  | || _____________|_|
  | |||_____________|
  | | |             label1
3 | | |     std::cout << "World";
  | | |_____________- label2
4 | |   }
  | |___^ label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 1, 14 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 15 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |     auto main() -> int {
  |  ________^             ^
  | | _____________________|
2 | ||      std::cout << "Hello";
  | || _______________-
3 | |||     std::cout << "World";
  | |||             ^ ^
  | |||_____________|_|
  | |_|_____________| label3
  |   |             label1
4 |   | }
  |   |_- label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 2, 14 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |   ______^             ^
  |  |____________________|
2 | ||     std::cout << "Hello";
  | ||_____________^ label1
3 | |      std::cout << "World";
  | | _______________-
4 | || }
  | || ^
  | ||_|
  | |__label2
  |    label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 1, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 2, 14 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:20
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
  | ||_____________^ label1
3 | |      std::cout << "World";
  | | _______________-
4 | || }
  | || ^
  | ||_|
  | |__label2
  |    label3)"
    );
}

TEST(HumanRendererMultilineAnnotationTest, LabelPosition) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello";
    std::cout << "World";
})";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||_________^ label1 ^
  |  |__________________|
  |                     label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 10 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||          ^       ^
  | ||__________|       |
  |  |          label1  |
  |  |__________________|
  |                     label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||             ^    ^
  | ||_____________|____|
  | |______________|    label2
  |                label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 13 },
                                ants::SourceLocation { 2, 14 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |             ^^
  | |             ||
  | |_____________|label2
  |               label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 12 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |             ^ label2
  | |_____________|
  |               label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |            ^^
  | |____________||
  |              |label1
  |              label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 10 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |         ^^^^^ label3
  |  _______|
  | | ______|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||            ^     ^
  | ||____________|_____|
  | |_____________|     label1
  |               label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 15 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^  ^
  | | _________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||            ^   ^^^ label3
  | ||____________|
  | |_____________label2
  |               label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 15 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^  ^
  | | _________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||            ^   ^^^ label3
  | ||____________|
  |  |____________label
  |               label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 15 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^  ^
  | | _________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||            ^   ^^^ label3
  | ||            |
  | ||____________label4
  | |_____________label2
  |               label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 15 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^  ^
  | | _________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||            ^   ^^^ label3
  | ||____________|
  |  |            label
  |  |____________label4
  |               label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 15 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |      auto main() -> int {
  |  _________^  ^  ^  ^
  | | ___________|  |  |
  | || _____________|  |
  | ||| _______________|
2 | ||||     std::cout << "Hello";
3 | ||||     std::cout << "World";
  | ||||         ^  ^  ^  ^
  | ||||_________|__|__|__|
  |  |||_________|__|__|  label1
  |   ||_________|__|  label2
  |    |_________|  label3
  |              label4)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 25 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 15 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |      auto main() -> int {
  |  _________^  ^  ^  ^
  | | ___________|  |  |
  | || _____________|  |
  | ||| _______________|
2 | ||||     std::cout << "Hello";
3 | ||||     std::cout << "World";
  | ||||         ^  ^  ^         ^
  | ||||_________|__|__|         |
  | | ||_________|__|  label2    |
  | |  |_________|  label3       |
  | |            label4          |
  | |____________________________|
  |                              label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 25 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 22 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |      auto main() -> int {
  |  _________^  ^  ^  ^
  | | ___________|  |  |
  | || _____________|  |
  | ||| _______________|
2 | ||||     std::cout << "Hello";
3 | ||||     std::cout << "World";
  | ||||         ^  ^         ^  ^
  | ||||_________|__|         |  |
  | || |_________|  label3    |  |
  | ||           label4       |  |
  | ||________________________|__|
  |  |________________________|  label1
  |                           label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 25 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 22 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 19 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |      auto main() -> int {
  |  _________^  ^  ^  ^
  | | ___________|  |  |
  | || _____________|  |
  | ||| _______________|
2 | ||||     std::cout << "Hello";
3 | ||||     std::cout << "World";
  | ||||_________^ label4  ^  ^  ^
  | |||____________________|__|__|
  |  ||____________________|__|  label1
  |   |____________________|  label2
  |                        label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 26 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 19 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |      auto main() -> int {
  |  _________^  ^  ^  ^
  | | ___________|  |  |
  | || _____________|  |
  | ||| _______________|
2 | ||||     std::cout << "Hello";
3 | ||||     std::cout << "World";
  | ||||         ^  ^      ^      ^
  | ||||_________|__|      |      |
  | || |_________|  label3 |      |
  | ||           label4    |      |
  | ||_____________________|      |
  | |                      label2 |
  | |_____________________________|
  |                               label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 10 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 10 },
                                ants::StyledStringView::inferred("label11")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 10 },
                                ants::StyledStringView::inferred("label111")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 10 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |      auto main() -> int {
  |  _________^  ^  ^  ^
  | | ___________|  |  |
  | || _____________|  |
  | ||| _______________|
2 | ||||     std::cout << "Hello";
3 | ||||     std::cout << "World";
  | ||||          ^
  | ||||__________|
  | |||___________label1
  |  ||___________label1
  |   |___________label11
  |               label111)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 2, 5 },
                                ants::SourceLocation { 2, 8 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 10 }
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:3:6
  |
1 |   auto main() -> int {
  |  _________^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |      ^^^ ^
  | |______|___|
  |        label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 8 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 10 }
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^  ^
  | | _________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||        ^ ^
  | ||________|_|
  | |_________|
  |           label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 6 },
                                ants::StyledStringView::inferred("a long message")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 15 },
                                ants::SourceLocation { 1, 9 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |        ^  ^      ^
  |        |  |      |
  |        |  label1 |
  |  ______|_________|
  | |      a long message
2 | |     std::cout << "Hello";
  | |_________^ label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 6 },
                                ants::StyledStringView::inferred("a long message")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 15 },
                                ants::SourceLocation { 1, 9 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |        ^  ^      ^
  |        |  |      |
  |        |  label  |
  |        |  label1 |
  |  ______|_________|
  | |      a long message
2 | |     std::cout << "Hello";
  | |_________^ label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 6 },
                                ants::StyledStringView::inferred("a long message")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label11")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 15 },
                                ants::SourceLocation { 1, 9 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |        ^  ^      ^
  |  ______|__|______|
  | |      |  label1
  | |      |  label11
  | |      a long message
2 | |     std::cout << "Hello";
  | |_________^ label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 6 },
                                ants::StyledStringView::inferred("a long message")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 15 },
                                ants::SourceLocation { 0, 16 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 20 },
                                ants::SourceLocation { 1, 9 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |        ^  ^      ^    ^
  |        |  |      |    |
  |        |  label1 |    |
  |  ______|_________|____|
  | |      |         label2
  | |      a long message
2 | |     std::cout << "Hello";
  | |_________^ label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 6 },
                                ants::StyledStringView::inferred("a long message")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 15 },
                                ants::SourceLocation { 0, 16 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 22 },
                                ants::SourceLocation { 1, 9 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |        ^  ^      ^      ^
  |        |  |      |      |
  |        |  label1 label2 |
  |        a long message   |
  |  _______________________|
2 | |     std::cout << "Hello";
  | |_________^ label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 6 },
                                ants::StyledStringView::inferred("a loooooong message")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 0, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 15 },
                                ants::SourceLocation { 0, 16 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 22 },
                                ants::SourceLocation { 1, 9 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |        ^  ^      ^      ^
  |        |  |      |      |
  |        |  label1 label2 |
  |  ______|________________|
  | |      a loooooong message
2 | |     std::cout << "Hello";
  | |_________^ label3)"
    );
}

TEST(HumanRendererMultilineAnnotationTest, FoldLines) {
    std::string source = "auto main() -> int {\n";
    for (unsigned i = 0; i != 10; ++i) {
        source.append(";\n");
    }
    source.push_back('}');

    ants::HumanRenderer renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 4, 0 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
3 | | ;
4 | | ;
  | |__^ label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 5, 0 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
... |
4 | | ;
5 | | ;
  | |__^ label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 6, 0 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
... |
5 | | ;
6 | | ;
  | |__^ label1)"
    );

    renderer.max_multiline_annotation_line_num = 3;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 5, 0 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
... |
4 | | ;
5 | | ;
  | |__^ label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 6, 0 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
... |
5 | | ;
6 | | ;
  | |__^ label1)"
    );

    renderer.max_multiline_annotation_line_num = 2;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 1, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
  | |_^ label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 2, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
... |
3 | | ;
  | |_^ label1)"
    );

    renderer.max_multiline_annotation_line_num = 0;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 2, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
3 | | ;
  | |_^ label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
3 | | ;
4 | | ;
  | |_^ label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 4, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |   auto main() -> int {
  |  _^
2 | | ;
3 | | ;
4 | | ;
5 | | ;
  | |_^ label1)"
    );

    renderer.max_multiline_annotation_line_num = 4;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 1, 0 },
                                ants::SourceLocation { 4, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 5, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:2:1
  |
1 |    auto main() -> int {
  |  __^
2 | |  ;
  | | _^
3 | || ;
4 | || ;
5 | || ;
  | ||_^ label1
6 | |  ;
  | |__^ label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 1, 0 },
                                ants::SourceLocation { 5, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 6, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:2:1
  |
1 |    auto main() -> int {
  |  __^
2 | |  ;
  | | _^
3 | || ;
... ||
5 | || ;
6 | || ;
  | ||_^ label1
7 | |  ;
  | |__^ label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 6, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 3, 0 },
                                ants::SourceLocation { 9, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
  --> main.cpp:1:1
   |
 1 |    auto main() -> int {
   |  __^
 2 | |  ;
 3 | |  ;
 4 | |  ;
   | | _^
 5 | || ;
 6 | || ;
 7 | || ;
   | ||_^ label1
 8 |  | ;
 9 |  | ;
10 |  | ;
   |  |_^ label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 7, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 3, 0 },
                                ants::SourceLocation { 10, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
  --> main.cpp:1:1
   |
 1 |    auto main() -> int {
   |  __^
 2 | |  ;
 3 | |  ;
 4 | |  ;
   | | _^
 5 | || ;
...  ||
 7 | || ;
 8 | || ;
   | ||_^ label1
 9 |  | ;
10 |  | ;
11 |  | ;
   |  |_^ label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 7, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 3, 0 },
                                ants::SourceLocation { 11, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
  --> main.cpp:1:1
   |
 1 |    auto main() -> int {
   |  __^
 2 | |  ;
 3 | |  ;
 4 | |  ;
   | | _^
 5 | || ;
...  ||
 7 | || ;
 8 | || ;
   | ||_^ label1
 9 |  | ;
...   |
11 |  | ;
12 |  | }
   |  |_^ label2)"
    );
}

TEST(HumanRendererMultilineAnnotationTest, MultilineLabel) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello";
    std::cout << "World";
})";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("line3\nline4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||_________^ line1  ^
  |  |           line2  |
  |  |__________________|
  |                     line3
  |                     line4)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("line1\nline2\nline3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("line4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||_________^ line1  ^
  |  |           line2  |
  |  |           line3  |
  |  |__________________|
  |                     line4)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 11 },
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("line3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||           ^      ^
  | ||___________|      |
  |  |           line1  |
  |  |           line2  |
  |  |__________________|
  |                     line3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 11 },
                                ants::StyledStringView::inferred("line1\nline2\nline3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("line4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||           ^      ^
  | ||___________|      |
  |  |           line1  |
  |  |           line2  |
  |  |           line3  |
  |  |__________________|
  |                     line4)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("line3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 19 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |  _______^             ^
  | | ____________________|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||             ^    ^
  | ||_____________|____|
  | |              |    line1
  | |______________|    line2
  |                line3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("line3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 12 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |             ^ line1
  | |             | line2
  | |_____________|
  |               line3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("line4")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 12 },
                                ants::SourceLocation { 2, 13 },
                                ants::StyledStringView::inferred("line1\nline2\nline3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |             ^ line1
  | |             | line2
  | |             | line3
  | |_____________|
  |               line4)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 0, 10 },
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |    auto main() -> int {
  |         ^^^^^ line1
  |         |     line2
  |  _______|
  | | ______|
2 | ||     std::cout << "Hello";
3 | ||     std::cout << "World";
  | ||            ^     ^
  | ||____________|_____|
  | |_____________|     label1
  |               label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 18 },
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 8 },
                                ants::SourceLocation { 2, 15 },
                                ants::StyledStringView::inferred("line3\nline4\nline5")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 11 },
                                ants::SourceLocation { 2, 12 },
                                ants::StyledStringView::inferred("line6")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 14 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("line7\nline8")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 17 },
                                ants::SourceLocation { 2, 25 },
                                ants::StyledStringView::inferred("line10\nline11")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 17 },
                                ants::SourceLocation { 2, 28 },
                                ants::StyledStringView::inferred("line10")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |        auto main() -> int {
  |  ___________^  ^  ^  ^  ^
  | | _____________|  |  |  |
  | || _______________|  |  |
  | ||| _________________|  |
  | |||| ___________________|
  | ||||| __________________|
2 | ||||||     std::cout << "Hello";
3 | ||||||     std::cout << "World";
  | ||||||         ^  ^  ^  ^      ^  ^
  | ||||||_________|__|__|__|      |  |
  |  |||||         |  |  |  line1  |  |
  |  |||||_________|__|__|  line2  |  |
  |   ||||         |  |  line3     |  |
  |   ||||         |  |  line4     |  |
  |   ||||_________|__|  line5     |  |
  |    |||_________|  line6        |  |
  |     ||         line7           |  |
  |     ||         line8           |  |
  |     ||_________________________|__|
  |     |__________________________|  line10
  |                                line10
  |                                line11)"
    );
}

TEST(HumanRendererMultilineAnnotationTest, MergeAnnotation) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello";
    std::cout << "World";
})";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |_________^ label1
  |             label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 |   auto main() -> int {
  |  ______^
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |_________^ label1
  |             label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_secondary_annotation(
                                ants::SourceLocation { 0, 5 },
                                ants::SourceLocation { 2, 9 },
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp
  |
1 |   auto main() -> int {
  |  ______-
2 | |     std::cout << "Hello";
3 | |     std::cout << "World";
  | |_________- label1
  |             label2)"
    );
}
}  // namespace
