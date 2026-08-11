#!/usr/bin/env python3
import argparse
import time
import subprocess
import sys
import os
from pathlib import Path
from datetime import datetime, timedelta
import shutil

def count_files_in_dir(directory: Path) -> int:
    """Returns the number of files in the given directory."""
    return len([f for f in directory.iterdir() if f.is_file()])

def get_newest_file(directory: Path):
    """Finds the most recently modified file in a directory."""
    files = [f for f in directory.iterdir() if f.is_file()]
    if not files:
        return None
    return max(files, key=lambda f: f.stat().st_mtime)

def main():
    parser = argparse.ArgumentParser(description="Drive the radar data acquisition process.")
    parser.add_argument("-o", "--output_dir", required=True, help="Location of the output data")
    parser.add_argument("-r", "--radar", required=True, help="Name of radar (ex. KTLX)")
    parser.add_argument("-b", "--begin", required=True, help="Begin time as YYYYMM (ex. 201810)")
    parser.add_argument("-e", "--end", required=True, help="End time as YYYYMM (ex. 201812)")
    parser.add_argument("-n", "--dryrun", action="store_true", help="Dry run flag, no execution")
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")
    
    args = parser.parse_args()

    out_dir = Path(args.output_dir).resolve()
    
    if not out_dir.exists():
        print(f"Error: No Output Directory found at {out_dir}")
        sys.exit(-1)

    if args.verbose:
        print(f"Radar name is: {args.radar}")
        print(f"Output dir: {out_dir}")

    # Parse dates
    try:
        start_date = datetime.strptime(args.begin, "%Y%m")
        end_date = datetime.strptime(args.end, "%Y%m")
    except ValueError as e:
        print(f"Error parsing dates: {start_date}. Please ensure format is YYYYMM.")
        sys.exit(-1)

    # Note: getRadarData.py should be in the same directory, or provide the full path here
    # In the Perl script, it was hardcoded to /home/John.Krause/tools_jkrause/getRadarData.py
    # We will assume it's in the same directory as this script for better portability.

    current_date = start_date
    
    # Loop day by day
    while current_date < end_date:
        date_str = current_date.strftime("%Y%m%d")
        
        if args.verbose:
            print(f"Date: {date_str}")
            print(f"Start: 00 end: 24")

        # Command to fetch radar data for the 24-hour block
        cmd = [
            "getRadarData.py",
            "-o", str(out_dir),
            "-r", args.radar,
            "-d", date_str,
            "-s", "00",
            "-e", "24"
        ]

        if args.dryrun:
            print("Dry run:")
            print(" ".join(cmd))
        else:
            print(" ".join(cmd))
            subprocess.run(cmd)
            time.sleep(60)

        # Wait until files have been processed (count drops <= 5)
        num_files = 10
        while num_files > 5:
            if args.dryrun:
                print("Dry run: checking file counts")
                num_files = 1
            else:
                num_files = count_files_in_dir(out_dir)
                print(f"num_files: {num_files}")
                if num_files > 5:
                    time.sleep(30)
        
        # Advance by one day
        current_date += timedelta(days=1)

    # Post-processing steps
    trigger_file = out_dir / "Reflectivity_XXXX.txt"
    if args.dryrun:
        print(f"Dry run: touch {trigger_file}")
    else:
        trigger_file.touch(exist_ok=True)

    time.sleep(5)

    # Copy the newest file to the complete directory
    ar_dir = (out_dir.parent / "output").resolve()
    complete_dir = (out_dir.parent.parent / "complete").resolve()
    
    if args.verbose:
        print(f"Searching for newest file in: {ar_dir}")
        
    if not args.dryrun:
        if ar_dir.exists():
            newest_file = get_newest_file(ar_dir)
            if newest_file:
                print(f"Newest file: {newest_file}")
                complete_dir.mkdir(parents=True, exist_ok=True)
                dest = complete_dir / newest_file.name
                shutil.copy2(newest_file, dest)
                print(f"Copied to {dest}")
            else:
                print("No files found to copy.")
        else:
            print(f"Warning: Directory {ar_dir} does not exist.")
    else:
        print(f"Dry run: copy newest file from {ar_dir} to {complete_dir}")


if __name__ == "__main__":
    main()
