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
# This will install repomix say on rocky8/9 oracle 8/9
# Set this to true to run the installation, or false to skip it
# Personally if you have another distro or non-root just ask AI
# what steps to do.  This works on a fresh container as root.
INSTALLREPOMIX=false

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

# 3. Run Repomix.  I don't use security check since we shouldn't be putting
# passwords in our code anyway.
repomix --include "**/*.{cc,h,txt}" --output="$OUTPUT_FILE" --no-security-check

echo "-------------------------------------------------------"
echo "DONE!"
echo "Code Packaged: $OUTPUT_FILE"
echo "Prompt Ready:  $PROMPT_FILE"
echo "-------------------------------------------------------"
echo "Step 1: Copy the text inside $PROMPT_FILE"
echo "Step 2: Upload $OUTPUT_FILE to your AI"
echo "Step 3: Paste the prompt and hit enter."
