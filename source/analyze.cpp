#include "analyze.hpp"

#include <filesystem>
#include <stack>

#include "exception.hpp"
#include "profile.hpp"
#include "trace.hpp"


// --- Implementation utils ---
// ----------------------------

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

std::vector<cbp::trace::event> extract_parsing_events(std::vector<cbp::trace::event>& events) {
    return extract_events(events, [&](const cbp::trace::event& event) { return event.name == "Source"; });
}

std::vector<cbp::trace::event> extract_instantiation_events(std::vector<cbp::trace::event>& events) {
    return extract_events(events, [&](const cbp::trace::event& event) {
        return event.name == "InstantiateClass" || event.name == "InstantiateClass";
    });
}

[[nodiscard]] std::string normalize_path(std::filesystem::path path) {
    return path.lexically_normal().string();
    // Note: Normalizing paths leads to a nicer output, otherwise we can end
    //       up with names like 'lib/bin/../include/' instead of 'lib/include/'
}


// --- Subtrees ---
// ----------------

// Assuming a correct trace schema, every '#include' has a pair of correspondig begin/end events
// (event types "b" and "e"). When these events are ordered chronologically, we can deduce
// transitive includes based on the "b" / "e" event nesting. Below is a simple example:
//
// Events:
//    > root-b      | depth = 0->1 // every event corresponds to a loop iteration
//    >    child1-b | depth = 1->2
//    >    child1-e | depth = 2->1
//    >    child2-b | depth = 1->2
//    >    child2-e | depth = 2->1
//    > root-e      | depth = 1->0 // to find & finish the 'root' node here we need to search
//                                 // upwards for the last node with 'depth == 1'
//
// Nodes:
//    > root      | node.depth = 0
//    >    child1 | node.depth = 1
//    >    child2 | node.depth = 1

std::vector<cbp::tree::node> build_parsing_subtree(const std::vector<cbp::trace::event>& parsing_events) {

    constexpr std::size_t root_depth = 1;

    auto parsing_node = cbp::tree::node{
        .name           = "Parsing",                                                //
        .time           = parsing_events.front().time,                              //
        .duration_total = parsing_events.back().time - parsing_events.front().time, //
        .duration_self  = cbp::milliseconds{},                                      // how do we compute this?
        .depth          = root_depth,                                               //
        .type           = cbp::tree::node_type::parsing                             //
    };

    auto        result = std::vector{std::move(parsing_node)};
    std::size_t depth  = root_depth;

    for (const auto& event : parsing_events) {
        // Include began
        if (event.type == "b") {
            ++depth;

            result.push_back({
                .name  = event.args.at("detail").get_string(), //
                .time  = event.time,                           //
                .depth = depth,                                //
                .type  = cbp::tree::node_type::source_parsing  //
            });
        }
        // Include ended
        else if (event.type == "e") {
            // Iterate upwards until we find a node with the same depth, this achieves 2 things simultaneously:
            //    1) Finds the node corresponding to this end event
            //    2) Computes a total duration of all its child nodes
            cbp::microseconds child_nodes_duration_total{};

            for (auto it = result.rbegin(); it != result.rend(); ++it) {
                if (it->depth == depth + 1) child_nodes_duration_total += it->duration_total;

                if (it->depth == depth) { // corresponding node is found
                    it->duration_total = event.time - it->time;
                    it->duration_self  = it->duration_total - child_nodes_duration_total;
                    break;
                }
            }

            --depth;
        }
        // Wrong event type, should never trigger assuming a correct schema
        else {
            throw cbp::exception{"Encountered an unknown parsing event type {{ {} }}.", event.type};
        }
    }

    return result;
}

// Nodes:
//    > root      | node.depth = 0
//    >    child1 | node.depth = 1
//    >    child2 | node.depth = 1

