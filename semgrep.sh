#!/bin/bash
# ==============================================================================
# C24/CIRRUS Migration - GitLab SAST CLI Tool
# Author: Toomey (May 2026)
#
# PURPOSE:
# This script replicates the GitLab Static Application Security Testing (SAST) 
# environment locally. It uses Podman/Docker to scan C/C++ code for vulnerabilities
# (like buffer overflows or unsafe functions) matching the exact enterprise rules
# used in our pipeline reports. Zero local OS installations are required.
#
# HOW TO USE THIS SCRIPT:
# Make the script executable first: chmod +x semgrep.sh
#
# EXAMPLE 1: The "Set it and Forget it" Mode (Recommended)
#   Command:  ./semgrep.sh export
#   Action:   Runs a fresh scan and generates three human-readable text files 
#             in this directory (sast-summary.txt, sast-action-list.txt, 
#             and sast-full-report.txt) to easily share with the team or Jira.
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

# Helper function to run the jq commands inside the container
run_jq() {
    local jq_cmd="$1"
    docker run --rm -v "$REPODIR:/src:z" "$JQ_IMAGE" sh -c "$jq_cmd"
}

# The actual JQ logic for parsing the JSON into human-readable formats
cmd_summary="jq -r '.vulnerabilities[]? | \"[\(.severity | ascii_upcase)] \(.name)\"' /src/gl-sast-report.json | sort | uniq -c | sort -nr"
cmd_action="jq -r '.vulnerabilities[]? | \"\(.location.file):\(.location.start_line) \n  ↳ [\(.severity | ascii_upcase)] \(.name)\n\"' /src/gl-sast-report.json"
cmd_full="jq -r '.vulnerabilities[]? | \"FILE: \(.location.file):\(.location.start_line)\nSEVERITY: \(.severity | ascii_upcase)\nISSUE: \(.name)\nDETAILS: \(.description)\n----------------------------------------\"' /src/gl-sast-report.json"

run_scan() {
    echo "--- 🔍 Running GitLab SAST Scanner ---"
    # Mounts the local code to /tmp/app (where GitLab expects it)
    # Sets SELinux :z flag for Oracle Linux compatibility
    docker run --rm \
        -v "$REPODIR:/tmp/app:z" \
        -e CI_PROJECT_DIR=/tmp/app \
        -e SAST_EXCLUDED_PATHS="spec,test,tests,tmp" \
        -w /tmp/app \
        "$SCAN_IMAGE" /analyzer run
    echo "--- ✅ Scan Complete (gl-sast-report.json generated) ---"
}

# Command Line Argument Parsing
case "$1" in
    scan)
        run_scan
        ;;
    summary)
        echo "--- 📊 BRIEF SUMMARY ---"
        run_jq "$cmd_summary"
        ;;
    action)
        echo "--- 🛠️  ACTION LIST ---"
        run_jq "$cmd_action"
        ;;
    full)
        echo "--- 📖 FULL CONTEXT REPORT ---"
        run_jq "$cmd_full"
        ;;
    export)
        echo "--- 💾 Exporting all reports to text files... ---"

        # Default to the current directory name if PROJECT is not set
        # ${PWD##*/} strips the full path down to just the folder name
        : "${PROJECT:=${PWD##*/}}"

        # Create a timestamp (e.g., 20260513_1323)
        TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

        # Define reusable filenames with the PROJECT prefix
        FILE_SUMMARY="${PROJECT}-sast-summary_$TIMESTAMP.txt"
        FILE_ACTION="${PROJECT}-sast-action-list_$TIMESTAMP.txt"
        FILE_FULL="${PROJECT}-sast-full-report_$TIMESTAMP.txt"

        # Run the scan first if the JSON doesn't exist yet
        if [ ! -f "$REPODIR/gl-sast-report.json" ]; then run_scan; fi
        
        # Execute jq and pipe to the variables
        run_jq "$cmd_summary" > "$REPODIR/$FILE_SUMMARY"
        run_jq "$cmd_action" > "$REPODIR/$FILE_ACTION"
        run_jq "$cmd_full" > "$REPODIR/$FILE_FULL"

        echo "Created: $FILE_SUMMARY"
        echo "Created: $FILE_ACTION"
        echo "Created: $FILE_FULL"
        ;;
    *)
        echo "Usage: ./semgrep.sh [COMMAND]"
        echo "Commands:"
        echo "  scan      Runs the analyzer and creates the raw JSON."
        echo "  summary   Prints vulnerability counts to the terminal."
        echo "  action    Prints file paths and line numbers to the terminal."
        echo "  full      Prints the deep-dive explanations to the terminal."
        echo "  export    Runs the scan and saves all 3 formats as text files."
        ;;
esac
