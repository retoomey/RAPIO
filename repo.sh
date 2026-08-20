#!/bin/bash
# Toomey Jan 2026

# Create a dated xml file for uploading to an AI with a large
# token context such a Gemini Pro, etc.
# Most AI uploads limit the -number- of files, such merging
# into a single file is needed.

# Also creates a prompt for AI in order to analyze
# and breakdown issues. Feel free to improve this if you
# figure out a better one.

# One of our goals is for the rapio core to be continually
# streamlined and improved upon, since we run tens of thousands
# of processes. 

#############################################################################
# CONFIGURATION
#############################################################################

# Install repomix (true/false)
INSTALLREPOMIX=false

# Include List: Specify file extensions, specific files, or directories to INCLUDE.
# Supports standard glob patterns.
INCLUDE_LIST=(
    "**/*.cc"
    "**/*.h"
    "**/*.txt"
)

# Exclude List: Specify file extensions, specific files, or directories to IGNORE.
# Supports standard glob patterns. Leave empty to ignore nothing.
EXCLUDE_LIST=(

    # Things not core we usually don't care about in general questions
    "base/croncpp.h" 
    "modules/**" 
    "programs/nse/**"

    "container/**"
    "modules/iogrib/**" # deprecated
    "build/**"
    "tests/**"
    "**/*.log"
)

#############################################################################
# INSTALLATION STEP
#############################################################################
if [ "$INSTALLREPOMIX" = true ]; then
    echo "Starting Installation..."

    # 1. Reset the nodejs module to clear any defaults
    sudo dnf module reset nodejs -y

    # 2. Enable the Node.js 20 stream
    sudo dnf module enable nodejs:20 -y

    # 3. Install Node.js (includes npm and npx)
    sudo dnf install nodejs -y

    # 4. Install Repomix globally
    sudo npm install -g repomix

    echo "Installation complete."
fi

#############################################################################
# EXECUTION
#############################################################################

# 1. Generate a timestamp
TIMESTAMP=$(date +"%Y-%m-%d_%H%M")
OUTPUT_FILE="rapio_${TIMESTAMP}.xml"
PROMPT_FILE="architect_prompt.txt"

# 2. Create the Prompt File (Heredoc)
cat <<EOF > "$PROMPT_FILE"
ROLE: Act as a Senior Software Architect with 20 years of experience in system design, security, and maintainability.

TASK: I am providing a complete codebase packed into an XML file. Please perform a deep-dive architectural review. 

DELIVERABLES:
1. Project Executive Summary: High-level overview of the system, tech stack, and design patterns.
2. The "Top 5" Critical Issues: Focused on Architecture (coupling/SOLID), Scalability (bottlenecks), and Security.
3. Code Smell & Technical Debt: Identification of "God objects" or overly complex logic.
4. Strategic Recommendations: The first three refactoring priorities for long-term health.

CONSTRAINT: Be direct, critical, and objective. Prioritize structural flaws over minor style nitpicks.
EOF

# 3. Process Includes and Excludes
# Save the internal field separator and set it to a comma
SAVE_IFS="$IFS"
IFS=,

# Initialize the repomix arguments array with the default flags
REPOMIX_ARGS=(
    "--output=$OUTPUT_FILE"
    "--no-security-check"
    "--style=markdown"
    "--remove-comments"
    "--remove-empty-lines"
    "--truncate-base64"
)

# If the include list has items, join them with commas and add to args
if [ ${#INCLUDE_LIST[@]} -gt 0 ]; then
    INCLUDE_STR="${INCLUDE_LIST[*]}"
    REPOMIX_ARGS+=("--include=$INCLUDE_STR")
fi

# If the exclude list has items, join them with commas and add to args (--ignore)
if [ ${#EXCLUDE_LIST[@]} -gt 0 ]; then
    EXCLUDE_STR="${EXCLUDE_LIST[*]}"
    REPOMIX_ARGS+=("--ignore=$EXCLUDE_STR")
fi

# Restore the original IFS
IFS="$SAVE_IFS"

# 4. Run Repomix
repomix "${REPOMIX_ARGS[@]}"

echo "-------------------------------------------------------"
echo "DONE!"
echo "Code Packaged: $OUTPUT_FILE"
echo "Prompt Ready:  $PROMPT_FILE"
echo "-------------------------------------------------------"
echo "Step 1: Copy the text inside $PROMPT_FILE"
echo "Step 2: Upload $OUTPUT_FILE to your AI"
echo "Step 3: Paste the prompt and hit enter."
