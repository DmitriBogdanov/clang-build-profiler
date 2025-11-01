// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "backend/analyze.hpp"

#include "backend/trace.hpp"
#include "utility/exception.hpp"


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

std::optional<cbp::trace::event> extract_event_by_name(std::vector<cbp::trace::event>& events, std::string_view name) {
    return extract_event(events, [&](const cbp::trace::event& event) { return event.name == name; });
}


// --- Parsing subtree ---
// -----------------------

// Assuming a correct trace schema, every '#include' has a pair of correspondig begin/end events
// (event types "b" and "e"). When these events are ordered chronologically, we can deduce
// transitive includes based on the "b" / "e" event nesting. Below is a simple example:
//
// Events:
//    > root-b      | parent: parsing | current: root   | cursor: 0 | creates 'root'
//    >    child1-b | parent: root    | current: child1 | cursor: 1 | creates 'child1'
//    >    child1-e | parent: root    | current: child1 | cursor: 2 | ends    'child1'
//    >    child2-b | parent: root    | current: child2 | cursor: 3 | creates 'child2'
//    >    child2-e | parent: root    | current: child2 | cursor: 4 | ends    'child2'
//    > root-e      | parent: root    | current: root   | cursor: 5 | ends    'root'
//
// Resulting tree:
//    > root
//    >    child1
//    >    child2
//
// While counting the time we also have to take into consideration that while most template instantiation is deferred
// for later, clang can (and will) instantiate some templates early during parsing, their time should be subtracted.

using event_span = std::span<const cbp::trace::event>;

void handle_parsing_event(event_span parsing_events, cbp::tree& parent, std::size_t& cursor, //
                          event_span instantiation_events, std::size_t& instantiation_cursor //
) {
    // Include began
    const auto& begin_event = parsing_events[cursor++];

    if (begin_event.type != "b") throw cbp::exception{"Incorrect trace schema, event type mismatch at pos {}", cursor};

    auto current = cbp::tree{
        .type = cbp::tree_type::parse,                     //
        .name = begin_event.args.at("detail").get_string() //
    };

    // Handle transitive includes
    if (cursor >= parsing_events.size()) throw cbp::exception{"Incorrect trace schema, source events end with begin."};

    while (parsing_events[cursor].type == "b")
        handle_parsing_event(parsing_events, current, cursor, instantiation_events, instantiation_cursor);

    // Include ended
    const auto& end_event = parsing_events[cursor++];

    current.total = end_event.time - begin_event.time;

    if (end_event.type != "e") throw cbp::exception{"Incorrect trace schema, event type mismatch at pos {}", cursor};

    // Substract internal template instantiation time
    cbp::microseconds last_instantiation_end = cbp::microseconds{};

    while (instantiation_cursor < instantiation_events.size()) {
        const auto& event = instantiation_events[instantiation_cursor];

        const cbp::microseconds begin    = event.time;
        const cbp::microseconds duration = event.duration.value(); // always present in valid schema
        const cbp::microseconds end      = begin + duration;

        const bool nested_in_parse_event = (begin_event.time <= begin) && (end <= end_event.time);

        const bool nested_instantiation = end < last_instantiation_end;

        if (!nested_in_parse_event) break;

        ++instantiation_cursor;

        if (!nested_instantiation) {      // this check prevents us from double-counting nested
            last_instantiation_end = end; // instantiations we only care about the top-level timing
            current.carry -= duration;
        }
    }

    // Finalize
    std::sort(current.children.begin(), current.children.end());

    parent.children.push_back(std::move(current));
}

