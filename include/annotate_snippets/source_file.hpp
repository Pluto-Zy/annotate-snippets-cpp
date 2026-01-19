#ifndef ANNOTATE_SNIPPETS_SOURCE_FILE_HPP
#define ANNOTATE_SNIPPETS_SOURCE_FILE_HPP

#include "annotate_snippets/source_location.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace ants {
/// Internal storage structure for `SourceFile`. Each `SourceFile` object contains a shared pointer
/// to a `SourceFileStorage` object that holds the actual data. Multiple `SourceFile` objects can
/// share the same `SourceFileStorage` object, so updating the `line_offsets` cache of one object
/// will also be visible to other `SourceFile`s that share the same storage object.
struct SourceFileStorage {
    /// The name of the source file.
    std::string_view name;
    /// The content of the source file.
    std::string_view content;
    /// The byte offsets of the beginning of each line in the source file. It is used to quickly
    /// locate the content of a specific line.
    ///
    /// The cache is initially empty, and will be populated on demand. It can also be set by the
    /// user if the information is already known.
    std::vector<std::size_t> line_offsets;
    /// The (1-indexed) line number of the first line in the source code. The line numbers of
    /// subsequent lines will be calculated based on this, which allows us to provide a portion of
    /// the source code and explicitly specify the actual line number of the first line to display
    /// the correct line number in the rendered result.
    unsigned first_line_number;

    explicit SourceFileStorage(std::string_view content, std::string_view name) :
        name(name), content(content), first_line_number(1) { }
};

/// Represents a source file, including its name and content.
///
/// `SourceFile` is used to represent a source code file. In addition to managing the file's name
/// and content, it also maintains a cache that stores the byte offsets of the beginning of each
/// line for quick access to specific lines. Users can build this cache on demand or build the
/// complete cache directly. If the user already has this information, they can also set the cache
/// directly. Furthermore, non-const methods of `SourceFile` will automatically build the cache as
/// needed to ensure that it contains the required information. When using const methods, it is the
/// user's responsibility to ensure that the cache is complete, as these methods typically assume
/// that the cache has been fully built. If the cache is incomplete, the results may be incorrect.
///
/// `SourceFile` is implemented internally as a shared pointer to a `SourceFileStorage` object.
/// Multiple `SourceFile` objects can share the same storage object, so updating the cache of one
/// object will affect all objects that share the same storage. This allows `SourceFile` objects to
/// be efficiently copied and passed around. `AnnotatedSource` references the source code content
/// and shares the cache information by holding a `SourceFile` object. This allows users to avoid
/// fixing the address of the `SourceFile` object itself.
///
/// A moved-from `SourceFile` object is no longer valid, and calls to most of its member functions
/// result in undefined behavior. In debug mode, this situation is detected through assertions.
///
/// `SourceFile` provides numerous helper functions for manipulating source code locations. These
/// are typically used by diagnostic emitters to correctly locate source code positions when
/// rendering diagnostic information. The emitter ensures that the cache is fully built before
/// calling these functions.
class SourceFile {
public:
    /// Creates an empty `SourceFile` object that is not associated with any file.
    SourceFile() = default;

    /// Creates a `SourceFile` object containing the content `content`.
    explicit SourceFile(std::string_view content) : SourceFile(content, /*name=*/ {}) { }

    /// Creates a `SourceFile` object associated with the file named `name` and containing the
    /// content `content`.
    SourceFile(std::string_view content, std::string_view name) :
        storage_(std::make_shared<SourceFileStorage>(remove_final_newline(content), name)) { }

    /// Returns whether this `SourceFile` object is valid (i.e., associated with a file).
    auto valid() const -> bool {
        return static_cast<bool>(storage_);
    }

    /// Returns whether this `SourceFile` object is invalid (i.e., not associated with any file).
    auto invalid() const -> bool {
        return !valid();
    }

    auto name() const -> std::string_view {
        return storage().name;
    }

    void set_name(std::string_view name) {
        storage().name = name;
    }

    auto with_name(std::string_view name) & -> SourceFile& {
        set_name(name);
        return *this;
    }

    auto with_name(std::string_view name) && -> SourceFile&& {
        set_name(name);
        return std::move(*this);
    }

    auto content() const -> std::string_view {
        return storage().content;
    }

    auto size() const -> std::size_t {
        return content().size();
    }

    auto first_line_number() const -> unsigned {
        return storage().first_line_number;
    }

    void set_first_line_number(unsigned number) {
        storage().first_line_number = number;
    }

    auto with_first_line_number(unsigned number) & -> SourceFile& {
        set_first_line_number(number);
        return *this;
    }

    auto with_first_line_number(unsigned number) && -> SourceFile&& {
        set_first_line_number(number);
        return std::move(*this);
    }

    /// Sets the internal line offsets cache to `line_offsets`.
    void set_lines(std::vector<std::size_t> line_offsets) {
        storage().line_offsets = std::move(line_offsets);
    }

