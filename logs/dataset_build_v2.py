import os
import pandas as pd
import random
import numpy as np  # For log and power conversions
import argparse

# Set up the argument parser
parser = argparse.ArgumentParser(description="Process a scenario and generate propagation loss dataset.")

# Add the 'scenario' argument with default value and list of accepted scenarios
parser.add_argument(
    "--scenario",
    type=str,
    choices=["tcp-uplink", "tcp-downlink", "tcp-bidir", "udp-uplink", "udp-downlink", "udp-bidir"],
    required=True,
    help="The scenario for the dataset. Accepted values are: TCP, TCP_Reverse, TCP_Bidirectional, UDP, UDP_Reverse, UDP_Bidirectional.",
)

# Add argument for randomizing positions
parser.add_argument(
    "--randomize_positions",
    action="store_true",
    help="Set this flag to randomize positions (default is False, positions are set to (0,0,0) (1,1,1)).",
)

# Add argument for selecting a specific stream in bidirectional scenarios
parser.add_argument(
    "--stream_id",
    type=int,
    help="Specify the stream ID to use for throughput in bidirectional scenarios. Required for TCP_Bidirectional and UDP_Bidirectional.",
)

# Add new flag to generate the trace-based CSV file
parser.add_argument(
    "--generate_trace_csv",
    action="store_true",
    help="Generate an additional CSV file for trace-based model (time_s, tx_node, rx_node, rx_power_dbm, throughput_kbps).",
)

# Parse the arguments
args = parser.parse_args()

# Use the scenario passed as a command line argument
scenario = args.scenario

# Whether to randomize positions
randomize_positions = args.randomize_positions

# Whether to generate trace-based CSV
generate_trace_csv = args.generate_trace_csv

# Ensure stream_id is specified for bidirectional scenarios
if "-bidir" in scenario and args.stream_id is None:
    raise ValueError("For bidirectional scenarios, you must specify --stream_id.")

script_dir = os.path.dirname(os.path.abspath(__file__))
data_directory = os.path.join(script_dir, "logs_22_02")  # Change this to your target directory

iperf3_file = os.path.join(data_directory + f"_extracted_data/{scenario}/iperf3.csv")
cellinfo_file = os.path.join(data_directory + f"_extracted_data/{scenario}/cell_info.csv")

# Load the CSVs using pandas
iperf3_df = pd.read_csv(iperf3_file)
cellinfo_df = pd.read_csv(cellinfo_file)

# Ensure required columns exist
required_columns_iperf3 = {"stream_id", "bits_per_second"}
required_columns_cellinfo = {"ssSinr", "ssRsrp", "ssRsrq"}

if not required_columns_iperf3.issubset(iperf3_df.columns):
    raise ValueError(f"Missing columns in iperf3 CSV: {required_columns_iperf3 - set(iperf3_df.columns)}")
if not required_columns_cellinfo.issubset(cellinfo_df.columns):
    raise ValueError(f"Missing columns in cell info CSV: {required_columns_cellinfo - set(cellinfo_df.columns)}")

# If it's a bidirectional scenario, filter for the selected stream_id
if "-bidir" in scenario:
    iperf3_df = iperf3_df[iperf3_df["stream_id"] == args.stream_id]

    if iperf3_df.empty:
        raise ValueError(f"No data found for stream_id {args.stream_id} in the iperf3 file.")
    
    # Reset the index after filtering to ensure consecutive time values
    iperf3_df = iperf3_df.reset_index(drop=True)

# Given number of resource blocks
N = 273
Tx_Power_db = 40  # Transmit power in dB

# Convert RSRP, SINR, and RSRQ from dB to linear
cellinfo_df["RSRP_linear"] = 10 ** (cellinfo_df["ssRsrp"] / 10)
cellinfo_df["SINR_linear"] = 10 ** (cellinfo_df["ssSinr"] / 10)
cellinfo_df["RSRQ_linear"] = 10 ** (cellinfo_df["ssRsrq"] / 10)

# Compute RSSI (linear)
cellinfo_df["RSSI_linear"] = (N * cellinfo_df["RSRP_linear"]) / cellinfo_df["RSRQ_linear"]

# Compute Noise Power (linear)
cellinfo_df["Noise_linear"] = cellinfo_df["RSSI_linear"] / cellinfo_df["SINR_linear"]

# Convert Noise Power back to dBm (avoid log of non-positive values)
cellinfo_df["noise_dbm"] = np.where(
    cellinfo_df["Noise_linear"] > 0, 10 * np.log10(cellinfo_df["Noise_linear"]), np.nan
)

# Convert RSSI from linear back to dBm
cellinfo_df["rssi_dbm"] = np.where(
    cellinfo_df["RSSI_linear"] > 0, 10 * np.log10(cellinfo_df["RSSI_linear"]), np.nan
)

