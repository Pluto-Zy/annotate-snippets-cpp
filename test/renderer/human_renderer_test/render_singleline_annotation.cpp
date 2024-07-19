#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/renderer/human_renderer.hpp"
#include "annotate_snippets/styled_string_view.hpp"
#include "level_for_test.hpp"

#include "gtest/gtest.h"

#include <string>
#include <string_view>

namespace {
TEST(HumanRendererSinglelineAnnotationTest, BasicFormat) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello World" << '\n';
    unsigned const result = 1 + 2;
    std::cout << result << '\n';
})";

    ants::HumanRenderer renderer;

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 1, 17 },
                                ants::SourceLocation { 1, 30 }
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |                  ^^^^^^^^^^^^^)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 1, 17 },
                                ants::SourceLocation { 1, 30 },
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |                  ^^^^^^^^^^^^^ This is a string literal.)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                38,
                                51,
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                            .with_secondary_annotation(
                                113,
                                119,
                                ants::StyledStringView::inferred("This is a variable.")
                            )

                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |                  ^^^^^^^^^^^^^ This is a string literal.
3 |     unsigned const result = 1 + 2;
4 |     std::cout << result << '\n';
  |                  ------ This is a variable.)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 1, 17 },
                                ants::SourceLocation { 1, 30 },
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("This is an object.")
                            )
                            .with_secondary_annotation(55, 59)
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------    ^^^^^^^^^^^^^    ----
  |     |            |
  |     |            This is a string literal.
  |     This is an object.)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                38,
                                51,
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("label")
                            )
                            .with_secondary_annotation(
                                55,
                                59,
                                ants::StyledStringView::inferred("This is a character.")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------    ^^^^^^^^^^^^^    ---- This is a character.
  |     |            |
  |     label        This is a string literal.)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(38, 51)
                            .with_secondary_annotation(25, 34)
                            .with_secondary_annotation(55, 59)
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------    ^^^^^^^^^^^^^    ----)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(38, 51, ants::StyledStringView::inferred("a"))
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("b")
                            )
                            .with_secondary_annotation(
                                55,
                                59,
                                ants::StyledStringView::inferred("c")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     --------- b  ^^^^^^^^^^^^^ a  ---- c)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("spaces"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(95, 96, ants::StyledStringView::inferred("space"))
                    )
            )
            .content(),
        R"(error: spaces
 --> main.cpp:3:35
  |
3 |     unsigned const result = 1 + 2;
  |                                   ^ space)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("spaces"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 2, 36 },
                                ants::SourceLocation { 2, 40 },
                                ants::StyledStringView::inferred("spaces")
                            )
                    )
            )
            .content(),
        R"(error: spaces
 --> main.cpp:3:37
  |
3 |     unsigned const result = 1 + 2;
  |                                     ^^^^^^ spaces)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("spaces"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(65, 66, ants::StyledStringView::inferred("l1"))
                            .with_annotation(66, 67, ants::StyledStringView::inferred("l2"))
                            .with_annotation(67, 68, ants::StyledStringView::inferred("l3"))
                            .with_annotation(69, 70, ants::StyledStringView::inferred("l4"))
                            .with_annotation(71, 72, ants::StyledStringView::inferred("l5"))
                            .with_annotation(74, 75, ants::StyledStringView::inferred("l6"))
                            .with_annotation(76, 77, ants::StyledStringView::inferred("l7"))
                            .with_annotation(78, 79, ants::StyledStringView::inferred("l8"))
                    )
            )
            .content(),
        R"(error: spaces
 --> main.cpp:3:5
  |
