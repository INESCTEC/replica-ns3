import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import sys
import os
import re

# Check if the user provided at least one file name
if len(sys.argv) <= 1:
    print('Missing file name(s)\nUsage: python plotter.py ../logs/.../propagation-loss-dataset.csv ../simulations/file2.csv ...')
    sys.exit(0)

input_files = sys.argv[1:]

colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k']  # Define colors for different files

def extract_label(filename):
    base_name = os.path.basename(filename)
    dir_name = os.path.dirname(filename)
    parent_folder = os.path.basename(dir_name)

    if 'logs' in dir_name:
        return 'real'

    elif 'simulations' in dir_name:
        try:
            parts = base_name.split('-')
            if parts[0] == 'xgb':
                parts[0] = 'MLPL'
            if parts[0] == 'trace':
                filtered_parts = [p for i, p in enumerate(parts[:2]) if i != 2]
            else:
                filtered_parts = [p for i, p in enumerate(parts[:1]) if i != 1]
            truncated_name = '-'.join(filtered_parts) if filtered_parts else base_name
            return truncated_name
        except Exception as e:
            print(f"Error processing {filename}: {e}")
            return base_name

    return base_name

def extract_dist_value(filename):
    match = re.search(r'dist(\d+)', filename)
    return f"Distance={match.group(1)}m" if match else "distX"

def extract_title_info(filename):
    protocol = 'tcp' if 'tcp' in filename.lower() else 'udp' if 'udp' in filename.lower() else 'unknown'
    direction = 'uplink' if 'uplink' in filename.lower() else 'downlink' if 'downlink' in filename.lower() else 'unknown'
    distance = extract_dist_value(filename)
    return f'{protocol.upper()} {direction.capitalize()} {distance}'

def get_throughput_column(filename, data):
    dir_name = os.path.dirname(filename)
    if 'simulations' in dir_name:
        if 'uplink' in filename and 'throughput_kbps_uplink' in data.columns:
            return data['throughput_kbps_uplink'].dropna()
        elif 'downlink' in filename and 'throughput_kbps_downlink' in data.columns:
            return data['throughput_kbps_downlink'].dropna()
        else:
            print(f"Error: No appropriate throughput column found in {filename}.")
            return pd.Series()
    else:
        if 'throughput_kbps' in data.columns:
            return data['throughput_kbps'].dropna()
        else:
            print(f"Error: 'throughput_kbps' column not found in {filename}.")
            return pd.Series()

# Prepare folder and dist info
log_folders = [os.path.basename(os.path.dirname(f)) for f in input_files if 'logs' in f]
log_folder_name = log_folders[0] if log_folders else 'default'
dist_value = extract_dist_value(' '.join(input_files))

# Extract plot title info
plot_title_info = extract_title_info(' '.join(input_files))

# CPF Plot
cpf_filename = f'cpf_{log_folder_name}_{dist_value}.png'
plt.figure(figsize=(8, 6))
plt.title(f'Cumulative Probability Function (CPF) - Throughput\n{plot_title_info}')

for i, input_file_name in enumerate(input_files):
    if not os.path.exists(input_file_name):
        print(f"Error: File {input_file_name} not found.")
        continue

    data = pd.read_csv(input_file_name)
    throughput = get_throughput_column(input_file_name, data)
    if throughput.empty:
        continue

    sorted_throughput = np.sort(throughput)
    cdf = np.arange(1, len(sorted_throughput) + 1) / len(sorted_throughput)

    color = colors[i % len(colors)]
    label = extract_label(input_file_name)
    plt.plot(sorted_throughput, cdf, marker='.', linestyle='none', label=label, color=color)

plt.xlabel('Throughput Kbps')
plt.ylabel('Cumulative Probability')
plt.legend()
plt.grid(True)
plt.savefig(cpf_filename)
plt.close()

# Boxplot
boxplot_filename = f'boxplot_{log_folder_name}_{dist_value}.png'
plt.figure(figsize=(8, 6))
plt.title(f'Throughput Distribution\n{plot_title_info}')

all_throughputs = []
labels = []

for input_file_name in input_files:
    if not os.path.exists(input_file_name):
        continue

    data = pd.read_csv(input_file_name)
    throughput = get_throughput_column(input_file_name, data)
    if throughput.empty:
        continue

    all_throughputs.append(throughput)
    label = extract_label(input_file_name)
    labels.append(label)

if all_throughputs:
    plt.boxplot(all_throughputs, vert=True, patch_artist=True, tick_labels=labels)
    plt.ylabel('Throughput Kbps')
    plt.grid(True)
    plt.savefig(boxplot_filename)
    plt.close()

# Summary CSV
summary_data = []

for input_file_name in input_files:
    if not os.path.exists(input_file_name):
        continue

    data = pd.read_csv(input_file_name)
    throughput = get_throughput_column(input_file_name, data)
    if throughput.empty:
        continue

    label = extract_label(input_file_name)
    percentiles = np.percentile(throughput, [0, 25, 50, 75, 100])
    mean_value = np.mean(throughput)

    summary_data.append({
        'Type': label,
        'Min 0%': percentiles[0],
        '25%': percentiles[1],
        '50%': percentiles[2],
        '75%': percentiles[3],
        'Max 100%': percentiles[4],
        'Mean': mean_value
    })

summary_df = pd.DataFrame(summary_data)
summary_filename = f'summary_{log_folder_name}_{dist_value}.csv'
summary_df.to_csv(summary_filename, index=False)

print(f'Generated {cpf_filename}, {boxplot_filename}, and {summary_filename}.')
