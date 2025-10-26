#include "analyze.hpp"

#include <filesystem>
#include <stack>

#include "exception.hpp"
#include "profile.hpp"
#include "trace.hpp"


// --- Event manipulation ---
// --------------------------

template <class Predicate>
std::vector<cbp::trace::event> extract_events(std::vector<cbp::trace::event>& events, Predicate&& predicate) {
    // Partition the vector so all events that satisfy the predicate are at the end
    const auto inverse_predicate = [&](const cbp::trace::event& event) { return !predicate(event); };
    const auto partition_begin   = std::stable_partition(events.begin(), events.end(), inverse_predicate);

    // Bulk-move partitioned events to a new vector
    std::vector<cbp::trace::event> result;

    result.insert(result.end(), std::make_move_iterator(partition_begin), std::make_move_iterator(events.end()));
    events.erase(partition_begin, events.end());

    return result;
}

std::vector<cbp::trace::event> extract_events_by_name(std::vector<cbp::trace::event>& events, std::string_view name) {
    return extract_events(events, [&](const cbp::trace::event& event) { return event.name == name; });
}

struct timeframe {
    cbp::microseconds time;
    cbp::microseconds duration;
};

timeframe get_timeframe(const std::vector<cbp::trace::event>& events) {
    cbp::microseconds min{};
    cbp::microseconds max{};

    for (const auto& e : events) {
        min = std::min(min, e.time);
        max = std::max(max, e.time);
    }

    return timeframe{.time = min, .duration = max - min};
}

template <cbp::tree::node node_type>
cbp::microseconds get_total_duration(const std::vector<node_type>& nodes) {
    cbp::microseconds total{};
    for (const auto& node : nodes) total += node.timing.duration_total;
    return total;
}


// --- Utils ---
// -------------

void sort_events_by_time(std::vector<cbp::trace::event>& events) {
    const auto order = [](const cbp::trace::event& a, const cbp::trace::event& b) { return a.time < b.time; };

    std::stable_sort(events.begin(), events.end(), order);
}

template <cbp::tree::node node_type>
void sort_nodes_by_duration(std::vector<node_type>& includes) {
    const auto order = [](const node_type& a, const node_type& b) {
        return a.timing.duration_total > b.timing.duration_total; // descending order - longer includes are placed first
    };

    std::stable_sort(includes.begin(), includes.end(), order);
}

[[nodiscard]] std::string normalize_path(std::filesystem::path path) {
    return path.lexically_normal().string();
    // Note: Normalizing paths leads to a nicer output, otherwise we can end
    //       up with names like 'lib/bin/../include/' instead of 'lib/include/'
}


// --- Parsing data ---
// --------------------

cbp::tree::parsing_node build_include_tree(std::vector<cbp::trace::event>& source_events) {
    sort_events_by_time(source_events);
    // ordering events by time allows us to deduce transitive includes based on event nesting

    // Build include trees & gather their timings, results should be sorted by duration
    std::vector<cbp::tree::source_include_node> top_level_includes;
    std::stack<cbp::tree::source_include_node*> include_stack;

    for (const auto& event : source_events) {
        // Include began
        if (event.type == "b") {
            std::string name = normalize_path(event.args.at("detail").as<std::string>());

            const bool include_is_transitive = !include_stack.empty();

            auto& includes = include_is_transitive ? include_stack.top()->transitive_includes : top_level_includes;

            includes.push_back(cbp::tree::source_include_node{
                .name   = std::move(name),                            //
                .timing = cbp::tree::timing_data{.time = event.time}, //
            });

            include_stack.push(&includes.back());
        }
        // Include ended
        else if (event.type == "e") {
            assert(include_stack.size());

            auto& include = include_stack.top(); // guaranteed to exist due to the event order

            include->timing.duration_total = event.time - include->timing.time;

            include->timing.duration_self = include->timing.duration_total;
            for (const auto& e : include->transitive_includes) include->timing.duration_self -= e.timing.duration_total;
            // should never underflow assuming monotonic time measurement

            sort_nodes_by_duration(include->transitive_includes);

            include_stack.pop();
        } else {
            throw cbp::exception{"Encountered unknown source event type {{ {} }} while parsing includes.", event.type};
        }
    }

    sort_nodes_by_duration(top_level_includes);

    // Gather total timings & build the resulting parsing node
    cbp::tree::parsing_node result;

    result.includes              = std::move(top_level_includes);
    result.timing.time           = source_events.empty() ? cbp::microseconds{} : source_events.front().time;
    result.timing.duration_total = get_total_duration(result.includes);
    result.timing.duration_self  = cbp::microseconds{};

    // TODO: Figure out a way to measure .cpp parsing time, there doesn't seem to be an event for this

    return result;
}

