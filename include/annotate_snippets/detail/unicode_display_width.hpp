#ifndef ANNOTATE_SNIPPETS_DETAIL_UNICODE_DISPLAY_WIDTH_HPP
#define ANNOTATE_SNIPPETS_DETAIL_UNICODE_DISPLAY_WIDTH_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

//! The following code is adapted from the open-source [fmt](https://github.com/fmtlib/fmt )
//! library, specifically from the file:
//! https://github.com/fmtlib/fmt/blob/0fae326c42e121bf888f12cfa4136ab02ebed3d0/include/fmt/format.h
//! This segment of code is used for calculating the display width of a UTF-8 encoded string on the
//! console. It is part of the fmtlib project under the MIT License. Refer to their repository for
//! more details.

namespace ants::detail {
// A public domain branchless UTF-8 decoder by Christopher Wellons:
// https://github.com/skeeto/branchless-utf8
/// Decode the next character, c, from s, reporting errors in e.
///
/// Since this is a branchless decoder, four bytes will be read from the buffer regardless of the
/// actual length of the next character. This means the buffer _must_ have at least three bytes of
/// zero padding following the end of the data stream.
///
/// Errors are reported in e, which will be non-zero if the parsed character was somehow invalid:
/// invalid byte sequence, non-canonical encoding, or a surrogate half.
///
/// The function returns a pointer to the next character. When an error occurs, this pointer will be
/// a guess that depends on the particular error, but it will always advance at least one byte.
constexpr auto utf8_decode(const char* s, uint32_t* c, int* e) -> const char* {
    constexpr const int masks[] = { 0x00, 0x7f, 0x1f, 0x0f, 0x07 };
    constexpr const uint32_t mins[] = { 4194304, 0, 128, 2048, 65536 };
    constexpr const int shiftc[] = { 0, 18, 12, 6, 0 };
    constexpr const int shifte[] = { 0, 6, 4, 2, 0 };

    // NOLINTNEXTLINE(bugprone-signed-char-misuse)
    int const len = "\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0\0\0\2\2\2\2\3\3\4"
        [static_cast<unsigned char>(*s) >> 3];
    // Compute the pointer to the next character early so that the next iteration can start working
    // on the next character. Neither Clang nor GCC figure out this reordering on their own.
    const char* next = s + len + !len;

    using uchar = unsigned char;  // NOLINT(readability-identifier-naming)

    // Assume a four-byte character and load four bytes. Unused bits are shifted out.
    *c = static_cast<uint32_t>(static_cast<uchar>(s[0]) & masks[len]) << 18;
    *c |= static_cast<uint32_t>(static_cast<uchar>(s[1]) & 0x3f) << 12;
    *c |= static_cast<uint32_t>(static_cast<uchar>(s[2]) & 0x3f) << 6;
    *c |= static_cast<uint32_t>(static_cast<uchar>(s[3]) & 0x3f) << 0;
    *c >>= shiftc[len];

    // Accumulate the various error conditions.
    *e = (*c < mins[len]) << 6;  // non-canonical encoding
    *e |= ((*c >> 11) == 0x1b) << 7;  // surrogate half?
    *e |= (*c > 0x10FFFF) << 8;  // out of range?
    *e |= (static_cast<uchar>(s[1]) & 0xc0) >> 2;
    *e |= (static_cast<uchar>(s[2]) & 0xc0) >> 4;
    *e |= static_cast<uchar>(s[3]) >> 6;
    *e ^= 0x2a;  // top two bits of each tail byte correct?
    *e >>= shifte[len];

    return next;
}

constexpr uint32_t invalid_code_point = ~uint32_t();

/// Invokes f(cp, sv) for every code point cp in s with sv being the string view corresponding to
/// the code point. cp is invalid_code_point on error.
template <typename F>
inline void for_each_codepoint(std::string_view s, F f) {
    auto decode = [f](const char* buf_ptr, const char* ptr) {
        auto cp = uint32_t();
        auto error = 0;
        char const* end = utf8_decode(buf_ptr, &cp, &error);
        bool const result =
            f(error ? invalid_code_point : cp,
              std::string_view(ptr, error ? 1 : static_cast<std::size_t>(end - buf_ptr)));
        // NOLINTNEXTLINE(readability-avoid-nested-conditional-operator)
        return result ? (error ? buf_ptr + 1 : end) : nullptr;
    };
    char const* p = s.data();
    const size_t block_size = 4;  // utf8_decode always reads blocks of 4 chars.
    if (s.size() >= block_size) {
        for (char const* end = p + s.size() - block_size + 1; p < end;) {
            p = decode(p, p);
            if (!p) {
                return;
            }
        }
    }
    if (auto num_chars_left = s.data() + s.size() - p) {
        char buf[2 * block_size - 1] = {};
        std::ranges::copy(p, p + num_chars_left, buf);
        const char* buf_ptr = buf;
        do {
            auto end = decode(buf_ptr, p);
            if (!end) {
                return;
            }
            p += end - buf_ptr;
            buf_ptr = end;
        } while (buf_ptr - buf < num_chars_left);
    }
}

/// Computes approximate display width of a UTF-8 string.
inline auto display_width(std::string_view s) -> size_t {
    size_t num_code_points = 0;
    // It is not a lambda for compatibility with C++14.
    // NOLINTNEXTLINE(readability-identifier-naming)
    struct count_code_points {
        size_t* count;
        auto operator()(uint32_t cp, std::string_view /*unused*/) const -> bool {
            *count += static_cast<unsigned>(
                1
                + (cp >= 0x1100
                   && (cp <= 0x115f ||  // Hangul Jamo init. consonants
                       cp == 0x2329 ||  // LEFT-POINTING ANGLE BRACKET
                       cp == 0x232a ||  // RIGHT-POINTING ANGLE BRACKET
                       // CJK ... Yi except IDEOGRAPHIC HALF FILL SPACE:
                       (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f)
                       || (cp >= 0xac00 && cp <= 0xd7a3) ||  // Hangul Syllables
                       (cp >= 0xf900 && cp <= 0xfaff) ||  // CJK Compatibility Ideographs
                       (cp >= 0xfe10 && cp <= 0xfe19) ||  // Vertical Forms
                       (cp >= 0xfe30 && cp <= 0xfe6f) ||  // CJK Compatibility Forms
                       (cp >= 0xff00 && cp <= 0xff60) ||  // Fullwidth Forms
                       (cp >= 0xffe0 && cp <= 0xffe6) ||  // Fullwidth Forms
                       (cp >= 0x20000 && cp <= 0x2fffd) ||  // CJK
                       (cp >= 0x30000 && cp <= 0x3fffd) ||
                       // Miscellaneous Symbols and Pictographs + Emoticons:
                       (cp >= 0x1f300 && cp <= 0x1f64f) ||
                       // Supplemental Symbols and Pictographs:
                       (cp >= 0x1f900 && cp <= 0x1f9ff)))
            );
            return true;
        }
    };
    // We could avoid branches by using utf8_decode directly.
    for_each_codepoint(s, count_code_points { &num_code_points });
    return num_code_points;
}
}  // namespace ants::detail

#endif  // ANNOTATE_SNIPPETS_DETAIL_UNICODE_DISPLAY_WIDTH_HPP
