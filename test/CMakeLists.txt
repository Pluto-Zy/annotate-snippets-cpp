include(AddGoogleTest)

add_executable(annotate_snippets_tests
    styled_string_view_test.cpp
    styled_string_test.cpp
    annotated_source_test.cpp

    renderer/human_renderer_test/render_title_message.cpp
    renderer/human_renderer_test/render_singleline_annotation.cpp
    renderer/human_renderer_test/render_multiline_annotation.cpp
)
target_link_libraries(annotate_snippets_tests PRIVATE annotate_snippets gtest_main)
gtest_discover_tests(annotate_snippets_tests)
