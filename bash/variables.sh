# __________________________________ CONTENTS ___________________________________
#
#    All the variables used by other scripts, edit this file to configure
#       - directories
#       - script paths
#       - default CMake preset
#       - coverage
#       ...
# _______________________________________________________________________________

# ===================
# ---- Constants ----
# ===================

path_executable="build/clang-build-profiler"

directory_build="build/"

script_run_static_analysis="bash/run_static_analysis.sh"

cppcheck_suppressions_file=".cppcheck"
cppcheck_cache_directory=".cache-cppcheck"

# =======================
# ---- Configuration ----
# =======================

preset="clang"
