# annotate-snippets

![Build](https://github.com/Pluto-Zy/annotate-snippets-cpp/actions/workflows/gtest.yml/badge.svg)

`annotate-snippests` is a C++ library that can generate diagnostic information output similar to the Rust compiler [`rustc`](https://github.com/rust-lang/rust). `annotate-snippets` can add any number of annotations to one or more source codes. Our algorithm and carefully designed rules can correctly and beautifully handle the overlap and arrangement of annotations.

![demo1](https://github.com/user-attachments/assets/3a83ed83-5dfd-4fca-ae67-073e2a8b1174)

> [!NOTE]
> `annotate-snippets` is still under development and the documentation is not yet complete. We are working hard to improve the stability and usability of the library. If you encounter any problems or have any suggestions, please feel free to open an issue.

## Using `annotate-snippets` in CMake Projects

There are 3 ways to use `annotate-snippets` in CMake projects:

- You can fetch `annotate-snippets` from GitHub using `FetchContent` and add it as a dependency to your CMake project:

  ```cmake
  include(FetchContent)

  FetchContent_Declare(
      annotate-snippets
      GIT_REPOSITORY git@github.com:Pluto-Zy/annotate-snippets-cpp.git
      GIT_TAG main  # Use a fixed version number or a specific branch according to your choice
  )
  FetchContent_MakeAvailable(annotate-snippets)
  ```

- You can also include the source code of `annotate-snippets` as a part of your project (e.g., through `git submodule`) and use `add_subdirectory()` to include it:

  ```cmake
  add_subdirectory(annotate_snippets_folder)
  ```

- You can also use `find_package()` to include `annotate-snippets`:

  ```cmake
  find_package(annotate-snippets REQUIRED)
  ```

  To use this method, you may need to build and install `annotate-snippets` in advance.

Regardless of which method you use, you need to link `annotate-snippets` to your project:

```cmake
target_link_libraries(your_target PRIVATE ants::annotate_snippets)
```

> [!NOTE]
> To use `annotate-snippets`, your compiler should support at least C++17.

## Quick Start

### 1. Define a Set of Diagnostic Levels

`annotate-snippets` does not provide predefined diagnostic levels because different users may need different sets of diagnostic levels. For example, some users may need to include the `Bug` level in the set, while others may need to include the `Info` level in the set.

The simplest way to define your own set of diagnostic levels is to define an `enum`:

```c++
enum class Level {
    Fatal,
    Error,
    Warning,
    Note,
    Help,
    SomethingIWant,
};
```

You can define your own `Level` enumeration arbitrarily. Then you need to define a function `title()` that converts `Level` to a string, and `annotate-snippets` will use this function to convert `Level` to the diagnostic level title printed on the screen:

```c++
auto title(Level level) -> char const* {
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
    case Level::SomethingIWant:
        return "something";
    default:
        return "";
    }
}
```

You need to ensure that this function is visible to `annotate-snippets`, for example, through [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl). In this example, the simplest way is to place this function after the definition of `Level`.

### 2. Add Annotations to Source Code

You can associate source code and file names with the `ants::AnnotatedSource` object:

```c++
#include <annotate_snippets/annotated_source.hpp>

char const* code = R"(auto main() -> int {
    std::cout << "Hello World\n";
})";
// Construct an `AnnotatedSource` object with source code and file name.
ants::AnnotatedSource source(code, "main.cpp");
// Add an annotation to the source code from the 30th to the 34th byte (i.e., `cout`).
source.add_annotation(30, 34);
// Add an annotation to the source code from the 5th to the 9th byte (i.e., `main`) and attach the
// label "name".
source.add_annotation(5, 9, "name");
// Use `SourceLocation` to annotate the 0th to the 4th byte of the 0th line of the source code
// (i.e., `auto`).
source.add_annotation(ants::SourceLocation {0, 0}, ants::SourceLocation {0, 4});
// Annotate all content from the 19th byte of the 0th line to the 1st byte of the 2nd line of the
// source code (i.e., the part surrounded by braces),
source.add_annotation(
    ants::SourceLocation {0, 19},
    ants::SourceLocation {2, 1},
    "function body"
);
// Add a secondary annotation to `World`.
//
// By default, the underline of the primary annotation is "^^^", and the underline of the secondary
// annotation is "---".
source.add_secondary_annotation(45, 50, "secondary label");
```

`AnnotatedSource` provides many interfaces for adding annotations to source code. Regardless of which interface you use, you must specify the range to be annotated. You can specify the offset of the annotated range in the entire source code, as the first two `add_annotation()` calls do. You can also use `ants::SourceLocation` to specify the line number and column number of the range to be annotated. When adding an annotation, you can also provide a label for the annotation, which will be rendered near the annotation.

> [!NOTE]
> When specifying the annotation range for `AnnotatedSource`, you specify the range of bytes rather than actual unicode characters. In addition, all subscripts start from 0. All annotation ranges are left-closed and right-open intervals.

> [!IMPORTANT]
> Please note that `annotate-snippets` does not own either the source code or the label strings you specify for the annotations (such as the strings `"name"`, `"function body"`, etc.), as only references to these strings are stored internally in the library. You must ensure that these strings remain valid until the rendering is complete. For example, if you choose to temporarily store `Diag` objects and delay rendering, you need to ensure that the strings you provide are also stored in data structures such as string pools.

By replacing all `add_*` with `with_*`, you can chain annotations while constructing `AnnotatedSource`:

```c++
auto source = ants::AnnotatedSource(code, "main.cpp")
    .with_annotation(30, 34)
    .with_annotation(5, 9, "name")
    .with_annotation(ants::SourceLocation {0, 0}, ants::SourceLocation {0, 4})
    .with_annotation(
        ants::SourceLocation {0, 19},
        ants::SourceLocation {2, 1},
        "function body"
    )
    .with_secondary_annotation(45, 50, "secondary label");
```

### 3. Add Fix Suggestions

Many compilers provide fix suggestions for erroneous user code. In `annotate-snippets`, you can supply a series of fix patches to an `AnnotatedSource`:

```c++
// Construct an `AnnotatedSource` object with source code and file name. `code` is the string
// defined above.
ants::AnnotatedSource fixed_source(code, "main.cpp");
// Replace the characters between byte 0 and byte 4 (i.e., `auto`) with `int`.
fixed_source.add_replacement_patch(0, 4, "int");
// Delete the characters between byte 11 and byte 18 (i.e., ` -> int`).
fixed_source.add_deletion_patch(11, 18);
// Insert two punctuation marks into the string.
fixed_source.add_addition_patch(ants::SourceLocation {1, 23}, ",");
fixed_source.add_addition_patch(ants::SourceLocation {1, 29}, "!");
```

We provide three types of patching interfaces: addition, deletion, and replacement. Just like when adding annotations, all interfaces support specifying the patch range either by byte offsets or via `SourceLocation`. All interfaces also support chaining calls using corresponding `with_*` methods.

### 4. Build a Diagnostic Entry

After adding annotations to the source code, you need to construct an `ants::Diag` object to package the diagnostic information:

```c++
#include <annotate_snippets/diag.hpp>
#include <utility>

// Construct `source` and `fixed_source` as described above...

// Construct a diagnostic entry. This diagnostic has a warning level and a title message.
ants::Diag diag(Level::Warning, "warning message");
// Add annotated source code to `diag`.
diag.add_source(std::move(source));
```

Similarly, you can chain the source code by using `with_source()`:

```c++
auto diag = ants::Diag(Level::Warning, "warning message")
    .with_source(std::move(source));
```

You can add sub-diagnostic entries to include diagnostic messages of different levels in one diagnostic message. You can supplement an `error` or `warning` message with a `note` or `help` message. You can also add the fix suggestions you defined above as a sub-diagnostic entry with `help` level:

```c++
auto diag = ants::Diag(Level::Warning, "warning message")
    .with_source(std::move(source))
    .with_sub_diag_entry(Level::Note, "note something")
    .with_sub_diag_entry(
        ants::DiagEntry(Level::Help, "help something").with_source(std::move(fixed_source))
    );
```

### 5. Render Diagnostic Entries

You can use `ants::HumanRenderer` to render the packaged `diag` to the console:

```c++
#include <annotate_snippets/renderer/human_renderer.hpp>
#include <iostream>

// Render `diag` to `std::cout`.
ants::HumanRenderer().render_diag(std::cout, std::move(diag));
```

You will get the following rendering output:

```
warning: warning message
 --> main.cpp:2:10
  |
1 |   auto main() -> int {
  |   ^^^^ ^^^^ name     ^
  |  ____________________|
2 | |     std::cout << "Hello World\n";
  | |          ^^^^           ----- secondary label
3 | | }
  | |_^ function body
  = note: note something
help: help something
 --> main.cpp
  |
1 - auto main() -> int {
1 + int main() {
2 |     std::cout << "Hello, World!\n";
  |                        +      +
```

You will notice that the rendering result displayed on the console is colorless. To get color-rich console text output, you need to provide a style sheet to `HumenRenderer` to guide it on how to output each part of the text in what style.

The style sheet is a callable object. It takes `ants::Style` and your defined `Level` and returns `ants::StyleSpec` to indicate how to render the text in the `Style` style when the level is `Level`. For example, the following style sheet definition allows us to render diagnostic information in a color style close to `rustc`:

```c++
auto const style_sheet = [](ants::Style const& style, Level level) -> ants::StyleSpec {
    switch (style.as_predefined_style()) {
    case ants::Style::LineNumber:
    case ants::Style::SecondaryUnderline:
    case ants::Style::SecondaryLabel:
        // For line numbers, secondary underlines and labels, display them in bright blue bold text.
        return ants::StyleSpec::BrightBlue + ants::StyleSpec::Bold;
    case ants::Style::PrimaryMessage:
        // For primary messages, display them in bold text.
        return ants::StyleSpec::Default + ants::StyleSpec::Bold;

    case ants::Style::PrimaryTitle:
    case ants::Style::SecondaryTitle:
    case ants::Style::PrimaryUnderline:
    case ants::Style::PrimaryLabel: {
        // For primary underlines and labels, as well as diagnostic level titles, their text style
        // depends on the level of the current diagnostic entry.
        ants::StyleSpec const color = [&] {
            switch (level) {
            case Level::Fatal:
            case Level::Error:
                return ants::StyleSpec::BrightRed;
            case Level::Warning:
                return ants::StyleSpec::BrightYellow;
            case Level::Note:
                return ants::StyleSpec::BrightGreen;
            case Level::Help:
                return ants::StyleSpec::BrightCyan;
            default:
                return ants::StyleSpec::Default;
            }
        }();

        return color + ants::StyleSpec::Bold;
    }

    case ants::Style::Addition:
        // For addition patches, display them in bright green text.
        return ants::StyleSpec::BrightGreen;
    case ants::Style::Deletion:
        // For deletion patches, display them in bright red text.
        return ants::StyleSpec::BrightRed;
    case ants::Style::Replacement:
        // For replacement patches, display them in bright yellow text.
        return ants::StyleSpec::BrightYellow;

    default:
        return {};
    }
};

// Render diagnostic information with the defined style sheet.
ants::HumanRenderer().render_diag(std::cout, std::move(diag), style_sheet);
```

We will get the following output:

![demo2](https://github.com/user-attachments/assets/c6bfcfd3-9d35-48d2-b602-c83bb046a1f5)

> [!TIP]
> Defining such a style sheet seems to be challenging for novice users. Perhaps we can provide a predefined style sheet. For now, you can directly copy the style sheet above for use.

## Build and Install from Source Code

To install `annotate-snippets` in your system or run the unit tests of `annotate-snippets`, you need to build and install `annotate-snippets` from the source code:

```shell
# Clone the source code from GitHub
git clone https://github.com/Pluto-Zy/annotate-snippets-cpp.git
cd annotate-snippets-cpp
# CMake configuration
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cd build
# Build
cmake --build .
```

By default, unit tests will be built together. If you do not want to build unit tests, you can disable unit tests by `-DANNOTATE_SNIPPETS_ENABLE_TESTING=OFF`.

After the build is complete, you can run the unit tests and install:

```shell
# Run unit tests
ctest --output-on-failure
# Install `annotate-snippets`
cmake --install .
# Or specify the installation directory
cmake --install . --prefix path/to/install
```
