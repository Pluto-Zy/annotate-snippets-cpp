#ifndef ANNOTATE_SNIPPETS_DETAIL_DIAG_DIAG_ENTRY_IMPL_HPP
#define ANNOTATE_SNIPPETS_DETAIL_DIAG_DIAG_ENTRY_IMPL_HPP

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/diag/level.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ants::detail {
/// Used in the implementation of `DiagEntry` and `Diag` to provide a consistent interface and
/// facilitate chainable calls that behave appropriately.
///
/// For more information, refer to the documentation comments of `DiagEntry`.
template <class Level, class Derived>
class DiagEntryImpl {
    static_assert(
        is_diagnostic_level<Level>,
        "The `Level` type cannot be used as a diagnostic level type, it must have a `title()` "
        "member function or an `title()` function found via ADL."
    );

public:
    DiagEntryImpl() : level_() { }

    explicit DiagEntryImpl(Level level) : level_(std::move(level)) { }

    explicit DiagEntryImpl(Level level, StyledStringView diag_message) :
        level_(std::move(level)), diag_message_(std::move(diag_message)) { }

    explicit DiagEntryImpl(Level level, StyledStringView diag_message, std::string_view err_code) :
        level_(std::move(level)), err_code_(err_code), diag_message_(std::move(diag_message)) { }

    auto level() const -> Level const& {
        return level_;
    }

    auto level() -> Level& {
        return level_;
    }

    void set_level(Level level) {
        level_ = std::move(level);
    }

    auto with_level(Level level) & -> Derived& {
        set_level(std::move(level));
        return static_cast<Derived&>(*this);
    }

    auto with_level(Level level) && -> Derived&& {
        set_level(std::move(level));
        return static_cast<Derived&&>(*this);
    }

    auto error_code() const -> std::string_view {
        return err_code_;
    }

    void set_error_code(std::string_view err_code) {
        err_code_ = err_code;
    }

    auto with_error_code(std::string_view err_code) & -> Derived& {
        set_error_code(err_code);
        return static_cast<Derived&>(*this);
    }

    auto with_error_code(std::string_view err_code) && -> Derived&& {
        set_error_code(err_code);
        return static_cast<Derived&&>(*this);
    }

    auto diag_message() const -> StyledStringView const& {
        return diag_message_;
    }

    auto diag_message() -> StyledStringView& {
        return diag_message_;
    }

    void set_diag_message(StyledStringView message) {
        diag_message_ = std::move(message);
    }

    auto with_diag_message(StyledStringView message) & -> Derived& {
        set_diag_message(std::move(message));
        return static_cast<Derived&>(*this);
    }

    auto with_diag_message(StyledStringView message) && -> Derived&& {
        set_diag_message(std::move(message));
        return static_cast<Derived&&>(*this);
    }

    auto associated_sources() const -> std::vector<AnnotatedSource> const& {
        return associated_sources_;
    }

    auto associated_sources() -> std::vector<AnnotatedSource>& {
        return associated_sources_;
    }

    void add_source(AnnotatedSource source) {
        associated_sources_.push_back(std::move(source));
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<AnnotatedSource, Args...>, int> = 0>
    void add_source(Args&&... args) {
        associated_sources_.emplace_back(std::forward<Args>(args)...);
    }

    auto with_source(AnnotatedSource source) & -> Derived& {
        add_source(std::move(source));
        return static_cast<Derived&>(*this);
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<AnnotatedSource, Args...>, int> = 0>
    auto with_source(Args&&... args) & -> Derived& {
        add_source(std::forward<Args>(args)...);
        return static_cast<Derived&>(*this);
    }

    auto with_source(AnnotatedSource source) && -> Derived&& {
        add_source(std::move(source));
        return static_cast<Derived&&>(*this);
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<AnnotatedSource, Args...>, int> = 0>
    auto with_source(Args&&... args) && -> Derived&& {
        add_source(std::forward<Args>(args)...);
        return static_cast<Derived&&>(*this);
    }

private:
    /// The error level of the current diagnostic entry (such as error, warning, note, or help).
    Level level_;
    /// The error code of the current diagnostic entry (optional; if empty, it is not displayed in
    /// the rendered results).
    std::string_view err_code_;
    /// The title message text of the diagnostic entry. It is displayed in front of all annotated
    /// source codes associated with this diagnostic entry.
    StyledStringView diag_message_;
    /// The annotated source codes associated with this diagnostic entry. The diagnostic entry may
    /// not be associated with any source code, for example, "note:" usually appears as a secondary
    /// diagnostic entry and is not associated with any source code (thus only displaying the title
    /// information).
    std::vector<AnnotatedSource> associated_sources_;
};
}  // namespace ants::detail

#endif  // ANNOTATE_SNIPPETS_DETAIL_DIAG_DIAG_ENTRY_IMPL_HPP
