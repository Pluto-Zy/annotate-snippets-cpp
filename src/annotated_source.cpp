#include "annotate_snippets/annotated_source.hpp"

#include "annotate_snippets/patch.hpp"
#include "annotate_snippets/source_location.hpp"

#include <functional>

namespace ants {
void AnnotatedSource::resolve_and_normalize_locations() {
    // For `primary_spans_` and `secondary_spans_`, resolve and adjust each span.
    for (auto const& span : { std::ref(primary_spans_), std::ref(secondary_spans_) }) {
        for (LabeledSpan& s : span.get()) {
            s.beg = fill_source_location(s.beg);
            s.end = fill_source_location(s.end);
            s = adjust_span(s);
        }
    }

    // For `patches_`, resolve each patch and normalize its locations.
    for (Patch& patch : patches_) {
        patch.set_location_begin(normalize_location(fill_source_location(patch.location_begin())));
        patch.set_location_end(normalize_location(fill_source_location(patch.location_end())));
    }
}
}  // namespace ants
