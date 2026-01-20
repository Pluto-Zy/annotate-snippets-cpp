#ifndef ANNOTATE_SNIPPETS_DETAIL_DIAG_DIAG_ENTRY_BUILDER_HPP
#define ANNOTATE_SNIPPETS_DETAIL_DIAG_DIAG_ENTRY_BUILDER_HPP

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
/// `DiagEntryBuilder` provides a series of chainable methods for building diagnostic entries. These
/// methods are forwarded to the corresponding methods of the `Derived` type. `Derived` must provide
/// the following member functions, and their non-const overloads must return references to the
/// corresponding member variables to ensure that their values can be modified:
///
/// - `get_level()`: Returns the error level of the current diagnostic entry.
/// - `get_error_code()`: Returns the error code of the current diagnostic entry.
/// - `get_diag_message()`: Returns the title message text of the current diagnostic entry.
/// - `get_associated_sources()`: Returns the list of annotated source codes associated with the
///   current diagnostic entry.
///
/// To reduce the number of member functions that need to be provided in the derived class,
/// `DiagEntryBuilder` always passes `self()` as the argument to these member functions.
template <class Level, class Derived>
class DiagEntryBuilder {
    static_assert(
        is_diagnostic_level<Level>,
        "The `Level` type cannot be used as a diagnostic level type, it must have a `title()` "
        "member function or an `title()` function found via ADL."
    );

    auto self() const -> Derived const& {
        return static_cast<Derived const&>(*this);
    }

    auto self() -> Derived& {
        return static_cast<Derived&>(*this);
    }

public:
    auto level() const -> Level const& {
        return Derived::get_level(self());
    }

    auto level() -> Level& {
        return Derived::get_level(self());
    }

    void set_level(Level level) {
        Derived::get_level(self()) = std::move(level);
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
        return Derived::get_error_code(self());
    }

    void set_error_code(std::string_view err_code) {
        Derived::get_error_code(self()) = err_code;
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
        return Derived::get_diag_message(self());
    }

    auto diag_message() -> StyledStringView& {
        return Derived::get_diag_message(self());
    }

    void set_diag_message(StyledStringView message) {
        Derived::get_diag_message(self()) = std::move(message);
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
        return Derived::get_associated_sources(self());
    }

    auto associated_sources() -> std::vector<AnnotatedSource>& {
        return Derived::get_associated_sources(self());
    }

    void add_source(AnnotatedSource source) {
        Derived::get_associated_sources(self()).push_back(std::move(source));
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<AnnotatedSource, Args...>, int> = 0>
    void add_source(Args&&... args) {
        Derived::get_associated_sources(self()).emplace_back(std::forward<Args>(args)...);
    }

    auto with_source(AnnotatedSource source) & -> Derived& {
        add_source(std::move(source));
        return static_cast<Derived&>(*this);
    }

    auto with_source(AnnotatedSource source) && -> Derived&& {
        add_source(std::move(source));
        return static_cast<Derived&&>(*this);
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<AnnotatedSource, Args...>, int> = 0>
    auto with_source(Args&&... args) & -> Derived& {
        add_source(std::forward<Args>(args)...);
        return static_cast<Derived&>(*this);
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<AnnotatedSource, Args...>, int> = 0>
    auto with_source(Args&&... args) && -> Derived&& {
        add_source(std::forward<Args>(args)...);
        return static_cast<Derived&&>(*this);
    }
};
}  // namespace ants::detail

#endif  // ANNOTATE_SNIPPETS_DETAIL_DIAG_DIAG_ENTRY_BUILDER_HPP
