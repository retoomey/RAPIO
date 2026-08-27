#!/bin/bash
# ==============================================================================
# Bundle just RAPIO core for basic API questions
# ==============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export REPODIR="$(cd "$SCRIPT_DIR/.." && pwd)"  # Resolves 1 level up from script dir

source "$SCRIPT_DIR/repomix.inc.sh"

BASE="rapiobase"

# Optional: Set custom prompt text. If left commented out, defaults to default prompt.
# CUSTOM_PROMPT="Act as a C++ Performance Lead. Focus strictly on memory allocation and thread safety."

INCLUDE_LIST=(
    "base/**/*.cc"       # Core
    "base/**/*.h"
    "CMakeLists.txt"
    "base/**/.txt"
    "modules/logspd/**/*.*"
    "rexample/**/*.cc"   # Code examples
    "rexample/**/*.h"
    "rexample/CMakeLists.txt"
)

EXCLUDE_LIST=(
    "base/croncpp.h"
)

run_repobase_pipeline "$1"
