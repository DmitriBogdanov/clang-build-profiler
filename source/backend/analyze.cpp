// ____________________________________ LICENSE ____________________________________
//
// Source repo: https://github.com/DmitriBogdanov/clang-build-profiler
//
// This project is licensed under the MIT License.
// _________________________________________________________________________________

#include "backend/analyze.hpp"

#include "external/fmt/chrono.h" // TEMP:
#include <stack>

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
//    > root-b      | parents.top(): parsing | creates 'root'  , expands stack
//    >    child1-b | parents.top(): root    | creates 'child1', expands stack
//    >    child1-e | parents.top(): child1  | ends    'child1', shrinks stack
//    >    child2-b | parents.top(): root    | creates 'child2', expands stack
//    >    child2-e | parents.top(): child2  | ends    'child2', shrinks stack
//    > root-e      | parents.top(): root    | ends    'root'  , shrinks stack
//
// Resulting tree:
//    > parsing
//    >    root
//    >       child1
//    >       child2
//
// While counting the time we also have to take into consideration that while most template instantiation is deferred
// for later, clang can (and will) instantiate some templates early during parsing, their time should be subtracted.
//
// The easiest way to arrange it all is to merge parsing & instantiation events into a single chronologically
// ordered array and iterate it, while keeping a manual track of the node stack. Recursion and non-merged handling
// looks like a more efficient options, but it makes attributing instantiations to parsing nodes too difficult.

cbp::tree build_parsing_subtree(std::vector<cbp::trace::event> parsing_events,
                                std::vector<cbp::trace::event> instantiation_events) {
    // Merge events & order them chronologically
    std::vector<cbp::trace::event> events = std::move(parsing_events);
    events.insert(events.end(), std::make_move_iterator(instantiation_events.begin()),
                  std::make_move_iterator(instantiation_events.end()));

    std::stable_sort(events.begin(), events.end());

    // Root node
    auto parsing_tree = cbp::tree{.type = cbp::tree_type::parsing, .name = "Parsing"};

    // Parse events while keeping a manual track of the node stack, the stack
    // will always have at least 1 element due to the begin-end event maching
    std::stack<cbp::tree*> parents;
    parents.push(&parsing_tree);

    cbp::microseconds last_instantiation_end = cbp::microseconds::min();

    for (const auto& event : events) {
        auto& current = parents.top();

        // Parse event
        if (event.name == "Source") {
            // Include began
            if (event.type == "b") {
                current->children.push_back({
                    .type  = cbp::tree_type::parse,                //
                    .name  = event.args.at("detail").get_string(), //
                    .total = -event.time                           //
                });
                parents.push(&current->children.back());
            }
            // Include ended
            else {
                current->total += event.time;
                parents.pop();

                if (parents.empty()) throw cbp::exception{"Incorrect trace schema: 'Source' event begin-end mismatch"};
            }
        }
        // Instantiation events
        else {
            if (event.time < last_instantiation_end) continue; // nested instantiation, skip
            if (parents.size() == 1) continue;                 // not during parsing, skip

            const auto duration = event.duration.value();

            current->carry -= duration; // always has a value in a correct schema
            last_instantiation_end = event.time + duration;
        }
    }

    // Gather root total
    for (const auto& child : parsing_tree.children) parsing_tree.total += child.total;

    return parsing_tree;
}

using event_span = std::span<const cbp::trace::event>;

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

    parent.children.push_back(std::move(current));
}

cbp::tree build_instantiation_subtree(event_span instantiation_events) {
    // Root node
    auto instantiation_tree = cbp::tree{.type = cbp::tree_type::instantiation, .name = "Template instantiation"};

    // Create child nodes from events
    for (std::size_t cursor = 0; cursor < instantiation_events.size();)
        handle_instantiation_event(instantiation_events, instantiation_tree, cursor);

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

    // The children might've got reordered after a carry, which means now (or later) is the only
    // correct time to perform sorting, compilations stage order however is to be preserved
    if (tree.type != cbp::tree_type::translation_unit) std::stable_sort(tree.children.begin(), tree.children.end());

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
