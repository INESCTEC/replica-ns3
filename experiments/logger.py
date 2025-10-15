#!/usr/bin/python3
import subprocess
import json
import time
import statistics
import re
import argparse
import sys
import glob
from concurrent.futures import ProcessPoolExecutor

def cell_info(host_ip, device_serial, c, X):
    """
    Collects cellular signal data (bands, signal strength and level) from an Android device.
    
    Parameters:
        host_ip (str): IP address of the host running ADB.
        device_serial (str): Serial number of the Android device.
        c (int): Number of times to collect samples.
        X (int): Interval (in seconds) between each sample collection.
    
    Returns:
        str: JSON-formatted string with raw samples and calculated statistics.
    """
     
    command = ['adb', '-H', host_ip, '-s', device_serial, 'shell', 'dumpsys', 'telephony.registry']

    mBands_samples = []
    ssRsrp_samples = []
    ssRsrq_samples = []
    ssSinr_samples = []
    level_samples = []

    for _ in range(c):
        try:
            result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            cell_info = result.stdout.decode('utf-8')
        except subprocess.CalledProcessError as e:
            print(f"Cell info command failed with return code {e.returncode}: {e.stderr.decode('utf-8')}")
            continue  

        mBands = int(re.search(r"mBands = \[(\d+)\]", cell_info).group(1))
        ssRsrp = int(re.search(r"ssRsrp = ([-\d]+)", cell_info).group(1))
        ssRsrq = int(re.search(r"ssRsrq = ([-\d]+)", cell_info).group(1))
        ssSinr = int(re.search(r"ssSinr = ([-\d]+)", cell_info).group(1))
        level = int(re.search(r"level = (\d+)", cell_info).group(1))
        
        mBands_samples.append(mBands)
        ssRsrp_samples.append(ssRsrp)
        ssRsrq_samples.append(ssRsrq)
        ssSinr_samples.append(ssSinr)
        level_samples.append(level)

        time.sleep(X)

    stats = {
        "mean": {
            "mBands": statistics.mean(mBands_samples),
            "ssRsrp": statistics.mean(ssRsrp_samples),
            "ssRsrq": statistics.mean(ssRsrq_samples),
            "ssSinr": statistics.mean(ssSinr_samples),
            "level": statistics.mean(level_samples),
        },
        "std_dev": {
            "mBands": statistics.stdev(mBands_samples) if len(mBands_samples) > 1 else 0,
            "ssRsrp": statistics.stdev(ssRsrp_samples) if len(ssRsrp_samples) > 1 else 0,
            "ssRsrq": statistics.stdev(ssRsrq_samples) if len(ssRsrq_samples) > 1 else 0,
            "ssSinr": statistics.stdev(ssSinr_samples) if len(ssSinr_samples) > 1 else 0,
            "level": statistics.stdev(level_samples) if len(level_samples) > 1 else 0,
        }
    }

    output = {
        "general_info": {
            "host_ip": host_ip,
            "device_serial": device_serial,
        },
        "samples": [
            {
                "iteration": i + 1,
                "mBands": mBands_samples[i],
                "ssRsrp": ssRsrp_samples[i],
                "ssRsrq": ssRsrq_samples[i],
                "ssSinr": ssSinr_samples[i],
                "level": level_samples[i]
            }
            for i in range(len(mBands_samples))
        ],
        "statistics": stats
    }
    
    return json.dumps(output, indent=4)

