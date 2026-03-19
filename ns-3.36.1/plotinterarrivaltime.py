import pandas as pd
import matplotlib.pyplot as plt
import re
from collections import defaultdict

# Step 1: Read and parse the file
data = []
with open('incomingLog.txt', 'r') as f:
    for line in f:
        # Extract fields using regex
        match = re.match(
            r'Src:\s+([0-9a-fA-F:]+)\s+DistributionType:\s+(\d+)\s+Time:\s+\+([\d.e+-]+)ns\s+queue-length:\s+(\d+)p',
            line
        )
        if match:
            src, dist_type, time_ns, queue_length = match.groups()
            data.append({
                'Src': src,
                'DistributionType': int(dist_type),
                'Time': float(time_ns),
                'queue_length': int(queue_length)
            })

# Step 2: Load into DataFrame
df = pd.DataFrame(data)

# Step 3: Process and plot for each unique source
unique_sources = df['Src'].unique()

exp_inter_arrival_f = 0
const_inter_arrival_f = 0

for src in unique_sources:
    src_df = df[df['Src'] == src].sort_values('Time').reset_index(drop=True)
    
    # Calculate inter-arrival times in milliseconds
    src_df['InterArrival'] = src_df['Time'].diff().fillna(0) / 1e6  # ns to ms
    
    # Filter distributions
    exp_inter_arrival = src_df[src_df['DistributionType'] == 0]['InterArrival']
    const_inter_arrival = src_df[src_df['DistributionType'] == 3]['InterArrival']
    
    exp_inter_arrival_f += len(exp_inter_arrival)
    const_inter_arrival_f += len(const_inter_arrival)
    
    # Filter large inter-arrival times
    exp_inter_arrival_filtered = exp_inter_arrival[exp_inter_arrival < 100]
    const_inter_arrival_filtered = const_inter_arrival[const_inter_arrival < 100]
    
    # Step 4: Plotting
    fig, axes = plt.subplots(3, 1, figsize=(12, 18))
    fig.suptitle(f'Source: {src}', fontsize=16)
    
    # Histogram: Exponential Inter-Arrival Time
    axes[0].hist(exp_inter_arrival_filtered, bins=50, alpha=0.7, label='Exponential (DistType 0)', color='blue')
    axes[0].set_xlabel('Inter-Arrival Time (ms)', fontsize=20)
    axes[0].set_ylabel('Frequency', fontsize=20)
    axes[0].set_title('Exponential Inter-Arrival Time Distribution', fontsize=20)
    axes[0].legend()
    axes[0].grid(True)
    
    # Histogram: Constant Inter-Arrival Time
    axes[1].hist(const_inter_arrival_filtered, bins=50, alpha=0.7, label='Constant (DistType 3)', color='orange')
    axes[1].set_xlabel('Inter-Arrival Time (ms)', fontsize=20)
    axes[1].set_ylabel('Frequency', fontsize=20)
    axes[1].set_title('Constant Inter-Arrival Time Distribution', fontsize=20)
    axes[1].legend()
    axes[1].grid(True)
    
    # Queue Length vs Time
    axes[2].plot(src_df['Time'], src_df['queue_length'], marker='o', linestyle='-', label='Queue Length', color='green')
    axes[2].set_xlabel('Time (ms)', fontsize=20)
    axes[2].set_ylabel('Queue Length', fontsize=20)
    axes[2].set_title('Queue Length Over Time', fontsize=20)
    axes[2].legend()
    axes[2].grid(True)
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    plt.savefig(f"analysis_{src.replace(':', '')}.png")
    plt.close()

print(f"Total Exponential Inter-Arrival Count: {exp_inter_arrival_f}")
print(f"Total Constant Inter-Arrival Count: {const_inter_arrival_f}")