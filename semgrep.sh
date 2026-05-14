#!/bin/bash
# ==============================================================================
# C24/CIRRUS Migration - GitLab SAST CLI Tool
# Author: Toomey (May 2026)
#
# PURPOSE:
# This script replicates the GitLab Static Application Security Testing (SAST) 
# environment locally. It automatically prefers local binaries (semgrep, jq) 
# for speed, but falls back to Podman/Docker containers if they are missing.
#
# HOW TO USE THIS SCRIPT:
# Make the script executable first: chmod +x semgrep.sh
#
# EXAMPLE 1: The "Set it and Forget it" Mode (Recommended)
#   Command:  ./semgrep.sh export
#   Action:   Runs a fresh scan and generates three human-readable text files 
#             in a dated folder to easily share with the team or Jira.
#
# EXAMPLE 2: The "Quick Summary" Mode
#   Command:  ./semgrep.sh summary
#   Action:   Reads the latest scan and prints a brief tally of the findings
#             straight to your terminal (e.g., "4 [HIGH] Buffer Overflow").
#
# EXAMPLE 3: Custom File Piping
#   Command:  ./semgrep.sh action > toomey-fixes.txt
#   Action:   Prints only the file names and line numbers where fixes are 
#             needed and saves them into your own custom text file.
#
# AVAILABLE COMMANDS: scan, summary, action, full, export
# ==============================================================================

REPODIR="$(cd "$(dirname "$0")" && pwd)"
SCAN_IMAGE="registry.gitlab.com/gitlab-org/security-products/analyzers/semgrep:5"
JQ_IMAGE="docker.io/semgrep/semgrep"

# Default to the current directory name if PROJECT is not set
# ${PWD##*/} strips the full path down to just the folder name
: "${PROJECT:=${PWD##*/}}"

# Generate a timestamp and define the new output directory
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
REPORTS_DIR="$REPODIR/${PROJECT}-reports-$TIMESTAMP"

# The actual JQ logic for parsing the JSON into human-readable formats.
# __FILE__ is a placeholder that gets replaced dynamically depending on local vs docker execution.
cmd_summary="jq -r '.vulnerabilities[]? | \"[\(.severity | ascii_upcase)] \(.name)\"' __FILE__ | sort | uniq -c | sort -nr"
cmd_action="jq -r '.vulnerabilities[]? | \"\(.location.file):\(.location.start_line) \n  ↳ [\(.severity | ascii_upcase)] \(.name)\n\"' __FILE__"
cmd_full="jq -r '.vulnerabilities[]? | \"FILE: \(.location.file):\(.location.start_line)\nSEVERITY: \(.severity | ascii_upcase)\nISSUE: \(.name)\nDETAILS: \(.description)\n----------------------------------------\"' __FILE__"

# Helper function to run jq commands, falling back to Docker if jq isn't installed
run_jq() {
    local cmd_template="$1"
    local target_file="$2"
    
    if command -v jq &> /dev/null; then
        # Run locally using sh -c to safely handle the pipes and quotes
        local local_cmd="${cmd_template/__FILE__/\"$REPODIR/$target_file\"}"
        sh -c "$local_cmd"
    else
        # Run via Docker
        local docker_cmd="${cmd_template/__FILE__/\"/src/$target_file\"}"
        docker run --rm -v "$REPODIR:/src:z" "$JQ_IMAGE" sh -c "$docker_cmd"
    fi
}

run_scan() {
    echo "--- 🔍 Running SAST Scanner for ${PROJECT} ---"
    
    if command -v semgrep &> /dev/null; then
        echo "⚡ Local 'semgrep' binary detected. Running fast scan..."
        # Uses --gitlab-sast flag to natively format the output exactly like the pipeline
        semgrep scan --config=auto --gitlab-sast \
            --exclude="spec" --exclude="test" --exclude="tests" --exclude="tmp" \
            -o "$REPODIR/gl-sast-report.json" "$REPODIR"
    else
        echo "🐳 Local semgrep not found. Falling back to Docker container..."
        # Mounts the local code to /tmp/app (where GitLab expects it)
        # Sets SELinux :z flag for Oracle Linux compatibility
        docker run --rm \
            -v "$REPODIR:/tmp/app:z" \
            -e CI_PROJECT_DIR=/tmp/app \
            -e SAST_EXCLUDED_PATHS="spec,test,tests,tmp" \
            -w /tmp/app \
            "$SCAN_IMAGE" /analyzer run
    fi
        
    # Move the file into the project-named dated folder immediately
    if [ -f "$REPODIR/gl-sast-report.json" ]; then
        mkdir -p "$REPORTS_DIR"
        mv "$REPODIR/gl-sast-report.json" "$REPORTS_DIR/gl-sast-report.json"
        echo "--- ✅ Scan Complete (Saved to ${PROJECT}-reports-${TIMESTAMP}/) ---"
    else
        echo "--- ❌ Scan Failed: No JSON report was generated ---"
    fi
}

# Helper function to dynamically grab the newest JSON report across all project folders
get_latest_report() {
    local latest
    # Search for the newest JSON file inside any matching project reports directory
    latest=$(ls -t "$REPODIR"/${PROJECT}-reports-*/gl-sast-report.json 2>/dev/null | head -n 1)
    
    if [ -z "$latest" ]; then
        echo "No recent scan found. Running a new scan first..." >&2
        run_scan >&2
        latest=$(ls -t "$REPODIR"/${PROJECT}-reports-*/gl-sast-report.json 2>/dev/null | head -n 1)
    fi
    
    # Strip the absolute path so JQ can find it relative to its mount point or local execution
    echo "${latest#$REPODIR/}"
}

# Command Line Argument Parsing
case "$1" in
    scan)
        run_scan
        ;;
    summary)
        echo "--- 📊 BRIEF SUMMARY ---"
        REPORT_FILE=$(get_latest_report)
        run_jq "$cmd_summary" "$REPORT_FILE"
        ;;
    action)
        echo "--- 🛠️  ACTION LIST ---"
        REPORT_FILE=$(get_latest_report)
        run_jq "$cmd_action" "$REPORT_FILE"
        ;;
    full)
        echo "--- 📖 FULL CONTEXT REPORT ---"
        REPORT_FILE=$(get_latest_report)
        run_jq "$cmd_full" "$REPORT_FILE"
        ;;
    export)
        echo "--- 💾 Exporting all reports to text files... ---"
        run_scan
        REPORT_FILE="${REPORTS_DIR#$REPODIR/}/gl-sast-report.json"
        
        # Write the exports directly into the dated folder
        run_jq "$cmd_summary" "$REPORT_FILE" > "$REPORTS_DIR/sast-summary.txt"
        run_jq "$cmd_action" "$REPORT_FILE" > "$REPORTS_DIR/sast-action-list.txt"
        run_jq "$cmd_full" "$REPORT_FILE" > "$REPORTS_DIR/sast-full-report.txt"
        
        echo "📂 All reports successfully saved to: $REPORTS_DIR/"
        ;;
    *)
        echo "Usage: ./semgrep.sh [COMMAND]"
        echo "Commands:"
        echo "  scan      Runs the analyzer and creates the raw JSON in a dated project folder."
        echo "  summary   Prints vulnerability counts to the terminal."
        echo "  action    Prints file paths and line numbers to the terminal."
        echo "  full      Prints the deep-dive explanations to the terminal."
        echo "  export    Runs a fresh scan and saves all 3 formats as text files in a dated project folder."
        ;;
esac
