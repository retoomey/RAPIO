# 🛠️ Code Utilities (`codeutil`)

This folder serves as the centralized hub for developer tools and scripts that interact with the RAPIO source tree. 

By grouping these utilities in one place, we maintain a clean root directory while standardizing how we format code, generate documentation, perform security scans, and package context for AI analysis.

---

## 🏗️ Core Architecture: The Container Fallback Pattern
Almost all scripts in this directory share a robust execution pattern:
1. **Local Fast Path:** They first attempt to run a locally installed binary (e.g., `semgrep`, `uncrustify`, `doxygen`, `repomix`) for maximum performance.
2. **Container Fallback:** If the local tool is missing, the scripts automatically detect **Docker** or **Podman** and spin up a transient container to perform the task.
3. **Root Execution:** To avoid relative path quirks, the scripts dynamically resolve the `REPO_ROOT` (one level up) and execute their commands from the root directory.

---

## 🖌️ Code Formatting (`prettyprint.sh`)
Maintains consistent C++ code style across the repository using `uncrustify`.

**Key Files:**
* `prettyprint.sh`: The main execution script.
* `uncrustify.cfg`: The custom formatting rules (120-col limit, 4-space tabs, etc.).
* `uncrustify.inc.sh`: Helper file managing file-matching and container logic.
* `uncrustify.dock`: Dockerfile used to build the local uncrustify container on the fly if needed.

**Usage:**
```bash
# Check formatting without modifying files
./prettyprint.sh -c                 # Default directories
./prettyprint.sh --check base tests # Specific directories

# Apply formatting (OVERWRITES files)
./prettyprint.sh -w                 # Default directories
./prettyprint.sh --write base       # Specific directories
```

---

## 🛡️ Security Scanning (`semgrep.sh`)
Replicates the GitLab Static Application Security Testing (SAST) environment locally. It scans for vulnerabilities and parses the JSON output into human-readable summaries using `jq`.

**Usage:**
```bash
./semgrep.sh export   # Runs a scan and exports 3 text reports to a dated folder
./semgrep.sh summary  # Prints a brief tally of findings (e.g., "4 [HIGH] Buffer Overflow")
./semgrep.sh action   # Prints file paths and line numbers needing fixes
./semgrep.sh full     # Prints the deep-dive explanations for all issues
```

---

## 🤖 AI Context Packaging (`repo*.sh`)
These scripts use `repomix` to bundle specific sub-sections of the repository into optimized XML files for Large Language Models (LLMs). They also generate a ready-to-use prompt file.

**Available Bundles:**
* `repobase.sh`: Bundles the core API (`base/`, `rexample/`).
* `repofusion.sh`: Bundles Core + Fusion programs and volume plugins.
* `repomodules.sh`: Bundles Core + IO modules.

**Usage:**
```bash
# Generate the XML bundle and prompt file
./repobase.sh all

# Just update the prompt file
./repobase.sh prompt
```

---

## 📚 Documentation (`rundoxygen.sh`)
Generates HTML and LaTeX documentation from C++ source comments. 

**Key Files:**
* `rundoxygen.sh`: The execution script that pipes overrides to Doxygen.
* `rapio.dox`: The main Doxygen configuration file.

**Usage:**
```bash
# Generate documentation into a timestamped folder
./rundoxygen.sh
```
*Output will be saved to the repository root in a folder named `rapiodoxygen_YYYYMMDD_HHMMSS`.*

