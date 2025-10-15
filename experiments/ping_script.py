import argparse
import subprocess
import json
import jc  
import time

parser = argparse.ArgumentParser(
                    prog='ping_script.py',
                    formatter_class=argparse.RawDescriptionHelpFormatter,
                    description=('''\

Description: Runs ping.

Example: python3 ping_script.py <destination> -c <count>
'''),
                    add_help=True)

parser.add_argument('destination')
parser.add_argument('-c', '--count', metavar='<count>', help='stop after <count> replies', dest='count', required=True)
args = parser.parse_args()

ping_command = ['ping', '-c', str(args.count), args.destination]

try: 
    result = subprocess.run(ping_command, capture_output=True, text=True, check=True)

    pingjson = jc.parse('ping', result.stdout)

    timestr = time.strftime("%Y%m%d-%H%M%S")
    
    with open(f"ping-{timestr}.json", "w") as outfile:
         json.dump(pingjson, outfile)

except subprocess.CalledProcessError as e:
    print(f"Ping command failed with return code {e.returncode}: {e.stderr}")
