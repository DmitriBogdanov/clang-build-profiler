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
                .name  = normalize_path(event.args.at("detail").as<std::string>()), //
                .time  = event.time,                                                //
                .depth = depth,                                                     //
                .type  = cbp::tree::node_type::source_parsing                       //
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

std::vector<cbp::tree::node>
build_instantiation_subtree([[maybe_unused]] const std::vector<cbp::trace::event>& instantiation_events,
                            [[maybe_unused]] const std::vector<cbp::tree::node>&   parsing_subtree) {
    return std::vector<cbp::tree::node>{}; // TEMP:
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
