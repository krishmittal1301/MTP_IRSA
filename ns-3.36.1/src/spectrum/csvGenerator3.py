import os
import re
import csv

DATA_DIR = "/DATA/rohit/krish/MTP/ns-allinone-3.36.1/ns-3.36.1/logsAnova3_changing_startTime"
OUTPUT_CSV = "Delay_finalComparisionwithBD2.csv"

# -------- filename parser --------
FILENAME_PATTERN = re.compile(
    r"delay_rate(?P<rate>\w+)_mean(?P<mean>\d+)_ncars(?P<ncars>\d+)_pkt(?P<pkt>\d+)_q(?P<queue>\d+)_prob(?P<prob>\d+)_run(?P<run>\d+)_dist(?P<dist>\d+)\.txt"
)

# -------- line parser --------
LINE_PATTERN = re.compile(
    r"Final Average Delay \(Dist\d+\):\s*([\d.]+)\s*Over packet:\s*(\d+)"
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
    prob = float("0." + params["prob"])
    distribution = int(params["dist"])   # ✅ distribution type (1 or 2)

    file_path = os.path.join(DATA_DIR, filename)

    total_packets = 0
    total_delay = 0.0
    runs = 0

    with open(file_path, "r") as f:
        for line in f:
            m = LINE_PATTERN.search(line)
            if m:
                delay = float(m.group(1))
                packets = int(m.group(2))

                total_delay += delay * packets
                total_packets += packets
                runs += 1

    avg_delay = total_delay / total_packets if total_packets > 0 else 0.0

    rows.append([
        params["rate"],
        mean,
        int(params["ncars"]),
        int(params["pkt"]),
        int(params["queue"]),
        prob,
        distribution,
        runs,
        avg_delay,
        int(params["run"])
    ])

# -------- write CSV --------
with open(OUTPUT_CSV, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow([
        "rate",
        "mean_interarrival",
        "ncars",
        "packet_size_bytes",
        "queue_size",
        "send_probability",
        "distribution",
        "num_runs",
        "avg_packet_delay_seconds",
        "run"
    ])
    writer.writerows(rows)

print(f"✅ CSV written to {OUTPUT_CSV}")