    /// Returns the number of lines currently cached.
    auto cached_line_count() const -> unsigned {
        return static_cast<unsigned>(storage().line_offsets.size());
    }

    /// Returns the internal line offsets cache.
    auto cached_lines() const -> std::vector<std::size_t> const& {
        return storage().line_offsets;
    }

    /// Returns whether the internal line offsets cache is fully built.
    auto is_fully_cached() const -> bool {
        return !cached_lines().empty() && cached_lines().back() >= content_end();
    }

    /// Ensures that the internal line offsets cache is built up to the offset `byte_offset`. If the
    /// cache already covers `byte_offset`, this function does nothing.
    void build_lines_to_offset(std::size_t byte_offset);

    /// Ensures that the internal line offsets cache is built up to the line `line`. If the cache
    /// already covers `line`, this function does nothing.
    void build_lines_to_line(unsigned line);

    /// Ensures that the internal line offsets cache is built up to the location specified by `loc`.
    /// It will use the appropriate field of `loc` (either line/col or byte offset) to determine how
    /// far to build the cache.
    ///
    /// Note that `loc` must be partially specified (i.e., it must have either line/col or byte
    /// offset specified).
    void build_lines_to_loc(SourceLocation loc);

    /// Builds the internal line offsets cache based on the current content of the source file. It
    /// will scan through the entire content to find the positions of newline characters and record
    /// the byte offsets of the beginning of each line.
    void build_lines() {
        // Build the cache up to the end of the content, which is the start of the last line.
        build_lines_to_offset(content_end());
    }

    /// Returns the total number of lines in the source file. This function ensures that the line
    /// offsets cache is fully built before returning the result.
    auto total_line_count() -> unsigned {
        build_lines();
        return cached_line_count();
    }

    /// Returns the total number of lines in the source file. Note that this function assumes that
    /// the line offsets cache is already fully built. If the cache is not fully built, the result
    /// may be incorrect.
    auto total_line_count() const -> unsigned {
        assert(is_fully_cached() && "line offsets cache is not fully built");
        return cached_line_count();
    }

    /// Returns the offset of the first byte of line `line`. If `line` is greater than the number of
    /// lines in the cache, the offset of the last line is returned.
    ///
    /// This function assumes that the line offsets cache is already built up to `line`. If the
    /// cache does not cover `line`, the result may be incorrect.
    auto line_offset(unsigned line) const -> std::size_t {
        return cached_lines()[(std::min)(line, cached_line_count() - 1)];
    }

    /// The non-const version of `line_offset()`. It ensures that the line offsets cache is built up
    /// to `line` before returning the result.
    auto line_offset(unsigned line) -> std::size_t {
        build_lines_to_line(line);
        return const_cast<const SourceFile*>(this)->line_offset(line);
    }

    /// Returns the content of the line `line`. If the line does not exist, returns an empty string.
    /// The returned string does not include the trailing newline character.
    ///
    /// This function assumes that the line offsets cache is already built up to `line`. If the
    /// cache does not cover `line`, the result may be incorrect.
    auto line_content(unsigned line) const -> std::string_view;

    /// The non-const version of `line_content()`. It ensures that the line offsets cache is built
    /// up to `line` before returning the result.
    auto line_content(unsigned line) -> std::string_view {
        // Note that we need to build the cache up to `line + 1` to ensure that we can get the end
        // offset of line `line`.
        build_lines_to_line(line + 1);
        return const_cast<const SourceFile*>(this)->line_content(line);
    }

    /// Returns the line and column number of the byte at offset `byte_offset` in the source file.
    /// The line and column numbers are both 0-indexed. If `byte_offset` exceeds the length of the
    /// source file, the last line is returned.
    ///
    /// This function does not modify the line offsets cache. If the cache does not cover
    /// `byte_offset`, the result may be incorrect.
    auto byte_offset_to_line_col(std::size_t byte_offset) const -> std::pair<unsigned, unsigned>;

    /// The non-const version of `byte_offset_to_line_col()`. It ensures that the line offsets cache
    /// is built up to `byte_offset` before returning the result.
    ///
    /// This function ensures that the line offsets cache is built up to `byte_offset` before
    /// returning the result.
    auto byte_offset_to_line_col(std::size_t byte_offset) -> std::pair<unsigned, unsigned> {
        build_lines_to_offset(byte_offset);
        return const_cast<const SourceFile*>(this)->byte_offset_to_line_col(byte_offset);
    }

    /// Returns the byte offset corresponding to the given `line` and `col`. Both `line` and `col`
    /// are 0-indexed.
    ///
    /// If `line` is greater than the actual number of lines in the source file, it is adjusted to
    /// the last line (which may be the added hypothetical line). The function does not check
    /// whether `col` exceeds the number of valid characters in that line. If it does, it falls back
    /// to the subsequent lines.
    ///
    /// Note that converting (line, col) to byte offset and then back to (line, col) may not yield
    /// the same result, especially when col exceeds the number of valid characters in that line.
    ///
    /// This function assumes that the line offsets cache is already built up to `line`. If the
    /// cache does not cover `line`, the result may be incorrect.
    auto line_col_to_byte_offset(unsigned line, unsigned col) const -> std::size_t;