def iperf3(device_serial, server, duration, interval, bitrate, port=5201, udp=False, reverse=False, bidirectional=False):
    """
    Runs an iperf3 network performance test on an Android device.
    
    Parameters:
        device_serial (str): Serial number of the Android device.
        server (str): IP address of the iperf3 server.
        duration (str): Duration of the test in seconds.
        interval (str): Interval in seconds for periodic reporting.
        bitrate (str): Target bitrate for the test.
        port (str): Port number for the iperf3 server.
        udp (bool): If True, use UDP protocol. Default is False (TCP).
        reverse (bool): If True, run reverse test from server to client.
    
    Returns:
        dict: Parsed JSON output from iperf3.
    """

    command = ['adb' ,'-s', device_serial, 'shell', '/data/local/tmp/iperf3.9', '-c', server, '-t', str(duration), '-i', str(interval), '-p', str(port), '-J']
    if udp:
        command.append('-u')  
    if reverse:
        command.append('-R') 
    if bidirectional:
        command.append('--bidir')
    if bitrate:
        command.extend(['-b', bitrate]) 

    try: 
        result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        iperf3json = json.loads(result.stdout)

        return iperf3json
    except subprocess.CalledProcessError as e:
        try:
            error_output = json.loads(e.stderr)
            print(f"Error: {error_output['error']}")
        except json.JSONDecodeError:
            print(e.stderr.decode())

def ping(device_serial, count, destination):
    """
    Sends ping requests from an Android device to a specified destination.
    
    Parameters:
        device_serial (str): Serial number of the Android device.
        count (int): Number of ping requests to send.
        destination (str): IP address or hostname to ping.
    
    Returns:
        dict: Parsed information about ping responses and round-trip statistics.
    """
        
    command = ['adb', '-s', device_serial, 'shell','ping', '-c', str(count), destination]
    
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        ping_output = result.stdout
        
        parsed_ping = {
            "destination_ip": None,
            "data_bytes": None,
            "destination": destination,
            "packets_transmitted": None,
            "packets_received": None,
            "packet_loss_percent": None,
            "time_ms": None,
            "round_trip_ms_min": None,
            "round_trip_ms_avg": None,
            "round_trip_ms_max": None,
            "round_trip_ms_stddev": None,
            "responses": [],
        }

        header_match = re.search(
            r'^PING\s+([a-zA-Z0-9\.-]+)\s+\(([\d.:]+)\)\s+(\d+)\((\d+)\)\s+bytes of data',
            ping_output,
            re.MULTILINE
        )
        if header_match:
            parsed_ping["destination_ip"] = header_match.group(2)
            parsed_ping["data_bytes"] = int(header_match.group(3))
        
        response_matches = re.findall(
            r'(\d+)\s+bytes from\s+[\w.-]*\s*\(?([\d.]+)\)?:\s+icmp_seq=(\d+)\s+ttl=(\d+)\s+time=([\d.]+)\s+ms', 
            ping_output
        )
        
        for match in response_matches:
            response = {
                "type": "reply",
                "bytes": int(match[0]),
                "response_ip": match[1],
                "icmp_seq": int(match[2]),
                "ttl": int(match[3]),
                "time_ms": float(match[4]),
            }
            parsed_ping["responses"].append(response)

        packet_stats = re.search(
            r'(\d+) packets transmitted, (\d+) received, (\d+)% packet loss, time (\d+)ms',
            ping_output
        )
        if packet_stats:
            parsed_ping["packets_transmitted"] = int(packet_stats.group(1))
            parsed_ping["packets_received"] = int(packet_stats.group(2))
            parsed_ping["packet_loss_percent"] = float(packet_stats.group(3))
            parsed_ping["time_ms"] = int(packet_stats.group(4))

        rtt_stats = re.search(
            r'rtt min/avg/max/mdev = ([\d.]+)/([\d.]+)/([\d.]+)/([\d.]+)\s+ms',
            ping_output
        )
        if rtt_stats:
            parsed_ping["round_trip_ms_min"] = float(rtt_stats.group(1))
            parsed_ping["round_trip_ms_avg"] = float(rtt_stats.group(2))
            parsed_ping["round_trip_ms_max"] = float(rtt_stats.group(3))
            parsed_ping["round_trip_ms_stddev"] = float(rtt_stats.group(4))
        return parsed_ping

    except subprocess.CalledProcessError as e:
        print(f"Ping command failed with return code {e.returncode}: {e.stderr}")
        return None

