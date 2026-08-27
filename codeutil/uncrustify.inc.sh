# ==============================================================================
# uncrustifyhelper.sh - Shared uncrustify container fallback & file matcher
# ==============================================================================

# Guard against direct execution (Bash/Zsh)
if [ "${BASH_SOURCE[0]}" -ef "$0" ]; then
    echo "Error: $(basename "$0") must be sourced, not executed directly." >&2
    echo "Usage: source $0" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Enable recursive globbing (**) and nullglob (don't error on empty matches)
shopt -s globstar nullglob extglob

run_crust() {
  if command -v uncrustify &> /dev/null; then
    uncrustify "$@"
  else
    local engine=""
    if command -v podman &> /dev/null; then
      engine="podman"
    elif command -v docker &> /dev/null; then
      engine="docker"
    else
      echo "uncrustify does not appear to be in your path, and no container engine was found."
      exit 1
    fi

    if ! $engine image inspect local-uncrustify &> /dev/null; then
      echo "🐳 Building custom uncrustify container using $engine..."
      $engine build -t local-uncrustify -f "$SCRIPT_DIR/uncrustify.dock" "$SCRIPT_DIR"
    fi

    $engine run --rm -v "$REPO_ROOT:/app:z" -w /app local-uncrustify uncrustify "$@"
  fi
}

# Yields a deduplicated, filtered list of files based on INCLUDE_LIST and EXCLUDE_LIST
get_target_files() {
  echo "📂 Scanning for files" >&2
  for inc in "${INCLUDE_LIST[@]}"; do
    #echo "📂 Scanning pattern: $inc" >&2
    for file in $inc; do
      if [ -f "$file" ]; then
        local excluded=false
        for exc in "${EXCLUDE_LIST[@]}"; do
          # Use bash pattern matching against the exclusion string
          if [[ "$file" == $exc ]]; then
            #echo "  [-] Skipping (excluded): $file" >&2
            excluded=true
            break
          fi
        done
        
        if ! $excluded; then
        #  echo "  [+] Queued: $file" >&2
          echo "$file" # Standard output goes to sort -u
        fi
      fi
    done
  done | sort -u
  #echo "Finished!" >&2 # redirect to stderr
}

# Populates the TARGET_FILES array for the parent scripts
gather_files() {
  echo "Gathering target files..."
  mapfile -t TARGET_FILES < <(get_target_files)

  if [ ${#TARGET_FILES[@]} -eq 0 ]; then
    echo "No files found to process."
    exit 0
  fi
}
