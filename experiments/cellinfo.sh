#!/bin/bash

# Define the CSV file path
output_file="ci_logs.csv"

# Write the CSV header (run only once)
echo "Timestamp,mCellConnectionStatus,mBands,ssRsrp,ssRsrq,ssSinr,level" > $output_file

while true
do
    # Record the start time of this iteration
    start_time=$(date +%s)

    # Get the current timestamp
    timestamp=$(date +"%Y-%m-%d %H:%M:%S")

    # Run the adb command and get the full mCellInfo output
    cell_info=$(adb -H 10.11.32.205 -s A75259FRCN9A2DV00E0 shell dumpsys telephony.registry)

    # Extract mCellConnectionStatus, mBands, ssRsrp, ssRsrq, ssSinr, and level values from the output
    mCellConnectionStatus=$(echo "$cell_info" | grep -m 1 -o "mCellConnectionStatus=[0-9]*" | cut -d'=' -f2 | tr -d ' ')
    mBands=$(echo "$cell_info" | grep -m 1 -o "mBands = \[[0-9]*\]" | grep -o "[0-9]*" | head -n 1)
    ssRsrp=$(echo "$cell_info" | grep -m 1 -o "ssRsrp = [-0-9]*" | cut -d'=' -f2 | tr -d ' ')
    ssRsrq=$(echo "$cell_info" | grep -m 1 -o "ssRsrq = [-0-9]*" | cut -d'=' -f2 | tr -d ' ')
    ssSinr=$(echo "$cell_info" | grep -m 1 -o "ssSinr = [-0-9]*" | cut -d'=' -f2 | tr -d ' ')
    level=$(echo "$cell_info" | grep -m 1 -o "level = [0-9]*" | cut -d'=' -f2 | tr -d ' ')

    # Prepare the data for CSV format
    csv_line="$timestamp,$mCellConnectionStatus,$mBands,$ssRsrp,$ssRsrq,$ssSinr,$level"

    # Display the CSV line in the terminal and append it to the file
    echo "$csv_line" | tee -a $output_file

    # Calculate the time taken to complete the operations
    end_time=$(date +%s)
    elapsed_time=$((end_time - start_time))

    # Sleep for the remaining time to ensure the loop iterates every second
    sleep_time=$((1 - elapsed_time))
    if [ $sleep_time -gt 0 ]; then
        sleep $sleep_time
    fi
done

