#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/diag.hpp"
#include "annotate_snippets/renderer/human_renderer.hpp"
#include "annotate_snippets/styled_string_view.hpp"
#include "level_for_test.hpp"

#include "gtest/gtest.h"

#include <string_view>

namespace {
TEST(HumanRendererMultipleDiagTest, MultipleSources) {
    std::string_view const source1 = "auto main() -> int {}";
    std::string_view const source2 = "auto add(int v1, int v2) -> int { return v1 + v2; }";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_source(
                        ants::AnnotatedSource(source2, "add.cpp")
                            .with_annotation(5, 8, ants::StyledStringView::inferred("function"))
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
  |
 ::: add.cpp:1:6
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |      ^^^ function)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_source(
                        ants::AnnotatedSource(source2, "add.cpp")
                            .with_annotation(5, 8, ants::StyledStringView::inferred("function"))
                    )
                    .with_source(  //
                        ants::AnnotatedSource(source2, "add.cpp")
                            .with_secondary_annotation(
                                28,
                                31,
                                ants::StyledStringView::inferred("type")
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
  |
 ::: add.cpp:1:6
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |      ^^^ function
  |
 ::: add.cpp
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |                             --- type)"
    );
}

TEST(HumanRendererMultipleDiagTest, MultipleEntries) {
    std::string_view const source1 = "auto main() -> int {}";
    std::string_view const source2 = "auto add(int v1, int v2) -> int { return v1 + v2; }";

    ants::HumanRenderer const renderer;

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_sub_diag_entry(ants::DiagEntry(
                        Level::Note,
                        ants::StyledStringView::inferred("note something")
                    ))
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
  = note: note something)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_sub_diag_entry(ants::DiagEntry(
                        Level::Note,
                        ants::StyledStringView::inferred("note something")
                    ))
                    .with_sub_diag_entry(ants::DiagEntry(
                        Level::Help,
                        ants::StyledStringView::inferred("help something")
                    ))
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
  = note: note something
  = help: help something)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_sub_diag_entry(ants::DiagEntry(
                        Level::Note,
                        ants::StyledStringView::inferred("note something")
                    ))
                    .with_sub_diag_entry(ants::DiagEntry(
                        Level::Help,
                        ants::StyledStringView::inferred("line1\nline2")
                    ))
                    .with_sub_diag_entry(
                        ants::DiagEntry(Level::Help, ants::StyledStringView::inferred("line3"))
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
  = note: note something
  = help: line1
          line2
  = help: line3)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_sub_diag_entry(  //
                        ants::DiagEntry(
                            Level::Note,
                            ants::StyledStringView::inferred("note something")
                        )
                            .with_source(  //
                                ants::AnnotatedSource(source2, "add.cpp")
                                    .with_annotation(
                                        5,
                                        8,
                                        ants::StyledStringView::inferred("function")
                                    )
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
note: note something
 --> add.cpp:1:6
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |      ^^^ function)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_sub_diag_entry(  //
                        ants::DiagEntry(
                            Level::Note,
                            ants::StyledStringView::inferred("note something")
                        )
                            .with_source(  //
                                ants::AnnotatedSource(source2, "add.cpp")
                                    .with_annotation(
                                        5,
                                        8,
                                        ants::StyledStringView::inferred("function")
                                    )
                            )
                            .with_source(  //
                                ants::AnnotatedSource(source1, "main.cpp")
                                    .with_annotation(
                                        15,
                                        18,
                                        ants::StyledStringView::inferred("type")
                                    )
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
note: note something
 --> add.cpp:1:6
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |      ^^^ function
  |
 ::: main.cpp:1:16
  |
1 | auto main() -> int {}
  |                ^^^ type)"
    );

    EXPECT_EQ(
        renderer
            .render_diag(
                ants::Diag(Level::Error, ants::StyledStringView::inferred("message"))
                    .with_source(
                        ants::AnnotatedSource(source1, "main.cpp")
                            .with_annotation(5, 9, ants::StyledStringView::inferred("function"))
                    )
                    .with_source(  //
                        ants::AnnotatedSource(source2, "add.cpp")
                            .with_annotation(
                                28,
                                31,
                                ants::StyledStringView::inferred("type")
                            )
                    )
                    .with_sub_diag_entry(  //
                        ants::DiagEntry(
                            Level::Note,
                            ants::StyledStringView::inferred("note something")
                        )
                            .with_source(  //
                                ants::AnnotatedSource(source2, "add.cpp")
                                    .with_annotation(
                                        5,
                                        8,
                                        ants::StyledStringView::inferred("function")
                                    )
                            )
                            .with_source(  //
                                ants::AnnotatedSource(source1, "main.cpp")
                                    .with_annotation(
                                        15,
                                        18,
                                        ants::StyledStringView::inferred("type")
                                    )
                            )
                    )
            )
            .content(),
        R"(error: message
 --> main.cpp:1:6
  |
1 | auto main() -> int {}
  |      ^^^^ function
  |
 ::: add.cpp:1:29
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |                             ^^^ type
note: note something
 --> add.cpp:1:6
  |
1 | auto add(int v1, int v2) -> int { return v1 + v2; }
  |      ^^^ function
  |
 ::: main.cpp:1:16
  |
1 | auto main() -> int {}
  |                ^^^ type)"
    );
}
}  // namespace
