import os
import re
import csv
import numpy as np

DATA_DIR = "/DATA/rohit/krish/MTP/ns-allinone-3.36.1/ns-3.36.1/logsAnova"
OUTPUT_CSV = "loss_resultsAnova.csv"

# filename parser
FILENAME_PATTERN = re.compile(
    r"loss_rate(?P<rate>\w+)_mean(?P<mean>\d+)_ncars(?P<ncars>\d+)_pkt(?P<pkt>\d+)_q(?P<queue>\d+)_prob(?P<prob>\d+)_run(?P<run>\d+)\.txt"
)

# line parser
LINE_PATTERN = re.compile(
    r"Sent:\s*(\d+)\s+notSent:\s*(\d+).*ReceivedSuccessfully:\s*(\d+)"
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
    mean = float("0." + params["mean"][1:])
    probaility = float("0." + params["prob"][1:])

    
    loss_rate = 0.0
    file_path = os.path.join(DATA_DIR, filename)

    Sent = 0
    notSent = 0
    ReceivedSuccessfully = 0
    no_of_nodes = 0
    with open(file_path, "r") as f:
        for line in f:
            m = LINE_PATTERN.search(line)
            if m:
                no_of_nodes += 1
                Sent += int(m.group(1))
                notSent += int(m.group(2))
                ReceivedSuccessfully += int(m.group(3))
                # if Sent + notSent < ReceivedSuccessfully:
                #     print(f"Warning: Inconsistent data in file {filename}")
                    
    total_sent = (Sent + notSent)*(no_of_nodes-1)
    if total_sent > 0:
        loss_rate = (1 - ((ReceivedSuccessfully / total_sent)))*100

    rows.append([
        params["rate"],
        mean,
        int(params["ncars"]),
        int(params["pkt"]),
        int(params["queue"]),
        probaility,
        int(params["run"]),
        loss_rate
    ])

# write CSV
with open(OUTPUT_CSV, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "rate",
        "mean_interarrival",
        "nCars",
        "packet_size_bytes",
        "queue_size",
        "probability",
        "run",
        "avg_packet_loss_rate"
    ])
    writer.writerows(rows)

print(f"✅ CSV written to {OUTPUT_CSV}")
