#include "analyze.hpp"

#include <filesystem>
#include <stack>

#include "exception.hpp"
#include "prettify.hpp"
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
        return event.name == "InstantiateFunction" || event.name == "InstantiateClass";
    });
}

template <class Predicate>
std::optional<cbp::trace::event> extract_event(std::vector<cbp::trace::event>& events, Predicate&& predicate) {
    auto it = std::find_if(events.begin(), events.end(), predicate);

    if (it == events.end()) return std::nullopt;

    auto result = std::move(*it);
    events.erase(it);

    return result;
}

cbp::trace::event extract_event_by_name(std::vector<cbp::trace::event>& events, std::string_view name) {
    auto result = extract_event(events, [&](const cbp::trace::event& event) { return event.name == name; });

    if (!result) throw cbp::exception{"Could not extact event with the name {{ {} }} from trace", name};

    return result.value();
}

[[nodiscard]] std::string normalize_path(std::filesystem::path path) {
    return path.lexically_normal().string();
    // Note: Normalizing paths leads to a nicer output, otherwise we can end
    //       up with names like 'lib/bin/../include/' instead of 'lib/include/'
}


// --- Parsing subtree ---
// -----------------------

// Assuming a correct trace schema, every '#include' has a pair of correspondig begin/end events
// (event types "b" and "e"). When these events are ordered chronologically, we can deduce
// transitive includes based on the "b" / "e" event nesting. Below is a simple example:
//
// Events:
//    > root-b      | call 'handle_parsing_event(events, root,   1)'
//    >    child1-b | call 'handle_parsing_event(events, child1, 2)'
//    >    child1-e | end  'handle_parsing_event(events, child1, 2)'
//    >    child2-b | call 'handle_parsing_event(events, child2, 4)'
//    >    child2-e | end  'handle_parsing_event(events, child2, 4)'
//    > root-e      | end  'handle_parsing_event(events, root,   1)'
//
// Resulting tree:
//    > root
//    >    child1
//    >    child2

// Events:
//    > root-b      | parent: parsing | cursor: 0 | creates 'root'
//    >    child1-b | parent: root    | cursor: 1 | creates 'child1'
//    >    child1-e | parent: child1  | cursor: 2 | ends    'child1'
//    >    child2-b | parent: root    | cursor: 3 | creates 'child2'
//    >    child2-e | parent: child2  | cursor: 4 | ends    'child2'
//    > root-e      | parent: root    | cursor: 5 | ends    'root'

// Events:
//    > root-b    | parent: parsing
//      > child-b | parent: root
//      > child-e | parent: child
//    > root-e    | parent: root

using event_span = std::span<const cbp::trace::event>;

void handle_parsing_event(event_span parsing_events, cbp::tree& parent, std::size_t& cursor) {
    // Include began
    const auto& begin_event = parsing_events[cursor++];

    if (begin_event.type != "b") throw cbp::exception{"Incorrect trace schema, event type mismatch at pos {}", cursor};

    auto current = cbp::tree{
        .type  = cbp::tree_type::parse,                      //
        .name  = begin_event.args.at("detail").get_string(), //
        .total = -begin_event.time                           //
    };

    // Handle transitive includes
    if (cursor >= parsing_events.size()) throw cbp::exception{"Incorrect trace schema, source events end with begin."};

    while (parsing_events[cursor].type == "b") handle_parsing_event(parsing_events, current, cursor);

    // Include ended
    const auto& end_event = parsing_events[cursor++];

    if (end_event.type != "e") throw cbp::exception{"Incorrect trace schema, event type mismatch at pos {}", cursor};

    // Gather total & self duration
    current.total += end_event.time;

    current.self = current.total;
    for (const auto& child : current.children) current.self -= child.total;

    // Finalize
    std::sort(current.children.begin(), current.children.end());

    parent.children.push_back(std::move(current));
}

cbp::tree build_parsing_subtree(event_span parsing_events) {
    // Root node
    auto parsing_tree = cbp::tree{.type = cbp::tree_type::parsing, .name = "Parsing"};

    // Create child nodes from events
    for (std::size_t cursor = 0; cursor < parsing_events.size();)
        handle_parsing_event(parsing_events, parsing_tree, cursor);

    std::sort(parsing_tree.children.begin(), parsing_tree.children.end());

    // Gather total duration for the root node
    for (const auto& child : parsing_tree.children) parsing_tree.total += child.total;
    // TODO: How do we get a self-parsing time? Is that even possible?

    return parsing_tree;
}

// --- Instantiation subtree ---
// -----------------------------

