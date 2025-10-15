import argparse
import subprocess
import json
import time

parser = argparse.ArgumentParser(
                    prog='iperf3_client.py',
                    formatter_class=argparse.RawDescriptionHelpFormatter,
                    description=('''\

Description:Runs iperf3 in client mode connecting to an iPerf server running on host 
to perform network throughput tests and store the result in JSON format.

Example: python3 iperf3_client.py -c 'localhost' -i 1 -t 10 -R -u -b 10000M
'''),
                    epilog='In normal mode client sends, server receives.',
                    add_help=True)

parser.add_argument('-b', '--bitrate', metavar='bitrate', help=' target bitrate in bits/sec', dest='bitrate') 
parser.add_argument('-c', '--client', metavar='<host>', help='connecting to <host> server (IPv4 literal, or IPv6 literal)',dest='server',required=True)
parser.add_argument('-i', '--interval', metavar='interval', help='seconds between periodic throughput reports (default = 1s)', dest='interval',default=1) 
parser.add_argument('-p', '--port', metavar='port', help='server port to listen on/connect to', dest='port',default=5201) 
parser.add_argument('-R', '--reverse', action='store_true', help='run in reverse mode (server sends, client receives)', dest='reverse') 
parser.add_argument('-t', '--time', metavar='time', help='time to transmit in seconds (default = 10s)', dest='time',default=10) 
parser.add_argument('-u', action='store_true', help=' use UDP rather than TCP', dest='udp') 

args = parser.parse_args()

server = args.server
duration = args.time
interval = args.interval
port = args.port
udp = args.udp
bitrate = args.bitrate

command = f"iperf3 -c {args.server} -t {args.time} -i {args.interval} -p {args.port} -J"
command += " -u" if args.udp else ""
command += " -R" if args.reverse else ""
command += f" -b {args.bitrate}" if args.bitrate else ""

try: 
    result = subprocess.run(command, shell= True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    output = json.loads(result.stdout)

    timestr = time.strftime("%Y%m%d-%H%M%S")
    file_name = "udp-" if args.udp else ""
    file_name += "reverse-" if args.reverse else ""
    file_name += timestr
    
    with open(f"iperf3-{file_name}.json", "w") as outfile:
         json.dump(output, outfile)

except subprocess.CalledProcessError as e:
    try:
        error_output = json.loads(e.stderr)
        print(f"Error: {error_output['error']}")
    except json.JSONDecodeError:
        print(e.stderr.decode())
