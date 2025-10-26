# __________________________________ CONTENTS ___________________________________
#   
#   This script contains shortcuts for building, running
#   and testing the project. All action keywords can be
#   chained which causes them to be executed one after another.
#
#   See "docs/guide_building_project.md" for the whole building guide.
#   
# ____________________________________ GUIDE ____________________________________
#   
#   Usage format:
#     > bash actions.sh [ACTIONS]
#   
#   Actions:
#     clear    - Clears "build/" folder
#     config   - Configures CMake with appropriate args
#     build    - Builds the project (requires configured CMake)
#     test     - Runs CTest tests (requires successful build)
#     coverage - Runs CTest coverage analysis (requires executed tests)
#     check    - Runs cppcheck static analysis (requires successful build)
#   
#   Usage example:
#     > bash actions.sh clear config build test
# _______________________________________________________________________________

# =======================
# ------ Functions ------
# =======================

source bash/variables.sh
source bash/functions.sh

command_clear() {
    # Clear 'build/'
    if [ -d "${directory_build}" ]; then
        rm -r "${directory_build}" # Note: 'rm --recursive' is a GNU-style extension and isn't supported on MacOS
        printf "Cleared directory [ \"${directory_build}\" ].\n"
    else
        printf "Directory [ \"${directory_build}\" ] is clear.\n"
    fi
    
    # Clear cppcheck cache
    if [ -d "${cppcheck_cache_directory}" ]; then
        rm -r "${cppcheck_cache_directory}"
        printf "Cleared directory [ \"${cppcheck_cache_directory}\" ].\n"
    else
        printf "Directory [ \"${cppcheck_cache_directory}\" ] is clear.\n"
    fi
}

command_config() {
    require_command_exists "cmake"
    cmake --preset "${preset}"
}

command_build() {
    # Run CMake build
    require_command_exists "cmake"
    cmake --build --preset "${preset}"
}

command_run() {
    ./${path_executable}
    
}

command_check() {
    # Invoke script to run static analyzers
    if [ -f "${script_run_static_analysis}" ]; then
        printf "${ansi_green}Running static analyzers...${ansi_reset}\n"
        bash "${script_run_static_analysis}"
        printf "${ansi_green}Analysis complete.${ansi_reset}\n"
    else
        printf "${ansi_red}# Error: Could not find \"${script_run_static_analysis}\".${ansi_reset}\n"
    fi
}

command_docs() {
    require_command_exists "mkdocs"
    mkdocs serve
}

# =======================
# --- Action selector ---
# =======================

valid_command=false

for var in "$@"
do
    valid_command=false
    
    if [ "${var}" = "clear" ]; then
        printf "${ansi_purple}# Action: Clear Files${ansi_reset}\n"
        command_clear
        valid_command=true
    fi

    if [ "${var}" = "config" ]; then
        printf "${ansi_purple}# Action: CMake Configure${ansi_reset}\n"
        command_config
        valid_command=true
    fi

    if [ "${var}" = "build" ]; then
        printf "${ansi_purple}# Action: CMake Build${ansi_reset}\n"
        command_build
        valid_command=true
    fi
    
    if [ "${var}" = "run" ]; then
        printf "${ansi_purple}# Action: Run executable${ansi_reset}\n"
        command_run
        valid_command=true
    fi
    
    if [ "${var}" = "check" ]; then
        printf "${ansi_purple}# Action: Run Static Analysis${ansi_reset}\n"
        command_check
        valid_command=true
    fi
    
    if [ "${var}" = "docs" ]; then
        printf "${ansi_purple}# Action: Build local documentation${ansi_reset}\n"
        command_docs
        valid_command=true
    fi
    
    if [ ${valid_command} = false ]; then
        printf "${ansi_red}# Error: Invalid action name -> ${var}${ansi_reset}\n"
        break
    fi

done