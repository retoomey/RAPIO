# ==============================================================================
# Repohelper - AI Codebase Packager (Routines to be called by other scripts)
# Author: Toomey (August 2026)
#
# PURPOSE:
# Packages repository code into a context-optimized XML file for AI analysis.
# Prefers local binaries (repomix/npx) for speed, falling back to Docker/Podman
# if missing.
#
# ==============================================================================

# Guard against direct execution (Bash/Zsh)
if [ "${BASH_SOURCE[0]}" -ef "$0" ]; then
    echo "Error: $(basename "$0") must be sourced, not executed directly." >&2
    echo "Usage: source $0" >&2
    exit 1
fi

REPOMIX_IMAGE="ghcr.io/yamadashy/repomix:latest"

# Default Architectural Prompt
DEFAULT_ARCHITECT_PROMPT=$(cat <<'EOF'
ROLE: Act as a Senior Software Architect with 20 years of experience in system design, security, and maintainability.

TASK: I am providing a complete codebase packed into an XML file. Please perform a deep-dive architectural review. 

DELIVERABLES:
1. Project Executive Summary: High-level overview of the system, tech stack, and design patterns.
2. The "Top 5" Critical Issues: Focused on Architecture (coupling/SOLID), Scalability (bottlenecks), and Security.
3. Code Smell & Technical Debt: Identification of "God objects" or overly complex logic.
4. Strategic Recommendations: The first three refactoring priorities for long-term health.

CONSTRAINT: Be direct, critical, and objective. Prioritize structural flaws over minor style nitpicks.
EOF
)

# Utility method for generating prompts with optional custom text
generate_prompt() {
    local target_file="${1:-$REPODIR/$PROMPT_FILE}"
    local prompt_content="${2:-${CUSTOM_PROMPT:-$DEFAULT_ARCHITECT_PROMPT}}"
    
    echo "--- 📝 Generating Architectural Prompt ---"
    echo "$prompt_content" > "$target_file"
    echo "--- ✅ Prompt Ready ($target_file) ---"
}

run_repomix() {
    echo "--- 📦 Packaging Codebase for AI Ingestion ---"
    echo "Target Directory: $REPODIR"
    
    if command -v repomix &> /dev/null; then
        echo "⚡ Local 'repomix' binary detected. Packaging..."
        (cd "$REPODIR" && repomix "${REPOMIX_ARGS[@]}")
    elif command -v npx &> /dev/null; then
        echo "⚡ Local 'npx' detected. Running via npx..."
        (cd "$REPODIR" && npx --yes repomix "${REPOMIX_ARGS[@]}")
    else
        echo "🐳 Local repomix/npx not found. Falling back to Docker container..."
        docker run --rm \
            -v "$REPODIR:/app:z" \
            -w /app \
            "$REPOMIX_IMAGE" "${REPOMIX_ARGS[@]}"
    fi

    if [ -f "$REPODIR/$OUTPUT_FILE" ]; then
        cp "$REPODIR/$OUTPUT_FILE" "$REPODIR/$STAMPED_FILE"
        echo "--- ✅ Packaging Complete ---"
        echo "  ↳ Drive Target: $OUTPUT_FILE"
        echo "  ↳ Local Copy:   $STAMPED_FILE"
    else
        echo "--- ❌ Packaging Failed: Output file was not created ---"
    fi
}

run_repobase_pipeline() {
    local command="$1"

    # Default REPODIR to parent of calling script if not explicitly set
    REPODIR="${REPODIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"

    OUTPUT_FILE="${BASE}.xml"
    TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
    STAMPED_FILE="${BASE}_${TIMESTAMP}.xml"
    PROMPT_FILE="${BASE}_prompt.txt"

    REPOMIX_ARGS=(
        "--output=$OUTPUT_FILE"
        "--no-security-check"
        "--style=markdown"
        "--remove-comments"
        "--remove-empty-lines"
        "--truncate-base64"
    )

    SAVE_IFS="$IFS"
    IFS=,
    if [ ${#INCLUDE_LIST[@]} -gt 0 ]; then
        REPOMIX_ARGS+=("--include=${INCLUDE_LIST[*]}")
    fi

    if [ ${#EXCLUDE_LIST[@]} -gt 0 ]; then
        REPOMIX_ARGS+=("--ignore=${EXCLUDE_LIST[*]}")
    fi
    IFS="$SAVE_IFS"

    case "$command" in
        pack)
            run_repomix
            ;;
        prompt)
            generate_prompt
            ;;
        all|"")
            run_repomix
            generate_prompt
            echo "-------------------------------------------------------"
            echo "DONE!"
            echo "Code Packaged: $REPODIR/$OUTPUT_FILE"
            echo "Prompt Ready:  $REPODIR/$PROMPT_FILE"
            echo "-------------------------------------------------------"
            echo "Step 1: Copy the text inside $PROMPT_FILE"
            echo "Step 2: Upload $OUTPUT_FILE to your AI model"
            echo "Step 3: Paste the prompt and submit."
            ;;
        *)
            echo "Usage: $0 [COMMAND]"
            echo "Commands:"
            echo "  pack    Runs Repomix (Local Binary -> NPX -> Docker Container)"
            echo "  prompt  Creates or updates the $PROMPT_FILE heredoc"
            echo "  all     Runs both packaging and prompt generation (default)"
            ;;
    esac
}
