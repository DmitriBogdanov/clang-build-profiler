#include "common.hpp"

#include <filesystem>
#include <functional>

#include "external/fmt/format.h"
#include "external/fmt/chrono.h"

#include "backend/analyze.hpp"
#include "backend/invoke.hpp"
#include "backend/profile.hpp"
#include "frontend/preprocessor.hpp"


const std::filesystem::path data_dir = "tests/data/";

const std::filesystem::path builds_dir  = data_dir / "builds";
const std::filesystem::path targets_dir = data_dir / "targets";
const std::filesystem::path files_dir   = data_dir / "files";

void verify_invariants(const cbp::tree& root) {
    root.for_all([](const cbp::tree& tree) {
        fmt::println("Testing node:");
        fmt::println("   name  = {}", tree.name);
        fmt::println("   total = {}", tree.total);
        fmt::println("   self  = {}", tree.self);
        
        // Durations should be positive
        REQUIRE(tree.total >= cbp::microseconds{});
        // REQUIRE(tree.self >= cbp::microseconds{});
        
        // Parent't cannot take less time that its children
        cbp::microseconds child_total{};
        for (const auto& child : tree.children) child_total += child.total;
        
        REQUIRE(tree.total >= child_total);
        
        // Duration sum should be preserved
        REQUIRE(tree.total == tree.self + child_total);
    });
}

TEST_CASE("Tree invariants / Files") {
    for (const auto& entry : std::filesystem::directory_iterator{files_dir}) {
        fmt::println("Testing file {{ {} }}...", entry.path().string());
        
        REQUIRE(entry.is_regular_file());

        // Tree invariants are preserved initially
        fmt::println("Before preprocessing...");
        
        cbp::profile profile;
        profile.tree = cbp::analyze_translation_unit(entry.path().string());

        verify_invariants(profile.tree);

        // And after preprocessing
        fmt::println("After preprocessing...");
        
        cbp::preprocess(profile, std::filesystem::current_path().string());
        
        verify_invariants(profile.tree);
    }
}