def parse_args():
    parser = argparse.ArgumentParser(
                        prog='logger.py',
                        formatter_class=argparse.RawDescriptionHelpFormatter,
                        description=('''\

    Description: Runs iperf3, ping and retrieves signal information from an UE specified by the SERIAL NUMBER
                 storing the result in JSON format.

    Example: python3 logger.py -i 1 -t 10 -R -U -bidir
    '''),
                        add_help=True)

    parser.add_argument('-i', '--interval', metavar='seconds', help='interval between periodic reports (default = 1s)', dest='interval',default=1)  
    parser.add_argument('-t', '--time', metavar='seconds', help='time to transmit in seconds (default = 10s)', dest='time',default=10)
    parser.add_argument('-R', '--reverse', action='store_true', help='run IPERF3 in reverse mode (server sends, client receives)', dest='reverse')
    parser.add_argument('-U','--udp', action='store_true', help='run IPERF3 in UDP rather than TCP', dest='udp')
    parser.add_argument('-B','--bidir', action='store_true', help='run IPERF3 in bidirectional mode', dest='bidir')
   
    return parser.parse_args()

def transfer_file(destination):
    json_files = glob.glob("*.json")
    if not json_files:
        print("No JSON files found for transfer.")
        return None

    command = ["scp"] + json_files + [destination]
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        #print(f"File transfer successful. Output:\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"File transfer failed with error:\n{e.stderr}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

def remove_files():
    json_files = glob.glob("*.json")
    if not json_files:
        print("No JSON files found for transfer.")
        return None
    
    command = ['rm', '-r'] + json_files
    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        #print(f"Files removed successfully. Output:\n{result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Error occurred while removing files:\n{e.stderr}")
    except FileNotFoundError:
        print("Error: 'rm' command not found. Please ensure it is installed and in your PATH.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

def main():
    args = parse_args()
   
    udp = args.udp
    reverse = args.reverse
    bidir = args.bidir
    count = args.time
    interval = args.interval

    bitrate = '10000000M'
    port = '5201'
    destination = '10.11.23.204'
    serial_number1 = 'A75259FRCN9A2DV0538'
    serial_number2 = 'RZCXB18P4RL'
    adb_client = '10.11.32.205'
    timestr = time.strftime("%Y%m%d-%H%M%S")

    print("Logging started...")

    with ProcessPoolExecutor() as executor:
        future_iperf3 = executor.submit(iperf3, serial_number1, destination, count, interval, bitrate, port, udp, reverse, bidir)
        future_ping = executor.submit(ping, serial_number1, count, destination)
        future_cellinfo = executor.submit(cell_info, adb_client, serial_number1, int(count), int(interval))
        future_cellinfo2 = executor.submit(cell_info, adb_client, serial_number2, int(count), int(interval))
        
        iperf3json = future_iperf3.result()
        pingjson = future_ping.result()
        cellinfojson = future_cellinfo.result()
        cellinfojson2 = future_cellinfo2.result()

    file_suffix = "udp-" if udp else ""
    file_suffix += "reverse-" if reverse else ""
    file_suffix += "bidir-" if bidir else ""
    file_suffix += timestr
    
    with open(f"iperf3-{file_suffix}.json", "w") as outfile:
        json.dump(iperf3json, outfile)

    with open(f"ping-{file_suffix}.json", "w") as outfile:
        json.dump(pingjson, outfile)

    with open(f"cell_info-{file_suffix}.json", "w") as outfile:
        outfile.write(cellinfojson)
    
    with open(f"cell_info2-{file_suffix}.json", "w") as outfile:
        outfile.write(cellinfojson2)

    transfer_file(f'morse@{destination}:/home/morse/logs')
    remove_files()

    print("Logging finished.")

if __name__ == '__main__':
    main()
