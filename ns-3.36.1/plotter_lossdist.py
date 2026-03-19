import pandas as pd
import matplotlib.pyplot as plt

# 1. Load the data
try:
    df = pd.read_csv('results.csv')
except FileNotFoundError:
    print("Error: results.csv not found. Please run your Bash script first.")
    exit()

# 2. Clean data: Remove rows where Sent or Received are empty/NaN
df = df.dropna(subset=['Sent', 'Received'])

# 3. Transform Distance for better axis readability (e.g., units of 10^10 meters)
df['Dist_units'] = df['Distance'] / 1e10

# 4. Create the plot
plt.figure(figsize=(12, 7))

# Plot Sent Packets (usually a constant line)
plt.plot(df['Dist_units'], df['Sent'], label='Packets Sent', color='red', linestyle='--', alpha=0.6)

# Plot Received Packets (the curve showing the Friis loss)
plt.plot(df['Dist_units'], df['Received'], label='Packets Received', color='blue', linewidth=2)

# 5. Add details
plt.title('Packet Reception vs Distance (Friis Propagation Model)', fontsize=14)
plt.xlabel('Distance ($10^{10}$ meters)', fontsize=12)
plt.ylabel('Number of Packets', fontsize=12)
plt.grid(True, which="both", ls="-", alpha=0.3)
plt.legend()

# 6. Optional: Zoom in on the "cliff" where it drops to zero
# plt.xlim(2.8, 3.05) 

# 7. Save and show
plt.savefig('packet_graph.png', dpi=300)
print("Graph saved as packet_graph.png")
# plt.show()