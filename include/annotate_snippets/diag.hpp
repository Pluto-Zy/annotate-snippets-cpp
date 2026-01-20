#ifndef ANNOTATE_SNIPPETS_DIAG_HPP
#define ANNOTATE_SNIPPETS_DIAG_HPP

#include "annotate_snippets/annotated_source.hpp"
#include "annotate_snippets/detail/diag/diag_entry_builder.hpp"
#include "annotate_snippets/detail/diag/level.hpp"
#include "annotate_snippets/styled_string_view.hpp"

#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ants {
template <class Level>
class Diag;

/// Represents a diagnostic entry in a complete diagnostic message. It combines several annotated
/// source codes with an overall diagnostic level and title information. A complete diagnostic
/// message consists of one or more such entries. Each diagnostic entry can specify:
///
/// 1. The level of the diagnostic entry (such as error, warning, or note).
/// 2. An error code (optional).
/// 3. The title text of the diagnostic entry.
/// 4. All the annotated source codes associated with this diagnostic entry (optional).
///
/// TODO: Add an example to explain these terms.
template <class Level>
class DiagEntry : public detail::DiagEntryBuilder<Level, DiagEntry<Level>> {
    using Base = detail::DiagEntryBuilder<Level, DiagEntry>;

public:
    DiagEntry() = default;
    explicit DiagEntry(Level level) : level_(std::move(level)) { }
    explicit DiagEntry(Level level, StyledStringView diag_message) :
        level_(std::move(level)), diag_message_(std::move(diag_message)) { }
    explicit DiagEntry(Level level, StyledStringView diag_message, std::string_view err_code) :
        level_(std::move(level)), err_code_(err_code), diag_message_(std::move(diag_message)) { }

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

    /// Provide access to `Base` for forwarding calls.
    friend Base;
    /// Provide access to `Diag` for accessing primary diagnostic entry information.
    friend class Diag<Level>;

    template <class Self>
    static auto get_level(Self&& self) -> decltype(auto) {
        return (self.level_);
    }

    template <class Self>
    static auto get_error_code(Self&& self) -> decltype(auto) {
        return (self.err_code_);
    }

    template <class Self>
    static auto get_diag_message(Self&& self) -> decltype(auto) {
        return (self.diag_message_);
    }

    template <class Self>
    static auto get_associated_sources(Self&& self) -> decltype(auto) {
        return (self.associated_sources_);
    }
};

// Deduction guide for `DiagEntry<>`.
template <class Level, class... Args, std::enable_if_t<detail::is_diagnostic_level<Level>, int> = 0>
DiagEntry(Level, Args&&...) -> DiagEntry<Level>;

/// Represents a complete diagnostic, consisting of several diagnostic entries (one primary
/// diagnostic and several secondary diagnostics), which are rendered in sequence. `Diag` is the
/// unit accepted by the renderer.
template <class Level>
class Diag : public detail::DiagEntryBuilder<Level, Diag<Level>> {
    using Base = detail::DiagEntryBuilder<Level, Diag>;

public:
    Diag() = default;
    explicit Diag(Level level) : primary_diag_(std::move(level)) { }
    explicit Diag(Level level, StyledStringView diag_message) :
        primary_diag_(std::move(level), std::move(diag_message)) { }
    explicit Diag(Level level, StyledStringView diag_message, std::string_view err_code) :
        primary_diag_(std::move(level), std::move(diag_message), err_code) { }

    auto primary_diag_entry() const -> DiagEntry<Level> const& {
        return primary_diag_;
    }

    auto primary_diag_entry() -> DiagEntry<Level>& {
        return primary_diag_;
    }

    auto secondary_diag_entries() const -> std::vector<DiagEntry<Level>> const& {
        return secondary_diags_;
    }

    auto secondary_diag_entries() -> std::vector<DiagEntry<Level>>& {
        return secondary_diags_;
    }

    void add_sub_diag_entry(DiagEntry<Level> entry) {
        secondary_diags_.push_back(std::move(entry));
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<DiagEntry<Level>, Args...>, int> = 0>
    void add_sub_diag_entry(Args&&... args) {
        secondary_diags_.emplace_back(std::forward<Args>(args)...);
    }

    auto with_sub_diag_entry(DiagEntry<Level> entry) & -> Diag& {
        add_sub_diag_entry(std::move(entry));
        return *this;
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<DiagEntry<Level>, Args...>, int> = 0>
    auto with_sub_diag_entry(Args&&... args) & -> Diag& {
        add_sub_diag_entry(std::forward<Args>(args)...);
        return *this;
    }

    auto with_sub_diag_entry(DiagEntry<Level> entry) && -> Diag&& {
        add_sub_diag_entry(std::move(entry));
        return std::move(*this);
    }

    template <
        class... Args,
        std::enable_if_t<std::is_constructible_v<DiagEntry<Level>, Args...>, int> = 0>
    auto with_sub_diag_entry(Args&&... args) && -> Diag&& {
        add_sub_diag_entry(std::forward<Args>(args)...);
        return std::move(*this);
    }

private:
    /// The primary diagnostic entry.
    DiagEntry<Level> primary_diag_;
    /// The secondary diagnostic entries.
    std::vector<DiagEntry<Level>> secondary_diags_;

    /// Provide access to `Base` for forwarding calls.
    friend Base;

    template <class Self>
    static auto get_level(Self&& self) -> decltype(auto) {
        return (self.primary_diag_.level_);
    }

    template <class Self>
    static auto get_error_code(Self&& self) -> decltype(auto) {
        return (self.primary_diag_.err_code_);
    }

    template <class Self>
    static auto get_diag_message(Self&& self) -> decltype(auto) {
        return (self.primary_diag_.diag_message_);
    }

    template <class Self>
    static auto get_associated_sources(Self&& self) -> decltype(auto) {
        return (self.primary_diag_.associated_sources_);
    }
};

// Deduction guide for `Diag<>`.
template <class Level, class... Args, std::enable_if_t<detail::is_diagnostic_level<Level>, int> = 0>
Diag(Level, Args&&...) -> Diag<Level>;
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_DIAG_HPP
