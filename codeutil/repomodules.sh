#!/bin/bash
# ==============================================================================
# Bundle core and iomodules for AI analysis
# ==============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export REPODIR="$(cd "$SCRIPT_DIR/.." && pwd)"  # Resolves 1 level up from script dir

source "$SCRIPT_DIR/repomix.inc.sh"

BASE="rapiomodules"

# Optional: Set custom prompt text. If left commented out, defaults to default prompt.
# CUSTOM_PROMPT="Act as a C++ Performance Lead. Focus strictly on memory allocation and thread safety."

# Include List: File extensions, specific files, or directories to INCLUDE
INCLUDE_LIST=(
    "base/**/*.cc"            # Core
    "base/**/*.h"
    "CMakeLists.txt"
    "base/**/.txt"
    "modules/**/*.*"          # Modules
)

# Exclude List: File extensions, specific files, or directories to IGNORE
EXCLUDE_LIST=(
    "base/croncpp.h"
)

run_repobase_pipeline "$1"
