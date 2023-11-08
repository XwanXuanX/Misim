#!/bin/bash

# How to use?
# 1. Put the file (or a copy of the file) under the dir you want to monitor.
# 2. Invoke the script by "./auto_compile.sh <path to the executable>"
# 3. Please make sure the Makefile is visible to the script

# Get the path of the output exe
OUTPUT_EXE=${1}

# Get script running dir
cd "$(dirname "$0")"
SCRIPT_DIR="$(pwd)"
echo "Script running in: $SCRIPT_DIR"

# Check if a Makefile is visible
if ! [ -f $SCRIPT_DIR/Makefile ]; then
  echo "No Makefile exist. Exiting..."
  exit 1
fi

# Check if inotifywait is installed
if [ -z "$(which inotifywait)" ]; then
  echo "inotifywait not installed."
  echo "In most distros, it is available in the inotify-tools package."
  exit 1
fi

# Clean exit when SIGINT or SIGTERM
trap "echo Exited!; exit;" SIGINT SIGTERM

# Setting up monitor
inotifywait --recursive --monitor --exclude \.${OUTPUT_EXE} --format "%e %w%f" \
--event modify,move,create,delete ./ \
| while read changed; do
  echo "$changed ------------------------------------------"

  #  Clear previous make silently
  set echo off
  make clear
  set echo on

  # Rebuild
  make

done