std::vector<cbp::tree::node>
build_instantiation_subtree([[maybe_unused]] const std::vector<cbp::trace::event>& instantiation_events,
                            [[maybe_unused]] const std::vector<cbp::tree::node>&   parsing_subtree) {
    // Create root node
    constexpr std::size_t root_depth = 1;

    std::vector<cbp::tree::node> result = parsing_subtree;

    result.front() = cbp::tree::node{
        .name           = "Template instantiation",           //
        .time           = instantiation_events.front().time,  //
        .duration_total = cbp::milliseconds{},                //
        .duration_self  = cbp::milliseconds{},                //
        .depth          = root_depth,                         //
        .type           = cbp::tree::node_type::instantiation //
    };

    // Create filename mapping & clear the timings
    std::unordered_map<std::string, std::size_t> source_mapping;

    for (std::size_t i = 0; i < result.size(); ++i) {
        auto& node = result[i];

        if (node.type != cbp::tree::node_type::source_parsing) continue;

        node.time           = cbp::milliseconds{};
        node.duration_total = cbp::milliseconds{};
        node.duration_self  = cbp::milliseconds{};
        node.type           = cbp::tree::node_type::source_instantiation;

        source_mapping[node.name] = i;
    }

    // Accumulate timings into the tree using the mapping
    for (const auto& event : instantiation_events) {
        const std::string name = event.args.at("file").get_string();

        if (!event.duration) throw cbp::exception{"Template instantiation event is missing a duration field."};
        // should never trigger assuming a correct schema

        // Instantiation happened in one of the includes
        if (auto it = source_mapping.find(name); it != source_mapping.end()) {
            const std::size_t i = it->second;

            result[i].duration_self += event.duration.value();
            result[i].duration_total += event.duration.value();
        }
        // Instantiation happened in the '.cpp'
        else {
            result.front().duration_self += event.duration.value();
            result.front().duration_total += event.duration.value();
        }
    }

    // Propagate self-duration upwards to get total duration for each node
    std::size_t max_depth = 0;
    for (const auto& node : result) max_depth = std::max(max_depth, node.depth);

    for (std::size_t depth = max_depth; depth > 0; --depth) {

        cbp::microseconds child_nodes_duration_total{};

        for (std::size_t i = result.size() - 1; i != std::size_t(-1); --i) {
            auto& node = result[i];

            if (node.depth == depth) child_nodes_duration_total += node.duration_total;

            if (node.depth == depth - 1) {
                node.duration_total += child_nodes_duration_total;
                child_nodes_duration_total = cbp::microseconds{};
            }
        }
    } // TEMP: This is a mess, but performant, complexity O(nodes * max_depth)

    return result;
}


// --- Analyze translation unit ---
// --------------------------------

cbp::tree cbp::analyze_translation_unit(std::string_view path) {
    // Read the trace & order events chronologically
    auto trace = cbp::read_file_json<cbp::trace>(path);

    std::stable_sort(trace.events.begin(), trace.events.end(),
                     [](const cbp::trace::event& a, const cbp::trace::event& b) { return a.time < b.time; });

    if (trace.events.empty()) throw cbp::exception{"Trace parsed from {{ {} }} contains no events", path};
    // this should never trigger for a correct trace

    // Create root node
    auto translation_unit_node = cbp::tree::node{
        .name           = normalize_path(path),                                 //
        .time           = trace.events.front().time,                            //
        .duration_total = trace.events.back().time - trace.events.front().time, //
        .duration_self  = cbp::microseconds{},                                  //
        .depth          = 0,                                                    //
        .type           = cbp::tree::node_type::translation_unit                //
    };

    auto nodes = std::vector{std::move(translation_unit_node)};

    // Build the "Parsing" subtree
    auto parsing_events  = extract_parsing_events(trace.events);
    auto parsing_subtree = build_parsing_subtree(parsing_events);

    // Build "Instantiation" subtree
    auto instantiation_events  = extract_instantiation_events(trace.events);
    auto instantiation_subtree = build_instantiation_subtree(instantiation_events, parsing_subtree);

    // Build the resulting translation unit subtree
    nodes.insert(nodes.end(), std::make_move_iterator(parsing_subtree.begin()),
                 std::make_move_iterator(parsing_subtree.end()));
    nodes.insert(nodes.end(), std::make_move_iterator(instantiation_subtree.begin()),
                 std::make_move_iterator(instantiation_subtree.end()));

    return cbp::tree{.nodes = std::move(nodes)};
}
