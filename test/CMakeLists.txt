CPMAddPackage(
    URI "gh:google/googletest@1.14.0"
    OPTIONS
        # We don't expect to install googletest along with the project.
        "INSTALL_GTEST OFF"
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        "gtest_force_shared_crt ON"
)
include(GoogleTest)

add_executable(annotate_snippets_tests
    source_file_test.cpp
    source_location_test.cpp
    styled_string_view_test.cpp
    styled_string_test.cpp
    style_spec_test.cpp

    renderer/human_renderer_test/render_title_message.cpp
    renderer/human_renderer_test/render_singleline_annotation.cpp
    renderer/human_renderer_test/render_multiline_annotation.cpp
    renderer/human_renderer_test/render_multiple_diag_entries.cpp
    renderer/human_renderer_test/render_suggestion.cpp
)
target_link_libraries(annotate_snippets_tests PRIVATE annotate_snippets gtest_main)
gtest_discover_tests(annotate_snippets_tests)
