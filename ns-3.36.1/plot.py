import re
import pandas as pd
import matplotlib.pyplot as plt

# Step 1: Read and parse the file
data = []
with open('delayLog.txt', 'r') as f:
    for line in f:
        match = re.search(
            r'Delay:\s+([0-9.e-]+)\s+PacketSize:\s+(\d+)\s+DistributionType:\s+(\d+)\s+Time:\s+\+([0-9.e+-]+)ns',
            line
        )
        if match:
            data.append({
                'Delay': float(match.group(1)),
                'PacketSize': int(match.group(2)),
                'DistributionType': int(match.group(3)),
                'Time': float(match.group(4))
            })

df = pd.DataFrame(data)

# Step 2: Map distribution types (remove numeric labels)
type_map = {
    0: "Event-Driven",
    3: "Regular"
}

# Print stats
print("\nPacket counts by Distribution Type:")
for dist, count in df['DistributionType'].value_counts().items():
    print(f"{type_map.get(dist, 'Unknown')}: {count}")

print("\nAverage Delay by Distribution Type:")
for dist, avg in df.groupby('DistributionType')['Delay'].mean().items():
    print(f"{type_map.get(dist, 'Unknown')}: {avg:.6f}")

# Step 3: Plot
dtypes = df['DistributionType'].unique()

for dtype in dtypes:
    subset = df[df['DistributionType'] == dtype]
    label = type_map.get(dtype, "Unknown")

    plt.figure(figsize=(12, 5))
    plt.scatter(subset['Time'], subset['Delay'], marker='o')
    plt.title(f"Packet Delay vs Time — {label}", fontsize=22)
    plt.xlabel("Time (ms)", fontsize=20)
    plt.ylabel("Delay (s)", fontsize=20)
    plt.grid(True)
    plt.tight_layout()

    # Save image with readable name
    filename = f"delay_vs_time_{label.replace('-', '').lower()}.png"
    plt.savefig(filename)
    plt.close()

    print(f"Saved: {filename}")
