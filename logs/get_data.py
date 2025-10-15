import os
import json
import csv
import glob
import re
from collections import defaultdict

# Define mapping of file patterns to scenarios
SCENARIO_MAP = {
    r"-udp-bidir-": "udp-bidir",
    r"-bidir-": "tcp-bidir",
    r"-udp-reverse-": "udp-downlink",
    r"-reverse-": "tcp-downlink",
    r"-udp-": "udp-uplink",
}

def determine_scenario(filename):
    for pattern, scenario in SCENARIO_MAP.items():
        if pattern in filename:
            return scenario
    return "tcp-uplink"  # Default to TCP if no scenario is specified

def extract_cell_info(file_path):
    with open(file_path, 'r') as file:
        data = json.load(file)
    
    general_info = data.get("general_info", {})
    samples = data.get("samples", [])
    statistics = data.get("statistics", {})
    
    extracted_data = []
    for sample in samples:
        row = {
            "file_name": os.path.basename(file_path),
            "device_serial": general_info.get("device_serial", ""),
            "host_ip": general_info.get("host_ip", ""),
            "iteration": sample.get("iteration", ""),
            "mBands": sample.get("mBands", ""),
            "ssRsrp": sample.get("ssRsrp", ""),
            "ssRsrq": sample.get("ssRsrq", ""),
            "ssSinr": sample.get("ssSinr", ""),
            "level": sample.get("level", ""),
            "mean_ssRsrp": statistics.get("mean", {}).get("ssRsrp", ""),
            "mean_ssRsrq": statistics.get("mean", {}).get("ssRsrq", ""),
            "mean_ssSinr": statistics.get("mean", {}).get("ssSinr", ""),
            "std_dev_ssRsrp": statistics.get("std_dev", {}).get("ssRsrp", ""),
            "std_dev_ssSinr": statistics.get("std_dev", {}).get("ssSinr", ""),
        }
        extracted_data.append(row)
    return extracted_data

def extract_iperf3(file_path):
    with open(file_path, "r") as file:
        data = json.load(file)
    
    start_info = data.get("start", {})
    intervals = data.get("intervals", [])
    extracted_data = []
    
    for interval in intervals:
        streams = interval.get("streams", [])
        sum_data = interval.get("sum", {})
        
        for stream in streams:
            row = {
                "file_name": os.path.basename(file_path),
                "local_host": start_info.get("connected", [{}])[0].get("local_host", ""),
                "remote_host": start_info.get("connected", [{}])[0].get("remote_host", ""),
                "protocol": start_info.get("test_start", {}).get("protocol", ""),
                "stream_id": stream.get("socket", ""),  # Unique identifier for each stream
                "bits_per_second": stream.get("bits_per_second", ""),
                "interval_start": stream.get("start", ""),
                "interval_end": stream.get("end", ""),
            }
            extracted_data.append(row)
    
    return extracted_data

def extract_ping(file_path):
    with open(file_path, "r") as file:
        data = json.load(file)
    extracted_data = []
    for response in data.get("responses", []):
        row = {
            "file_name": os.path.basename(file_path),
            "destination_ip": data.get("destination_ip", ""),
            "icmp_seq": response.get("icmp_seq", ""),
            "time_ms": response.get("time_ms", ""),
        }
        extracted_data.append(row)
    return extracted_data

def write_to_csv(data, output_file):
    if not data:
        return
    fieldnames = list(data[0].keys())
    non_empty_fieldnames = [field for field in fieldnames if any(row.get(field) for row in data)]
    with open(output_file, "w", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=non_empty_fieldnames)
        writer.writeheader()
        for row in data:
            writer.writerow({field: row.get(field, "") for field in non_empty_fieldnames})

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    logs_directory = os.path.join(script_dir, "logs_22_02")  # Change this to your target directory
    json_files = glob.glob(os.path.join(logs_directory, "*.json"))
    
    extracted_data_folder = logs_directory + "_extracted_data"
    os.makedirs(extracted_data_folder, exist_ok=True)
    
    categorized_data = defaultdict(lambda: defaultdict(list))
    
    for file_path in json_files:
        scenario = determine_scenario(os.path.basename(file_path))
        
        try:
            if "cell_info" in file_path:
                categorized_data[scenario]["cell_info"].extend(extract_cell_info(file_path))
            elif "iperf3" in file_path:
                categorized_data[scenario]["iperf3"].extend(extract_iperf3(file_path))
            elif "ping" in file_path:
                categorized_data[scenario]["ping"].extend(extract_ping(file_path))
        except Exception as e:
            print(f"Error processing {file_path}: {e}")
            continue
    
    for scenario, file_types in categorized_data.items():
        folder_name = os.path.join(extracted_data_folder, scenario)
        os.makedirs(folder_name, exist_ok=True)
        
        for file_type, data in file_types.items():
            output_file = os.path.join(folder_name, f"{file_type}.csv")
            write_to_csv(data, output_file)

if __name__ == "__main__":
    main()