// --- Template data ---
// ---------------------

// To create "Templates" subtree we need to clone the include structure from the "Parsing" subtree
// and then accumulate the instantiation times to the appropriate nodes (where each node corresponds
// to a single include). This is quite tricky, but can be managed by descending recursively down the tree.

template <cbp::tree::node node_type>
using source_mapping = std::unordered_map<std::string, node_type*>;

cbp::tree::template_include_node clone_node(const cbp::tree::source_include_node& include_node) {
    cbp::tree::template_include_node template_node;

    // Copy the node itself
    template_node.name = include_node.name;

    // Recursively copy the child nodes
    cbp::tree::apply_down(include_node,
                          [&](const auto& child) { template_node.transitive_includes.push_back(clone_node(child)); });

    return template_node;
}

source_mapping<cbp::tree::template_include_node>
build_source_mapping(std::vector<cbp::tree::template_include_node>& includes) {
    source_mapping<cbp::tree::template_include_node> mapping;

    for (auto& include : includes)
        cbp::tree::apply_recursively(include, [&](auto& child) { mapping[child.name] = std::addressof(child); });

    return mapping;
}

cbp::tree::templates_node build_templates_tree(std::vector<cbp::trace::event>& template_events,
                                               const cbp::tree::parsing_node&  parsing) {
    cbp::tree::templates_node result;
    
    // Clone the include tree & build the name mapping
    for (const auto& include : parsing.includes) result.includes.push_back(clone_node(include));

    auto source_mapping = build_source_mapping(result.includes);
    
    // Gather template instantiation times into self-duration
    for (auto& event : template_events) {
        const std::string name = normalize_path(event.args.at("file").as<std::string>());

        // Template instantiated in one of the includes
        if (const auto it = source_mapping.find(name); it != source_mapping.end()) {
            auto& include_node = *it->second;

            include_node.timing.duration_self += event.duration.value(); // value should always be present due to schema
        }
        // Template instantiated in the '.cpp'
        else {
            result.timing.duration_self += event.duration.value();
        }
    }
    
    // Collect the total duration from child nodes
    // TODO:
    
    return result;
}

// --- Analyze translation unit ---
// --------------------------------

cbp::tree::translation_unit_node cbp::analyze_translation_unit(std::string_view path) {
    // Parse the trace
    auto trace  = cbp::read_file_json<cbp::trace::trace>(path);
    auto result = cbp::tree::translation_unit_node{.name = std::string(path)};

    // Get total timeframe
    const auto timeframe = get_timeframe(trace.events);

    result.timing.time           = timeframe.time;
    result.timing.duration_total = timeframe.duration;
    result.timing.duration_self  = microseconds{};
    // Note: Self-duration for a translation unit doesn't make much sense in our tree so we set it to 0

    // Gather source parsing times
    std::vector<cbp::trace::event> source_events = extract_events_by_name(trace.events, "Source");

    result.parsing = build_include_tree(source_events);

    // Gather template instantiation times
    std::vector<cbp::trace::event> instantiation_events =
        extract_events(trace.events, [](const cbp::trace::event& event) {
            return event.name == "InstantiateClass" || event.name == "InstantiateClass";
        });

    result.templates = build_templates_tree(instantiation_events, result.parsing);

    return result;
}

// --- Analyze build directory ---
// -------------------------------

// --- Analyze CMake build ---
// ---------------------------