    /// The non-const version of `line_col_to_byte_offset()`. It ensures that the line offsets cache
    /// is built up to `line` before returning the result.
    auto line_col_to_byte_offset(unsigned line, unsigned col) -> std::size_t {
        build_lines_to_line(line);
        return const_cast<const SourceFile*>(this)->line_col_to_byte_offset(line, col);
    }

    /// Fills in the missing information in `loc` based on the content of the source file. Note that
    /// this function does not modify `loc`, but returns a new `SourceLocation` object with the
    /// filled information.
    ///
    /// `loc` must be partially specified (i.e., it must have either line/col or byte offset
    /// specified).
    [[nodiscard]] auto fill_source_location(SourceLocation loc) const -> SourceLocation;

    /// The non-const version of `fill_source_location()`. It ensures that the line offsets cache is
    /// built up to the necessary extent before filling in the missing information.
    [[nodiscard]] auto fill_source_location(SourceLocation loc) -> SourceLocation {
        build_lines_to_loc(loc);
        return const_cast<const SourceFile*>(this)->fill_source_location(loc);
    }

    /// Adjusts `loc` to ensure it is within the valid range of the source code.
    ///
    /// This function checks if `loc` is within the range of the source code. It handles the
    /// following two scenarios:
    /// - If the column of `loc` exceeds the number of valid characters in that line, it adjusts
    ///   `loc` to point to the start of the next line.
    /// - If the line of `loc` exceeds the actual number of lines in the source code, it adjusts
    ///   `loc` to point to the end of the source code.
    ///
    /// This function requires `loc` to have line and column information to perform the first check.
    /// The returned `SourceLocation` object will always have complete location information.
    ///
    /// This function assumes that the line offsets cache is already built up to the necessary
    /// extent.
    [[nodiscard]] auto normalize_location(SourceLocation loc) const -> SourceLocation;

    /// A non-const version of `normalize_location()`. It ensures that the line offsets cache is
    /// built up to the necessary extent before normalizing the location.
    [[nodiscard]] auto normalize_location(SourceLocation loc) -> SourceLocation;

    /// Adjusts the range of the span. Returns the adjusted span without modifying `span`.
    ///
    /// This function adjusts the span in two ways:
    /// 1. If the current span points to an empty range (`beg == end`), it adjusts it to include 1
    ///    byte.
    /// 2. If the current span's `end` points to the start of a line, it adjusts it to point to the
    ///    end of the previous line. Since the character pointed to by `end` is not included in the
    ///    span, this ensures that we render the correct result (e.g., it does not treat a
    ///    single-line span as a multi-line span). Consider the following example:
    ///
    ///        "hello"
    ///        ^^^^^^^ We want to annotate this word.
    ///        "world"
    ///        ^ However, `end` points to the start of the next line. We cannot render this line.
    ///
    /// This function requires the `SourceLocation`s in `span` to have line and column information
    /// to perform the necessary checks.
    [[nodiscard]] auto adjust_span(LabeledSpan span) const -> LabeledSpan;

    /// A non-const version of `adjust_span()`. It ensures that the line offsets cache is built up
    /// to the necessary extent before adjusting the span.
    [[nodiscard]] auto adjust_span(LabeledSpan span) -> LabeledSpan;

private:
    /// Pointer to the internal storage structure.
    std::shared_ptr<SourceFileStorage> storage_;

    /// Returns a reference to the internal storage structure. Asserts that the storage is valid.
    /// This overload ensures that the returned reference is const if the `SourceFile` object is
    /// const.
    auto storage() const -> SourceFileStorage const& {
        assert(storage_ && "`SourceFile` is not associated with any file");
        return *storage_;
    }

    /// Returns a reference to the internal storage structure. Asserts that the storage is valid.
    auto storage() -> SourceFileStorage& {
        assert(storage_ && "`SourceFile` is not associated with any file");
        return *storage_;
    }

    /// Returns the offset of the last byte in the source content, which is the start of the
    /// hypothetical line after the last line. See `remove_final_newline()` for details.
    auto content_end() const -> std::size_t {
        return size() + 1;
    }

    /// Removes the trailing newline character from `str` if it exists.
    ///
    /// Our implementation requires that the source always ends with a newline character. If it does
    /// not, we append a newline character at the end. However, we cannot append characters to
    /// `string_view`, so we choose to remove the newline character at the end of `str` to ensure
    /// that we can handle the source code in a consistent manner.
    static auto remove_final_newline(std::string_view str) -> std::string_view;
};
}  // namespace ants

#endif  // ANNOTATE_SNIPPETS_SOURCE_FILE_HPP
