#!/usr/bin/env python3
import os
import time
import argparse
import subprocess
import sys
from pathlib import Path

def create_directory(path):
    """Creates a directory if it doesn't exist."""
    path.mkdir(parents=True, exist_ok=True)
    print(f"Ensured directory exists: {path}")

def run_background_process(command, log_file):
    """Runs a command in the background and redirects output to a log file."""
    print(f"Launching: {' '.join(command)}")
    with open(log_file, "w") as log:
        # Use Popen to run the process in the background, similar to '&' in bash
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
    return process

def main():
    parser = argparse.ArgumentParser(description="Unified makeLTAR Processing Driver")
    parser.add_argument("-r", "--radar", required=True, help="4-character radar name (e.g., KTLX)")
    parser.add_argument("-b", "--begin", required=True, help="Begin time as YYYYMM (e.g., 201810)")
    parser.add_argument("-e", "--end", required=True, help="End time as YYYYMM (e.g., 201812)")
    parser.add_argument("-i", "--input_dir", required=True, help="Base working directory")
    parser.add_argument("-n", "--dry_run", action="store_true", help="output commands, don't run")
    
    args = parser.parse_args()

    # 1. Setup Directories
    base_dir = Path(args.input_dir)
    netcdf_dir = base_dir / "netcdf"
    raw_dir = base_dir / "raw"
    logs_dir = base_dir / "logs"
    out_dir = base_dir / "output"

    #fam location
    fam_dir = netcdf_dir / "code_index.fam"

    for d in [netcdf_dir, raw_dir, logs_dir, out_dir]:
        create_directory(d)

    time.sleep(2) # Give the file system a moment to catch up

    active_processes = []

    try:
        # 2. Start Data Conversion (ldm2netcdf)
        ldm_cmd = [
            "ldm2netcdf",
            "-K", "-r", "-a", "-p", "K",
            "-s", args.radar,
            "-o", str(netcdf_dir),
            "-i", str(raw_dir)
        ]

        if args.dry_run:
            print(f"cmd {" ".join(ldm_cmd)}")
        else:
            active_processes.append(run_background_process(ldm_cmd, logs_dir / "ldm2netcdf.log"))

        # 3. Start C++ Processing (RefGC)
        refgc_cmd = [
            f"makeLTAR",
            "-r",
            "-i", str(fam_dir),
            "-o", str(out_dir),
            "-R", args.radar
        ]
        if args.dry_run:
            print(f"cmd {" ".join(refgc_cmd)}")
        else:
            active_processes.append(run_background_process(refgc_cmd, logs_dir / "makeLTAR.log"))
        
        time.sleep(5) # Wait for listeners to spin up

        # 4. Start Data Acquisition
        # Note: Assuming drive_getRadarData is also converted to Python eventually, but calling it as is for now.
        data_cmd = [
            f"drive_getRadarData.py",
            "-r", args.radar,
            "-o", str(raw_dir),
            "-b", args.begin,
            "-e", args.end
        ]

        if args.dry_run:
            print(f"cmd {" ".join(data_cmd)}")
        else:
            active_processes.append(run_background_process(data_cmd, logs_dir / f"getRadarData.log"))

        # 5. Start Cleanup Processes
        cleanup_script = f"cleanUpFiles.py"
        
        cleanup_netcdf = [cleanup_script, "-v", "-d", str(netcdf_dir), "-m", "40"]
        if args.dry_run:
            print(f"cmd {" ".join(cleanup_netcdf)}")
        else:
            active_processes.append(run_background_process(cleanup_netcdf, logs_dir / "cleanup_netcdf.log"))

        cleanup_raw = [cleanup_script, "-v", "-d", str(raw_dir), "-m", "40"]
        if args.dry_run:
            print(f"cmd {" ".join(cleanup_raw)}")
        else:
            active_processes.append(run_background_process(cleanup_raw, logs_dir / "cleanup_raw.log"))

        
        cleanup_logs = [cleanup_script, "-v", "-s", "10", "-d", str(logs_dir)]
        if args.dry_run:
            print(f"cmd {" ".join(cleanup_logs)}")
        else:
            active_processes.append(run_background_process(cleanup_logs, logs_dir / "cleanup_logs.log"))

        print("\nAll processes launched successfully. Processing data in the background.")
        print("Press Ctrl+C at any time to safely shut down all tasks.")

        # Keep the main script alive to manage subprocesses
        empty_dir_seconds = 0
        check_interval = 10  # Check the directory every 10 seconds

        while True:
            if not args.dry_run:
                # Count files in the raw directory
                raw_files = [f for f in raw_dir.iterdir() if f.is_file()]
                
                if len(raw_files) == 0:
                    empty_dir_seconds += check_interval
                    print(f"\nRaw data directory has been empty for {empty_dir_seconds/60} minutes. Waiting")
                else:
                    empty_dir_seconds = 0  # Reset timer if files are present
                
                # Exit Condition 1: 5 minutes (300 seconds) with no data
                if empty_dir_seconds >= 300:
                    print("\nRaw data directory has been empty for 5 minutes. Shutting down...")
                    break
                
                time.sleep(check_interval)
            else:
                # If it's a dry run, just break immediately
                break

    # Clean up all background processes after breaking out of the loop
        if not args.dry_run:
            print("Terminating background processes...")
            for p in active_processes:
                p.terminate()
                p.wait()
            print("All tasks completed. Exiting.")

    except KeyboardInterrupt:
        print("\nShutdown signal received! Terminating background processes safely...")
        for p in active_processes:
            p.terminate()
            p.wait() # Ensure they have fully closed
        print("All processes stopped. Exiting.")
        sys.exit(0)

if __name__ == "__main__":
    main()
