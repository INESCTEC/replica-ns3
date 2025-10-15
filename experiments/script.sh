#!/bin/bash

# Define the remote server credentials
USER="handy"
HOST="10.11.32.205"
PASSWORD="hello123"
REMOTE_SCRIPT="./cellinfo.sh"

# Get the current date and time to append to the filename
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Define the output file name with date and time
LOCAL_OUTPUT="ci_logs_$TIMESTAMP.csv"

# Write the CSV header
echo "Timestamp,mCellConnectionStatus,mBands,ssRsrp,ssRsrq,ssSinr,level" > "$LOCAL_OUTPUT"

# Use sshpass to automate password entry and run the script
sshpass -p $PASSWORD ssh $USER@$HOST "$REMOTE_SCRIPT" | tee -a "$LOCAL_OUTPUT"

echo "Output is being saved to $LOCAL_OUTPUT"

