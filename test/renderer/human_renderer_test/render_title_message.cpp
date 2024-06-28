#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/diag.hpp"
#include "annotate_snippets/renderer/human_renderer.hpp"
#include "annotate_snippets/styled_string_view.hpp"
#include "level_for_test.hpp"

#include "gtest/gtest.h"

namespace {
TEST(HumanRendererTitleMessageTest, SingleDiagEntry) {
    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(ants::Diag(Level::Error, ants::StyledStringView::inferred("message")))
            .content(),
        "error: message"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(
                ants::Diag(Level::Warning, ants::StyledStringView::inferred("warning message"))
            )
            .content(),
        "warning: warning message"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(ants::Diag(Level::Fatal, ants::StyledStringView::inferred("hello world")))
            .content(),
        "fatal error: hello world"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(  //
                ants::Diag(Level::Error)
                    .with_diag_message(ants::StyledStringView::inferred("error message"))
                    .with_error_code("E0001")
            )
            .content(),
        "error[E0001]: error message"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(
                ants::Diag(Level::Fatal)
                    .with_diag_message(ants::StyledStringView::inferred("fatal error message"))
                    .with_error_code("E1")
            )
            .content(),
        "fatal error[E1]: fatal error message"
    );
}

TEST(HumanRendererTitleMessageTest, MultilineDiagMessage) {
    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(
                ants::Diag(Level::Error)
                    .with_diag_message(ants::StyledStringView::inferred("message1\nmessage2"))
            )
            .content(),
        //                error:
        "error: message1\n       message2"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(  //
                ants::Diag(Level::Fatal)
                    .with_diag_message(
                        ants::StyledStringView::inferred("long message1\nmessage2\nshort msg3")
                    )
            )
            .content(),
        R"(fatal error: long message1
             message2
             short msg3)"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(  //
                ants::Diag(Level::Fatal)
                    .with_diag_message(
                        ants::StyledStringView::inferred("long message1\nmessage2\nshort msg3")
                    )
                    .with_error_code("E001")
            )
            .content(),
        R"(fatal error[E001]: long message1
                   message2
                   short msg3)"
    );
}

TEST(HumanRendererTitleMessageTest, MultipleDiagEntries) {
    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(  //
                ants::Diag(Level::Error)
                    .with_diag_message(ants::StyledStringView::inferred("primary"))
                    .with_error_code("ABC")
                    .with_sub_diag_entry(
                        ants::DiagEntry(Level::Note)
                            .with_diag_message(ants::StyledStringView::inferred("secondary"))
                    )
            )
            .content(),
        R"(error[ABC]: primary
 = note: secondary)"
    );

    EXPECT_EQ(
        ants::HumanRenderer()
            .render_diag(  //
                ants::Diag(Level::Error)
                    .with_diag_message(ants::StyledStringView::inferred("message1\nmessage2"))
                    .with_error_code("ABC")
                    .with_sub_diag_entry(  //
                        ants::DiagEntry(Level::Help)
                            .with_diag_message(
                                ants::StyledStringView::inferred("line1\nlong message")
                            )
                    )
            )
            .content(),
        R"(error[ABC]: message1
            message2
 = help: line1
         long message)"
    );
}

TEST(HumanRendererTitleMessageTest, ShortMessage) {
    ants::HumanRenderer render { .short_message = true };

    EXPECT_EQ(
        render.render_diag(ants::Diag(Level::Error, ants::StyledStringView::inferred("message")))
            .content(),
        "error: message"
    );

    EXPECT_EQ(
        render
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(ants::AnnotatedSource("source", "main.cpp").with_annotation(3, 4))
            )
            .content(),
        "main.cpp:1:4: error: message"
    );

    EXPECT_EQ(
        render
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource("source", "main.cpp")
                            .with_annotation(2, 4)
                            .with_annotation(3, 4)
                    )
            )
            .content(),
        "main.cpp:1:3: error: message"
    );

    EXPECT_EQ(
        render
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(ants::AnnotatedSource("source", "main.cpp").with_annotation(2, 4))
                    .with_source(
                        ants::AnnotatedSource("sou\nrce", "lib.cpp").with_annotation(4, 100)
                    )
            )
            .content(),
        "main.cpp:1:3: \nlib.cpp:2:1: error: message"
    );

    EXPECT_EQ(
        render
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(  //
                        ants::AnnotatedSource("source", "main.cpp")
                            .with_first_line_number(2)
                            .with_annotation(2, 4)
                    )
                    .with_source(  //
                        ants::AnnotatedSource("sou\nrce", "lib.cpp")
                            .with_first_line_number(100)
                            .with_annotation(4, 100)
                    )
            )
            .content(),
        "main.cpp:2:3: \nlib.cpp:101:1: error: message"
    );

    EXPECT_EQ(
        render
            .render_diag(  //
                ants::Diag(Level::Error, ants::StyledStringView::inferred("line\nmessage"))
                    .with_source(  //
                        ants::AnnotatedSource("source", "main.cpp")
                            .with_first_line_number(2)
                            .with_annotation(2, 4)
                    )
                    .with_source(  //
                        ants::AnnotatedSource("sou\nrce", "lib.cpp")
                            .with_first_line_number(100)
                            .with_annotation(4, 100)
                    )
            )
            .content(),
        "main.cpp:2:3: \nlib.cpp:101:1: error: line\n                      message"
    );
}
}  // namespace