# Compute Rx Power (dB)
cellinfo_df["Rx_power_db"] = cellinfo_df["ssSinr"] + cellinfo_df["noise_dbm"]

# Compute Loss (dB)
cellinfo_df["loss_db"] = Tx_Power_db - cellinfo_df["Rx_power_db"]

# Initialize lists to collect rows for both output files
output_rows = []
trace_based_rows = []

# Define tx_node and rx_node based on scenario
if scenario in ["udp-downlink", "tcp-downlink"]:
    tx_node, rx_node = 0, 1
elif scenario in ["udp-uplink", "tcp-uplink"]:
    tx_node, rx_node = 1, 0
elif "-bidir" in scenario:
    if args.stream_id == 5:
        tx_node, rx_node = 0, 1
    elif args.stream_id == 7:
        tx_node, rx_node = 1, 0
    else:
        raise ValueError("Invalid stream_id for bidirectional scenario. Use 5 or 7.")

# Limit Max Rows on Trace-based dataset, to control simulation duration
max_rows_trace=540

# Iterate through iperf3 rows and process data
for index, iperf3_row in iperf3_df.iterrows():


    throughput_kbps = iperf3_row["bits_per_second"] / 1000  # Convert bps to kbps

    if randomize_positions:
        # Randomize positions (between 0 and 1)
        x_tx, y_tx, z_tx = random.uniform(0, 1), random.uniform(0, 1), random.uniform(0, 1)
        x_rx, y_rx, z_rx = random.uniform(0, 1) + 1, random.uniform(0, 1) + 1, random.uniform(0, 1) + 1
    else:
        # Set all positions to (0,0,0)
        x_tx, y_tx, z_tx = 0, 0, 0
        x_rx, y_rx, z_rx = 0, 30, 0

    # Match cell info row based on corresponding row in cell_info
    # If bidirectional, we need to be careful about the cellinfo index
    if index < len(cellinfo_df):
        cellinfo_row = cellinfo_df.iloc[index]
        loss_db = cellinfo_df.iloc[index]["loss_db"]
        rssi_dbm = cellinfo_row["rssi_dbm"]  # Get RSSI value
    else:
        snr_db, noise_dbm, rssi_dbm = None, None, None  # Default if no matching row

    # Create standard row for the position-based output
    standard_row = {
        "x_tx": round(x_tx, 2),
        "y_tx": round(y_tx, 2),
        "z_tx": round(z_tx, 2),
        "x_rx": round(x_rx, 2),
        "y_rx": round(y_rx, 2),
        "z_rx": round(z_rx, 2),
        "loss_db": round(loss_db, 2) if loss_db is not None else None,
        "throughput_kbps": round(throughput_kbps, 2),
    }
    
    # Only add row if it contains non-None values
    if any(v is not None for v in standard_row.values()):
        output_rows.append(standard_row)
    
    # Create trace-based row if the flag is set
    if generate_trace_csv and index < max_rows_trace:
        trace_row = {
            "time_s": index+1, 
            "tx_node": tx_node,
            "rx_node": rx_node,
            "rx_power_dbm": round(rssi_dbm, 2) if rssi_dbm is not None else None,  # RSSI as rx_power
            "throughput_kbps": round(throughput_kbps, 2),
        }
        
        # Only add trace-based row if it contains non-None values
        if any(v is not None for v in trace_row.values()):
            trace_based_rows.append(trace_row)

swapped_rows = [{"x_tx": row["x_rx"], "y_tx": row["y_rx"], "z_tx": row["z_rx"], "x_rx": row["x_tx"], "y_rx": row["y_tx"], "z_rx": row["z_tx"], "loss_db": row["loss_db"], "throughput_kbps": row["throughput_kbps"]} for row in output_rows]


swapped_rows_trace = [{"time_s": row["time_s"], "tx_node": row["rx_node"], "rx_node": row["tx_node"], "rx_power_dbm": row["rx_power_dbm"], "throughput_kbps": row["throughput_kbps"]} for row in trace_based_rows]

# Create standard DataFrame
output_df = pd.DataFrame(output_rows + swapped_rows) if output_rows else pd.DataFrame()

# Prepare the output file path for standard output
output_dir = os.path.dirname(iperf3_file)  # Store in the same directory
output_file = os.path.join(output_dir, "propagation-loss-dataset.csv")

# Save the standard output DataFrame to CSV
output_df.to_csv(output_file, index=False)
print(f"Processed position-based CSV saved at: {output_file}")

# Create and save trace-based DataFrame if flag is set
if generate_trace_csv and trace_based_rows:
    trace_df = pd.DataFrame(trace_based_rows + swapped_rows_trace)
    trace_output_file = os.path.join(output_dir, f"trace-based-{scenario}.csv")    
    trace_df.to_csv(trace_output_file, index=False)
    print(f"Processed trace-based CSV saved at: {trace_output_file}")