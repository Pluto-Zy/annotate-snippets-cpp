#ifndef ANNOTATE_SNIPPETS_TEST_RENDERER_HUMAN_RENDERER_TEST_LEVEL_FOR_TEST_HPP
#define ANNOTATE_SNIPPETS_TEST_RENDERER_HUMAN_RENDERER_TEST_LEVEL_FOR_TEST_HPP

enum class Level {
    Fatal,
    Error,
    Warning,
    Note,
    Help,
};

inline auto title(Level level) -> char const* {
    switch (level) {
    case Level::Fatal:
        return "fatal error";
    case Level::Error:
        return "error";
    case Level::Warning:
        return "warning";
    case Level::Note:
        return "note";
    case Level::Help:
        return "help";
    default:
        return "";
    }
}

#endif  // ANNOTATE_SNIPPETS_TEST_RENDERER_HUMAN_RENDERER_TEST_LEVEL_FOR_TEST_HPP
