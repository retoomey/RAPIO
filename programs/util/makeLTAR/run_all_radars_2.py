#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description="Batch runner for WSR-88D radars")
    parser.add_argument("-l", "--list_file", required=True, help="Path to radar list (e.g., radar_list.txt)")
    parser.add_argument("-b", "--begin", required=True, help="Begin time as YYYYMM (e.g., 202603)")
    parser.add_argument("-e", "--end", required=True, help="End time as YYYYMM (e.g., 202604)")
    parser.add_argument("-i", "--input_dir", required=True, help="Base working directory")
    parser.add_argument("-n", "--dry_run", action="store_true", help="Print commands without running")
    
    args = parser.parse_args()

    list_path = Path(args.list_file).resolve()
    base_dir = Path(args.input_dir).resolve()
    
    if not list_path.exists():
        print(f"Error: List file {list_path} not found.")
        sys.exit(1)

    # Assumes makeLTAR_driver.py is in the same directory as this script
    driver_script = Path(__file__).parent / "makeLTAR_driver.py"

    with open(list_path, 'r') as textfile:
        for line in textfile:
            # Skip empty lines
            if not line.strip():
                continue
            
            # .split() automatically handles multiple spaces and tabs
            row = line.split()
            
            # Ensure the row has enough columns to check the radar type
            if len(row) < 6:
                continue
            
            radar_id = row[0]
            radar_type = row[5]
            
            # Filter specifically for WSR-88D radars
            if radar_type == "WSR-88D":
                
                # Create a unique directory for this radar to prevent collisions
                radar_working_dir = base_dir / f"{radar_id}_{args.begin}"
                
                cmd = [
                    sys.executable, str(driver_script),
                    "-r", radar_id,
                    "-b", args.begin,
                    "-e", args.end,
                    "-i", str(radar_working_dir)
                ]
                
                if args.dry_run:
                    print(" ".join(cmd))
                else:
                    print(f"\n=======================================================")
                    print(f" Launching processing for {radar_id}")
                    print(f"=======================================================")
                    
                    # Ensure the unique working directory exists before running
                    radar_working_dir.mkdir(parents=True, exist_ok=True)
                    
                    # Run the driver script and wait for it to finish before starting the next
                    subprocess.run(cmd)

if __name__ == "__main__":
    main()