3 |     unsigned const result = 1 + 2;
  |     ^^^ ^ ^  ^ ^ ^ l8
  |     ||| | |  | |
  |     ||| | l5 | l7
  |     ||| l4   l6
  |     ||l3
  |     |l2
  |     l1)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, LabelPosition) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello World" << '\n';
    unsigned const result = 1 + 2;
    std::cout << result << '\n';
})";

    ants::HumanRenderer renderer { .label_position = ants::HumanRenderer::Right };

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                38,
                                51,
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("This is an object.")
                            )
                            .with_secondary_annotation(55, 59)
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------    ^^^^^^^^^^^^^    ----
  |             |                |
  |             |                This is a string literal.
  |             This is an object.)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                38,
                                51,
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("object")
                            )
                            .with_secondary_annotation(55, 59)
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------    ^^^^^^^^^^^^^    ----
  |             |                |
  |             object           This is a string literal.)"
    );

    renderer.label_position = ants::HumanRenderer::Left;

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                38,
                                51,
                                ants::StyledStringView::inferred("This is a string literal.")
                            )
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("label")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:18
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------    ^^^^^^^^^^^^^ This is a string literal.
  |     |
  |     label)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(33, 34, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                25,
                                33,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:13
  |
2 |     std::cout << "Hello World" << '\n';
  |     --------^
  |     |       |
  |     label2  label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(30, 32, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:10
  |
2 |     std::cout << "Hello World" << '\n';
  |     -----^^-- label2
  |          |
  |          label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(30, 37, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:10
  |
2 |     std::cout << "Hello World" << '\n';
  |     -----^^^^^^^
  |     |    |
  |     |    label1
  |     label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(25, 37, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:5
  |
2 |     std::cout << "Hello World" << '\n';
  |     ---------^^^ label1
  |     |
  |     label2)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, LineNumAlignment) {
    std::string source = "auto main() -> int {\n";
    for (unsigned i = 0; i != 99; ++i) {
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
                                ants::SourceLocation { 0, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 10, 0 },
                                ants::SourceLocation { 10, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 100, 0 },
                                ants::SourceLocation { 100, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
   --> main.cpp:1:1
    |
  1 | auto main() -> int {
    | ^ label1
...
 11 | ;
    | ^ label2
...
101 | }
    | ^ label3)"
    );

    renderer.line_num_alignment = ants::HumanRenderer::AlignLeft;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 0, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 10, 0 },
                                ants::SourceLocation { 10, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 100, 0 },
                                ants::SourceLocation { 100, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
   --> main.cpp:1:1
    |
1   | auto main() -> int {
    | ^ label1
...
11  | ;
    | ^ label2
...
101 | }
    | ^ label3)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, OmittedLine) {
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
                                ants::SourceLocation { 0, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 3, 0 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 7, 0 },
                                ants::SourceLocation { 7, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | auto main() -> int {
  | ^ label1
2 | ;
3 | ;
4 | ;
  | ^ label2
...
8 | ;
  | ^ label3)"
    );

    renderer.max_unannotated_line_num = 1;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 0, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 3, 0 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 7, 0 },
                                ants::SourceLocation { 7, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 9, 0 },
                                ants::SourceLocation { 9, 1 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
  --> main.cpp:1:1
   |
 1 | auto main() -> int {
   | ^ label1
...
 4 | ;
   | ^ label2
...
 8 | ;
   | ^ label3
 9 | ;
10 | ;
   | ^ label4)"
    );

    renderer.max_unannotated_line_num = 0;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                ants::SourceLocation { 0, 0 },
                                ants::SourceLocation { 0, 1 },
                                ants::StyledStringView::inferred("label1")
                            )
                            .with_annotation(
                                ants::SourceLocation { 2, 0 },
                                ants::SourceLocation { 2, 1 },
                                ants::StyledStringView::inferred("label2")
                            )
                            .with_annotation(
                                ants::SourceLocation { 3, 0 },
                                ants::SourceLocation { 3, 1 },
                                ants::StyledStringView::inferred("label3")
                            )
                            .with_annotation(
                                ants::SourceLocation { 5, 0 },
                                ants::SourceLocation { 5, 1 },
                                ants::StyledStringView::inferred("label4")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | auto main() -> int {
  | ^ label1
...
3 | ;
  | ^ label2
4 | ;
  | ^ label3
...
6 | ;
  | ^ label4)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, Underline) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello World" << '\n';
})";

    ants::HumanRenderer renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_secondary_annotation(0, 4)
                            .with_secondary_annotation(5, 9)
                            .with_annotation(0, 18)
                            .with_secondary_annotation(15, 20)
                            .with_annotation(38, 51)
                            .with_secondary_annotation(38, 51)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | auto main() -> int {
  | ----^----^^^^^^^^^--
2 |     std::cout << "Hello World" << '\n';
  |                  ^^^^^^^^^^^^^)"
    );

    renderer.primary_underline = '~';
    renderer.secondary_underline = '^';

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_secondary_annotation(0, 4)
                            .with_secondary_annotation(5, 9)
                            .with_annotation(0, 18)
                            .with_secondary_annotation(15, 20)
                            .with_annotation(38, 51)
                            .with_secondary_annotation(38, 51)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | auto main() -> int {
  | ^^^^~^^^^~~~~~~~~~^^
2 |     std::cout << "Hello World" << '\n';
  |                  ~~~~~~~~~~~~~)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, MultilineLabel) {
    std::string_view const source = R"(auto main() -> int {
    std::cout << "Hello World" << '\n';
    unsigned const result = 1 + 2;
    std::cout << result << '\n';
})";

    ants::HumanRenderer renderer;

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                25,
                                34,
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                            .with_secondary_annotation(
                                55,
                                59,
                                ants::StyledStringView::inferred("line1\nline2\nline3")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:5
  |
2 |     std::cout << "Hello World" << '\n';
  |     ^^^^^^^^^ line1               ---- line1
  |               line2                    line2
  |                                        line3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                50,
                                51,
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                            .with_secondary_annotation(
                                55,
                                59,
                                ants::StyledStringView::inferred("line1\nline2\nline3")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:30
  |
2 |     std::cout << "Hello World" << '\n';
  |                              ^    ---- line1
  |                              |         line2
  |                              |         line3
  |                              line1
  |                              line2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("string literal"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                50,
                                51,
                                ants::StyledStringView::inferred("line1\nline2")
                            )
                            .with_secondary_annotation(55, 59)
                            .with_annotation(
                                47,
                                48,
                                ants::StyledStringView::inferred("line1\nline2\nline3")
                            )
                    )
            )
            .content(),
        R"(warning: string literal
 --> main.cpp:2:30
  |
