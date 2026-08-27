#!/bin/bash
# ==============================================================================
# doxygen.sh - Doxygen wrapper with container fallback
# ==============================================================================

# Resolve script directory and project root (one level up)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Define the Doxyfile location relative to this script
DOXYFILE="$SCRIPT_DIR/rapio.dox"
DOXYGEN_IMAGE="hrektts/doxygen:latest"

# Generate a timestamp and define the output folder
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
OUT_DIR="rapiodoxygen_$TIMESTAMP"

echo "--- 📚 Generating Doxygen Documentation ---"
echo "Targeting Output: $OUT_DIR/"

# Move to repo root so Doxyfile's relative INPUT paths (base, etc.) resolve properly
cd "$REPO_ROOT" || exit 1

# Check for local binary or fallback to container engine
if command -v doxygen &> /dev/null; then
    echo "⚡ Local 'doxygen' binary detected. Running generator..."
    
    # Pipe the config and append the dynamic output directory
    (cat "$DOXYFILE"; echo "OUTPUT_DIRECTORY=$OUT_DIR") | doxygen -
else
    echo "🐳 Local doxygen not found. Falling back to container engine..."
    
    ENGINE=""
    if command -v podman &> /dev/null; then
        ENGINE="podman"
    elif command -v docker &> /dev/null; then
        ENGINE="docker"
    else
        echo "Error: Neither doxygen, docker, nor podman is installed."
        exit 1
    fi

    # The -i flag keeps standard input open so we can pipe the config override
    (cat "$DOXYFILE"; echo "OUTPUT_DIRECTORY=$OUT_DIR") | $ENGINE run -i --rm \
        -v "$REPO_ROOT:/app:z" \
        -w /app \
        "$DOXYGEN_IMAGE" doxygen -
fi

echo "--- ✅ Documentation Generation Complete ($OUT_DIR/) ---"
