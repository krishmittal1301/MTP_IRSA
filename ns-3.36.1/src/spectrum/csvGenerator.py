import os
import re
import csv
import numpy as np

DATA_DIR = "/DATA/rohit/krish/MTP/ns-allinone-3.36.1/ns-3.36.1/logs3"
OUTPUT_CSV = "loss_results5.csv"

# filename parser
FILENAME_PATTERN = re.compile(
    r"delay_rate(?P<rate>\w+)_mean(?P<mean>\d+)_dist(?P<Distance>\d+)_pkt(?P<pkt>\d+)_q(?P<queue>\d+)\.txt"
)

# line parser
LINE_PATTERN = re.compile(
    r"Average Delay:\s*([\d.]+)\s*seconds over\s*(\d+)\s*packets"
)

rows = []

for filename in os.listdir(DATA_DIR):
    if not filename.endswith(".txt"):
        continue

    match = FILENAME_PATTERN.match(filename)
    if not match:
        print(f"Skipping filename (pattern mismatch): {filename}")
        continue

    params = match.groupdict()
    mean = float("0." + params["mean"])
    
    loss_rate = 0.0
    file_path = os.path.join(DATA_DIR, filename)

    no_of_nodes = 0
    total_packets = 0
    total_delay = 0.0
    with open(file_path, "r") as f:
        for line in f:
            # print(line)
            m = LINE_PATTERN.search(line)
            if m:
                no_of_nodes += 1
                delay = float(m.group(1))
                packets = int(m.group(2))

                total_delay += delay * packets
                # print(delay, packets)
                total_packets += packets

                # if Sent + notSent < ReceivedSuccessfully:
                #     print(f"Warning: Inconsistent data in file {filename}")
                    
        avg_delay = total_delay / total_packets if total_packets > 0 else 0.0

    rows.append([
        params["rate"],
        mean,
        params["Distance"],
        int(params["pkt"]),
        int(params["queue"]),
        avg_delay
    ])

# write CSV
with open(OUTPUT_CSV, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "rate",
        "mean_interarrival",
        "distance",
        "packet_size_bytes",
        "queue_size",
        "avg_packet_delay_seconds"
    ])
    writer.writerows(rows)

print(f"✅ CSV written to {OUTPUT_CSV}")
