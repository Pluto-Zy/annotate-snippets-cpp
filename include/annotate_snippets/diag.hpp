#ifndef ANNOTATE_SNIPPETS_DIAG_HPP
#define ANNOTATE_SNIPPETS_DIAG_HPP

#include "annotate_snippets/detail/diag/diag_entry_impl.hpp"
#include "annotate_snippets/detail/diag/level.hpp"

#include <concepts>
#include <vector>

namespace ants {
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
template <detail::diagnostic_level Level>
class DiagEntry : public detail::DiagEntryImpl<Level, DiagEntry<Level>> {
    using detail::DiagEntryImpl<Level, DiagEntry>::DiagEntryImpl;
};

// Deduction guide for `DiagEntry<>`.
template <detail::diagnostic_level Level, class... Args>
DiagEntry(Level, Args&&...) -> DiagEntry<Level>;

/// Represents a complete diagnostic, consisting of several diagnostic entries (one primary
/// diagnostic and several secondary diagnostics), which are rendered in sequence. `Diag` is the
/// unit accepted by the renderer.
template <detail::diagnostic_level Level>
class Diag : public detail::DiagEntryImpl<Level, Diag<Level>> {
    using DiagEntryImpl = detail::DiagEntryImpl<Level, Diag>;
    using Base = DiagEntryImpl;

public:
    using Base::Base;

    auto primary_diag_entry() const -> DiagEntryImpl const& {
        return static_cast<DiagEntryImpl const&>(*this);
    }

    auto primary_diag_entry() -> DiagEntryImpl& {
        return static_cast<DiagEntryImpl&>(*this);
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

    template <class... Args>
        requires std::constructible_from<DiagEntry<Level>, Args...>
    void add_sub_diag_entry(Args&&... args) {
        secondary_diags_.emplace_back(std::forward<Args>(args)...);
    }

    auto with_sub_diag_entry(DiagEntry<Level> entry) & -> Diag& {
        add_sub_diag_entry(std::move(entry));
        return *this;
    }

    template <class... Args>
        requires std::constructible_from<DiagEntry<Level>, Args...>
    auto with_sub_diag_entry(Args&&... args) & -> Diag& {
        add_sub_diag_entry(std::forward<Args>(args)...);
        return *this;
    }

    auto with_sub_diag_entry(DiagEntry<Level> entry) && -> Diag&& {
        add_sub_diag_entry(std::move(entry));
        return std::move(*this);
    }

    template <class... Args>
        requires std::constructible_from<DiagEntry<Level>, Args...>
    auto with_sub_diag_entry(Args&&... args) && -> Diag&& {
        add_sub_diag_entry(std::forward<Args>(args)...);
        return std::move(*this);
    }

private:
    std::vector<DiagEntry<Level>> secondary_diags_;
};

// Deduction guide for `Diag<>`.
template <detail::diagnostic_level Level, class... Args>
Diag(Level, Args&&...) -> Diag<Level>;
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_DIAG_HPP