2 |     std::cout << "Hello World" << '\n';
  |                           ^  ^    ----
  |                           |  |
  |                           |  line1
  |                           |  line2
  |                           line1
  |                           line2
  |                           line3)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, MergeAnnotation) {
    std::string_view const source = "func(args)";
    ants::HumanRenderer renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(0, 4)
                            .with_secondary_annotation(0, 4)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | func(args)
  | ^^^^)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_secondary_annotation(0, 4)
                            .with_secondary_annotation(0, 4)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp
  |
1 | func(args)
  | ----)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(0, 4, ants::StyledStringView::inferred("label"))
                            .with_secondary_annotation(0, 4)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | func(args)
  | ^^^^ label)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(0, 4, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                0,
                                4,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | func(args)
  | ^^^^ label1
  |      label2)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(
                                0,
                                4,
                                ants::StyledStringView::inferred("label1\nlabel2")
                            )
                            .with_secondary_annotation(
                                0,
                                4,
                                ants::StyledStringView::inferred("label3")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | func(args)
  | ^^^^ label1
  |      label2
  |      label3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(0, 4)
                            .with_secondary_annotation(
                                0,
                                4,
                                ants::StyledStringView::inferred("label1\nlabel2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 | func(args)
  | ^^^^ label1
  |      label2)"
    );
}

TEST(HumanRendererSinglelineAnnotationTest, SourceNormalization) {
    std::string_view const source = "\tfunc(args1,\targs2)";
    ants::HumanRenderer renderer;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(0, 4, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                5,
                                19,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |     func(args1,    args2)
  | ^^^^^^^ ----------------- label2
  | |
  | label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp").with_annotation(6, 12)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:7
  |
1 |     func(args1,    args2)
  |          ^^^^^^)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp").with_annotation(6, 13)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:7
  |
1 |     func(args1,    args2)
  |          ^^^^^^^^^^)"
    );

    renderer.display_tab_width = 8;

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp")
                            .with_annotation(0, 4, ants::StyledStringView::inferred("label1"))
                            .with_secondary_annotation(
                                5,
                                19,
                                ants::StyledStringView::inferred("label2")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:1
  |
1 |         func(args1,        args2)
  | ^^^^^^^^^^^ --------------------- label2
  | |
  | label1)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp").with_annotation(6, 12)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:7
  |
1 |         func(args1,        args2)
  |              ^^^^^^)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource(source, "main.cpp").with_annotation(6, 13)
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:7
  |
1 |         func(args1,        args2)
  |              ^^^^^^^^^^^^^^)"
    );
}
}  // namespace