void handle_instantiation_event(event_span instantiation_events, cbp::tree& parent, std::size_t& cursor) {
    // Instantiation began
    const auto& event = instantiation_events[cursor];

    auto current = cbp::tree{
        .type  = cbp::tree_type::instantiate,          //
        .name  = event.args.at("detail").get_string(), //
        .total = event.duration.value()                //
    };

    const cbp::microseconds event_end_time = event.time + current.total;

    // Handle nested instantiations
    while (++cursor < instantiation_events.size() && instantiation_events[cursor].time < event_end_time)
        handle_instantiation_event(instantiation_events, current, cursor);

    // Instantiation ended, gather self-duration
    current.self = current.total;
    for (const auto& child : current.children) current.self -= child.total;

    // Finalize
    std::sort(current.children.begin(), current.children.end());

    parent.children.push_back(std::move(current));
}

cbp::tree build_instantiation_subtree(event_span instantiation_events) {
    // Root node
    auto instantiation_tree = cbp::tree{.type = cbp::tree_type::instantiation, .name = "Template instantiation"};

    // Create child nodes from events
    for (std::size_t cursor = 0; cursor < instantiation_events.size();)
        handle_instantiation_event(instantiation_events, instantiation_tree, cursor);

    std::sort(instantiation_tree.children.begin(), instantiation_tree.children.end());

    // Gather total duration for the root node
    for (const auto& child : instantiation_tree.children) instantiation_tree.total += child.total;

    return instantiation_tree;
}


// --- Analyze translation unit ---
// --------------------------------

cbp::tree cbp::analyze_translation_unit(std::string_view path) {
    // Read the trace & order events chronologically
    auto trace = cbp::read_file_json<cbp::trace>(path);

    if (trace.events.empty()) throw cbp::exception{"Trace parsed from {{ {} }} contains no events", path};
    // this should never trigger for a correct trace

    std::stable_sort(trace.events.begin(), trace.events.end());

    // Create root node
    auto translation_unit_tree = cbp::tree{
        .type  = cbp::tree_type::translation_unit,                    //
        .name  = normalize_path(path),                                //
        .total = trace.events.back().time - trace.events.front().time //
    };

    // Build the "Parsing" subtree
    auto parsing_events  = extract_parsing_events(trace.events);
    auto parsing_subtree = build_parsing_subtree(parsing_events);

    // Build "Template instantiation" subtree
    auto instantiation_events  = extract_instantiation_events(trace.events);
    auto instantiation_subtree = build_instantiation_subtree(instantiation_events);

    // Build "LLVM IR generation" subtree
    extract_event_by_name(trace.events, "Frontend");
    auto llvm_codegen_event    = extract_event_by_name(trace.events, "Frontend");
    auto llvm_codegen_duration = llvm_codegen_event.duration.value();
    auto llvm_codegen_subtree  = cbp::tree{
         .type  = cbp::tree_type::llvm_codegen, //
         .name  = "LLVM IR generation",         //
         .total = llvm_codegen_duration,        //
         .self  = llvm_codegen_duration         //
    };

    // Note: There are 2 "Frontend" events, 1st contains parsing + instantiation total, 2nd contains LLVM IR
    //       codegen total, we already handled parsing + instantiation so we can throw the first one away

    // Build "Optimization" subtree
    auto optimization_event    = extract_event_by_name(trace.events, "Optimizer");
    auto optimization_duration = optimization_event.duration.value();
    auto optimization_subtree  = cbp::tree{
         .type  = cbp::tree_type::optimization, //
         .name  = "Optimization",               //
         .total = optimization_duration,        //
         .self  = optimization_duration         //
    };

    // Build "Machine code generation" subtree
    auto native_codegen_event    = extract_event_by_name(trace.events, "CodeGenPasses");
    auto native_codegen_duration = native_codegen_event.duration.value();
    auto native_codegen_subtree  = cbp::tree{
         .type  = cbp::tree_type::native_codegen, //
         .name  = "Machine code generation",      //
         .total = native_codegen_duration,        //
         .self  = native_codegen_duration         //
    };

    // Add them to the translation unit tree
    translation_unit_tree.children.push_back(std::move(parsing_subtree));
    translation_unit_tree.children.push_back(std::move(instantiation_subtree));
    translation_unit_tree.children.push_back(std::move(llvm_codegen_subtree));
    translation_unit_tree.children.push_back(std::move(optimization_subtree));
    translation_unit_tree.children.push_back(std::move(native_codegen_subtree));

    // Prettify names
    translation_unit_tree.for_all_children([](cbp::tree& child) {
        if (child.type == cbp::tree_type::parse) child.name = normalize_path(std::move(child.name));
        if (child.type == cbp::tree_type::instantiate) child.name = cbp::symbol::prettify(std::move(child.name));
    });

    return translation_unit_tree;
}