cbp::tree build_parsing_subtree(event_span parsing_events, event_span instantiation_events) {
    // Root node
    auto parsing_tree = cbp::tree{.type = cbp::tree_type::parsing, .name = "Parsing"};

    // Create child nodes from events
    for (std::size_t cursor = 0, instantiation_cursor = 0; cursor < parsing_events.size();)
        handle_parsing_event(parsing_events, parsing_tree, cursor, instantiation_events, instantiation_cursor);

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

    auto current = cbp::tree{.type  = cbp::tree_type::instantiate,          //
                             .name  = event.args.at("detail").get_string(), //
                             .total = event.duration.value()};

    const cbp::microseconds event_end_time = event.time + current.total;

    // Handle nested instantiations
    while (++cursor < instantiation_events.size() && instantiation_events[cursor].time < event_end_time)
        handle_instantiation_event(instantiation_events, current, cursor);

    // Instantiation ended

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

// --- Duration carry ---
// ----------------------

cbp::microseconds carry_duration(cbp::tree& tree) {
    // Gather duration carry & compute self-duration in the same pass
    cbp::microseconds children_carry = cbp::microseconds{};
    cbp::microseconds children_total = cbp::microseconds{};

    for (auto& child : tree.children) {
        children_carry += carry_duration(child);
        children_total += child.total;
    }

    tree.carry += children_carry;
    tree.total += tree.carry;
    tree.self = tree.total - children_total;

    // Propagate carry upwards in the recursion
    return std::exchange(tree.carry, cbp::microseconds{});
}

// --- Analysis ---
// ----------------

cbp::tree cbp::analyze_trace(cbp::trace trace, std::string_view name) try {
    if (trace.events.empty()) throw cbp::exception{"Could not analyze an empty trace"};
    // this should never trigger for a correct trace

    std::stable_sort(trace.events.begin(), trace.events.end());

    // Create root node
    auto translation_unit_tree = cbp::tree{
        .type  = cbp::tree_type::translation_unit,                    //
        .name  = std::string{name},                                   //
        .total = trace.events.back().time - trace.events.front().time //
    };

    // Parsing & instantiations events can be interweaved, need to be handled together
    const auto parsing_events       = extract_parsing_events(trace.events);
    const auto instantiation_events = extract_instantiation_events(trace.events);

    // Build the "Parsing" subtree
    if (!parsing_events.empty()) {
        auto parsing_subtree = build_parsing_subtree(parsing_events, instantiation_events);

        translation_unit_tree.children.push_back(std::move(parsing_subtree));
    }

    // Build "Template instantiation" subtree
    if (!instantiation_events.empty()) {
        auto instantiation_subtree = build_instantiation_subtree(instantiation_events);

        translation_unit_tree.children.push_back(std::move(instantiation_subtree));
    }

    // Build "LLVM IR generation" subtree
    [[maybe_unused]] auto first_frontend_event = extract_event_by_name(trace.events, "Frontend");

    if (auto llvm_codegen_event = extract_event_by_name(trace.events, "Frontend")) {
        auto llvm_codegen_duration = llvm_codegen_event->duration.value();
        auto llvm_codegen_subtree  = cbp::tree{
             .type  = cbp::tree_type::llvm_codegen, //
             .name  = "LLVM IR generation",         //
             .total = llvm_codegen_duration,        //
             .self  = llvm_codegen_duration         //
        };

        translation_unit_tree.children.push_back(std::move(llvm_codegen_subtree));
    }

    // Note: There are 2 "Frontend" events, 1st contains parsing + instantiation total, 2nd contains LLVM IR
    //       codegen total, we already handled parsing + instantiation so we can throw the first one away

    // Build "Optimization" subtree
    if (auto optimization_event = extract_event_by_name(trace.events, "Total Optimizer")) {
        auto optimization_duration = optimization_event->duration.value();
        auto optimization_subtree  = cbp::tree{
             .type  = cbp::tree_type::optimization, //
             .name  = "Optimization",               //
             .total = optimization_duration,        //
             .self  = optimization_duration         //
        };

        translation_unit_tree.children.push_back(std::move(optimization_subtree));
    }

    // Build "Machine code generation" subtree
    if (auto native_codegen_event = extract_event_by_name(trace.events, "Total CodeGenPasses")) {
        auto native_codegen_duration = native_codegen_event->duration.value();
        auto native_codegen_subtree  = cbp::tree{
             .type  = cbp::tree_type::native_codegen, //
             .name  = "Machine code generation",      //
             .total = native_codegen_duration,        //
             .self  = native_codegen_duration         //
        };

        translation_unit_tree.children.push_back(std::move(native_codegen_subtree));
    }

    // Compute resulting total & self durations
    const cbp::microseconds root_carry = carry_duration(translation_unit_tree);
    translation_unit_tree.total -= root_carry; // cancel out the upwards carry, whatever duration we can't attribute
    translation_unit_tree.self -= root_carry;  // to anything else we will attribute to the translation unit itself

    return translation_unit_tree;

} catch (std::exception& e) { throw cbp::exception{"Could not analyze trace, error:\n{}", e.what()}; }
