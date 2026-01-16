#ifndef ANNOTATE_SNIPPETS_SOURCE_LOCATION_HPP
#define ANNOTATE_SNIPPETS_SOURCE_LOCATION_HPP

#include "annotate_snippets/styled_string_view.hpp"

#include <cassert>
#include <cstddef>

namespace ants {
/// Represents the location of a *byte* in the source code.
///
/// There are two ways to specify a location in the source code: line and column numbers, and byte
/// offset. When constructing diagnostic information, users often only need to provide one of these
/// two types of location information. The other type of location information will be filled in only
/// when the diagnostic is emitted. This allows users to build diagnostic information more quickly
/// and flexibly. The maximum value of the corresponding field is used to indicate that the location
/// is not specified in that way.
///
/// Note that in most cases, a `SourceLocation` should only contain one type of location information
/// (either line/column numbers or byte offset), i.e., `is_partially_specified()` should be `true`.
/// Many functions that accept `SourceLocation` parameters assert that the passed arguments satisfy
/// this condition. This is to prevent users from specifying both types of location information,
/// which may lead to confusion if the two types of information are inconsistent. When emitting
/// diagnostics, the emitter will fill in the missing information.
class SourceLocation {
public:
    SourceLocation() = default;
    constexpr SourceLocation(unsigned line, unsigned col) : line_(line), col_(col) { }
    constexpr SourceLocation(std::size_t byte_offset) : byte_offset_(byte_offset) { }

    constexpr auto line() const -> unsigned {
        return line_;
    }

    constexpr auto col() const -> unsigned {
        return col_;
    }

    constexpr auto byte_offset() const -> std::size_t {
        return byte_offset_;
    }

    constexpr void set_line(unsigned line) {
        assert(!has_byte_offset() && "Cannot set line when byte offset is already set");
        line_ = line;
    }

    constexpr void set_col(unsigned col) {
        assert(!has_byte_offset() && "Cannot set column when byte offset is already set");
        col_ = col;
    }

    constexpr void set_line_col(unsigned line, unsigned col) {
        set_line(line);
        set_col(col);
    }

    constexpr void set_byte_offset(std::size_t byte_offset) {
        assert(!has_line_col() && "Cannot set byte offset when line/col is already set");
        byte_offset_ = byte_offset;
    }

    /// Returns whether both line and column numbers are specified.
    constexpr auto has_line_col() const -> bool {
        return line_ != static_cast<unsigned>(-1) && col_ != static_cast<unsigned>(-1);
    }

    /// Returns whether the byte offset is specified.
    constexpr auto has_byte_offset() const -> bool {
        return byte_offset_ != static_cast<std::size_t>(-1);
    }

    /// Returns whether both line/column numbers and byte offset are specified.
    constexpr auto is_fully_specified() const -> bool {
        return has_line_col() && has_byte_offset();
    }

    /// Returns whether neither line/column numbers nor byte offset are specified.
    constexpr auto is_unspecified() const -> bool {
        return !has_line_col() && !has_byte_offset();
    }

    /// Returns whether only one of line/column numbers and byte offset is specified.
    constexpr auto is_partially_specified() const -> bool {
        return has_line_col() != has_byte_offset();
    }

    friend constexpr auto operator==(SourceLocation lhs, SourceLocation rhs) -> bool {
        return lhs.line_ == rhs.line_ && lhs.col_ == rhs.col_
            && lhs.byte_offset_ == rhs.byte_offset_;
    }

    friend constexpr auto operator!=(SourceLocation lhs, SourceLocation rhs) -> bool {
        return !(lhs == rhs);
    }

    /// Creates a `SourceLocation` object from the specified `line` and `col`.
    static constexpr auto from_line_col(unsigned line, unsigned col) -> SourceLocation {
        return SourceLocation(line, col);
    }

    /// Creates a `SourceLocation` object from the specified `byte_offset`.
    static constexpr auto from_byte_offset(std::size_t byte_offset) -> SourceLocation {
        return SourceLocation(byte_offset);
    }

private:
    /// The (0-indexed) line number of the location.
    unsigned line_ = static_cast<unsigned>(-1);
    /// The (0-indexed) column number of the location.
    unsigned col_ = static_cast<unsigned>(-1);
    /// The (0-indexed) byte offset of the location.
    std::size_t byte_offset_ = static_cast<std::size_t>(-1);

    /// Make `SourceFile` a friend class so that it can use the `with_byte_offset()` and
    /// `with_line_col()` methods to create new `SourceLocation` objects with filled information.
    friend class SourceFile;

    constexpr SourceLocation(unsigned line, unsigned col, std::size_t byte_offset) :
        line_(line), col_(col), byte_offset_(byte_offset) { }

    /// Returns a new `SourceLocation` object with the same line and column numbers as this one, but
    /// with the specified `byte_offset`.
    constexpr auto with_byte_offset(std::size_t byte_offset) const -> SourceLocation {
        return SourceLocation(line_, col_, byte_offset);
    }

    /// Returns a new `SourceLocation` object with the same byte offset as this one, but with the
    /// specified `line` and `col`.
    constexpr auto with_line_col(unsigned line, unsigned col) const -> SourceLocation {
        return SourceLocation(line, col, byte_offset_);
    }
};

/// Represents a single annotation span in the source code, with an optional label. When rendering
/// diagnostic information, a non-empty label will be rendered together with the corresponding span.
struct LabeledSpan {
    SourceLocation beg;
    SourceLocation end;
    /// The label attached to this span. If label.empty() is true, we consider the annotation to
    /// have no label attached.
    StyledStringView label;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_SOURCE_LOCATION_HPP
