#!/bin/bash
# ==============================================================================
# Toomey Aug 2026
# Runs uncrustify to pretty format code
# prettyprint.sh - Check or Apply uncrustify formatting
#
# Usage:
#   ./prettyprint.sh <-c|-w> [directories...]
#
# Examples:
#   ./prettyprint.sh -c                 # Check default directories
#   ./prettyprint.sh --check base tests # Check specific folders
#   ./prettyprint.sh -w                 # Format default directories
#   ./prettyprint.sh --write base       # Format specific folders
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/uncrustify.inc.sh"

cd "$REPO_ROOT" || exit 1

ACTION=""
TARGET_DIRS=()

print_help() {
  echo "Usage: $0 <-c|-w> [directories...]"
  echo "  -c, --check    Check formatting without modifying files"
  echo "  -w, --write    Apply formatting to files (OVERWRITES files)"
  echo "  -h, --help     Show this help message"
  echo ""
  echo "Examples:"
  echo "  $0 -c                 # Check default directories"
  echo "  $0 --check base tests # Check specific directories"
  echo "  $0 -w                 # Format default directories"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--check)
      ACTION="check"
      shift
      ;;
    -w|--write)
      ACTION="write"
      shift
      ;;
    -h|--help)
      print_help
      exit 0
      ;;
    *)
      # Any non-flag arguments are treated as target directories
      TARGET_DIRS+=("$1")
      shift
      ;;
  esac
done

# Require an explicit action flag
if [ -z "$ACTION" ]; then
  print_help
  exit 1
fi

# Build INCLUDE_LIST dynamically if directories were passed, otherwise use defaults
if [ ${#TARGET_DIRS[@]} -gt 0 ]; then
  INCLUDE_LIST=()
  for f in "${TARGET_DIRS[@]}"; do
    INCLUDE_LIST+=("$f/**/*.cc" "$f/**/*.h")
  done
else
  INCLUDE_LIST=(
    "base/**/*.cc"
    "base/**/*.h"
    "rexample/**/*.cc"
    "rexample/**/*.h"
    "PYTHON/**/*.cc"
    "PYTHON/**/*.h"
    "tests/**/*.cc"
    "tests/**/*.h"
    "programs/**/*.cc"
    "programs/**/*.h"
  )
fi

# Replicates your custom exclusion logic
EXCLUDE_LIST=(
  "**/w*.cc"
  "**/w*.h"
  "base/croncpp.h" # Special stuff in here not our code either
  "programs/polar/**.*" # Ignore polar for now
)

# Leverage the helper to build the TARGET_FILES array
gather_files

if [ "$ACTION" == "write" ]; then
  # ----------------------------------------------------------------------------
  # WRITE MODE
  # ----------------------------------------------------------------------------
  echo "uncrustify environment ready. Typically I pretty format before git adding/checking in files."
  read -p "This will OVERWRITE ${#TARGET_FILES[@]} files with pretty printed copies. Are you sure? [Y/N] >" -n 1 -r
  echo
  
  if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Formatting ${#TARGET_FILES[@]} files (Pass 1)..."
    run_crust -c "$(basename "$SCRIPT_DIR")/uncrustify.cfg" --no-backup "${TARGET_FILES[@]}" | grep FAIL
    
    echo "Formatting ${#TARGET_FILES[@]} files (Pass 2 - fixing quirks)..."
    run_crust -c "$(basename "$SCRIPT_DIR")/uncrustify.cfg" --no-backup "${TARGET_FILES[@]}" | grep FAIL
    
    if [ -f "CMakeLists.txt" ]; then
      echo "Syncing file permissions..."
      chown --reference=CMakeLists.txt "${TARGET_FILES[@]}"
    fi
    
    echo "Format complete."
  else
    echo "Operation aborted by user."
  fi

elif [ "$ACTION" == "check" ]; then
  # ----------------------------------------------------------------------------
  # CHECK MODE
  # ----------------------------------------------------------------------------
  echo "Checking pretty print of ${#TARGET_FILES[@]} files... (This may take a moment)"
  
  # Execute uncrustify ONCE for all files
  run_crust -c "$(basename "$SCRIPT_DIR")/uncrustify.cfg" --check "${TARGET_FILES[@]}" | grep FAIL
  
  echo "Check complete."
fi
