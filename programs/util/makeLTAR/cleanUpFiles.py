#!/usr/bin/python3
#
#
#01/16/2018
#   -John Krause, John.Krause@noaa.gov.
#
#Totally unsupported script. Use at own risk.
#


import os
import argparse
import time
import sys

# Initialize the argument parser
parser = argparse.ArgumentParser(description="Cleanup Directory every x minutes for files x minutes old")

# Add a positional argument (e.g., a required input file)
parser.add_argument("-d", "--dir", type=str, help="dir location where the files to remove are located")
parser.add_argument("-m", "--minutes", type=int, help="number of minutes before the file is 'old' and should be removed")
parser.add_argument("-s", "--size", type=int, help="Size of file in MB to remove")
# Add optional flags
parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose mode")
#parser.add_argument("-h", "--help", action="store_true", help="print this output")
# Parse the command-line arguments
args = parser.parse_args()

if not args.dir or not args.minutes:
    print("script requires -d directory and -m minutes to function");
    sys.exit()

print("Command used:")
print(' '.join(sys.argv))

# Define the directory containing the files
directory_path = args.dir
if not directory_path.endswith("/"):
    directory_path += "/"

minutes = args.minutes
if args.size:
    size_limit = args.size * 1024 * 1024  # 5 MB in bytes

workdir = directory_path 
snooze_time = minutes*60 

while True:
    print("sleeping: %d " % snooze_time)
    time.sleep(snooze_time)

    now = time.time()
    #5 minutes
    old = now - minutes * 60

    for dirpath, dirnames, filenames in os.walk(workdir):
        for f in filenames:
            path = os.path.join(dirpath, f)
            if os.path.isfile(path):
                stat = os.stat(path)
                if args.size:
                    if os.path.getsize(path) > size_limit:
                        print(f"Deleting {path} ({os.path.getsize(path)} bytes)")
                        os.remove(path)
                else:
                    if stat.st_ctime < old:
                         if args.verbose:
                             print ( "removing: %s" % path )
                         os.remove(path) # uncomment

