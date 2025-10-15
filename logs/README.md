# Logs

## get_data.py

### Usage

Change the line:

```python
logs_directory = os.path.join(script_dir, "logs_DD_MM")
```

to the desired timestamp.

Run:

```bash
python get_data.py
```

The script will:

- Look for JSON files in the `logs_DD_MM` directory (relative to the script location)
- Process each file and categorize it by scenario
- Create a new directory structure in `logs_DD_MM_extracted_data` with scenario-specific folders
- Generate CSV files for each data type within the scenario folders

### Supported File Types

The script extracts data from three types of JSON files:

1. **Cell Info**: Signal strength and quality metrics
2. **iPerf3**: Network throughput test results
3. **Ping**: Network latency test results

### Scenario Classification

Files are automatically classified into the following scenarios based on their filenames:

| File Pattern | Scenario |
| ------------ | -------- |
| `-udp-bidir-` | `udp-bidir` (UDP bidirectional) |
| `-bidir-` | `tcp-bidir` (TCP bidirectional) |
| `-udp-reverse-` | `udp-downlink` (UDP downlink) |
| `-reverse-` | `tcp-downlink` (TCP downlink) |
| `-udp-` | `udp-uplink` (UDP uplink) |
| No pattern match | `tcp-uplink` (TCP uplink, default) |

### Output Structure

```text
logs_DD_MM_extracted_data/
├── tcp-uplink/
│   ├── cell_info.csv
│   ├── iperf3.csv
│   └── ping.csv
├── tcp-downlink/
│   ├── cell_info.csv
│   ├── iperf3.csv
│   └── ping.csv
└── [other scenarios]...
```

### Output CSV Files

#### cell_info.csv

Contains signal quality metrics:

- Device information
- RSRP (Reference Signal Received Power)
- RSRQ (Reference Signal Received Quality)
- SINR (Signal-to-Interference-plus-Noise Ratio)
- Statistics (mean, standard deviation)

#### iperf3.csv

Contains throughput test results:

- Connection details (local/remote hosts)
- Protocol information
- Stream ID
- Bandwidth (bits per second)
- Time intervals

#### ping.csv

Contains network latency measurements:

- Destination IP
- ICMP sequence numbers
- Response times (in milliseconds)

## dataset_build_v2.py

### Usage

Change the line:

```python
data_directory = os.path.join(script_dir, "logs_DD_MM")
```

to the desired timestamp.

Run:

```bash
python dataset_build_v2.py --scenario SCENARIO [OPTIONS]
```

#### Required Arguments

- `--scenario`: The network scenario to process. Must be one of:
  - `tcp-uplink`: TCP uplink traffic
  - `tcp-downlink`: TCP downlink traffic
  - `tcp-bidir`: TCP bidirectional traffic
  - `udp-uplink`: UDP uplink traffic
  - `udp-downlink`: UDP downlink traffic
  - `udp-bidir`: UDP bidirectional traffic

#### Optional Arguments

- `--randomize_positions`: Randomize node positions (default is fixed positions at (0,0,0) and (0,30,0))
- `--generate_trace_csv`: Generate an additional trace-based CSV file
- `--stream_id`: Stream ID to use for bidirectional scenarios (required when using bidirectional scenarios)
  - Use `5` for uplink streams
  - Use `7` for downlink streams

#### Examples

1. Generate dataset for UDP uplink traffic:

   ```shell
   python dataset_build_v2.py --scenario udp-uplink
   ```

2. Generate dataset for TCP downlink with trace-based CSV:

   ```shell
   python dataset_build_v2.py --scenario tcp-downlink --generate_trace_csv
   ```

3. Generate dataset for TCP bidirectional traffic, focusing on the uplink stream:

   ```shell
   python dataset_build_v2.py --scenario tcp-bidir --stream_id 5
   ```

4. Generate dataset for UDP uplink traffic with randomized positions:

   ```shell
   python dataset_build_v2.py --scenario udp-downlink --randomize_positions
   ```

### Output Files

The script generates one or two CSV files in the same directory as the input files:

1. `propagation-loss-dataset.csv`: Position-based dataset containing node coordinates, loss in dB, and throughput
2. `trace-based-[scenario].csv`: Time-based dataset containing time, node IDs, received power in dBm, and throughput (only generated if `--generate_trace_csv` flag is used)

### Data Structure

#### Position-based Dataset

- `x_tx`, `y_tx`, `z_tx`: Transmitter node coordinates
- `x_rx`, `y_rx`, `z_rx`: Receiver node coordinates
- `loss_db`: Signal loss in dB
- `throughput_kbps`: Throughput in kbps

#### Trace-based Dataset

- `time_s`: Time in seconds
- `tx_node`: Transmitter node ID (0 or 1)
- `rx_node`: Receiver node ID (0 or 1)
- `rx_power_dbm`: Received power in dBm
- `throughput_kbps`: Throughput in kbps

### Directory Structure

The script expects to find input CSV files in:
`./logs_DD_MM_extracted_data/[scenario]/`

Input files needed:

- `iperf3.csv`: Contains throughput data
- `cell_info.csv`: Contains signal quality data
