import os
import re
import csv

DATA_DIR = "/DATA/rohit/krish/MTP/ns-allinone-3.36.1/ns-3.36.1/logsAnova5"
OUTPUT_CSV = "loss_finalComparisonwithBD5.csv"

FILENAME_PATTERN = re.compile(
    r"loss_rate(?P<rate>\w+)_mean(?P<mean>\d+)_ncars(?P<ncars>\d+)_pkt(?P<pkt>\d+)_q(?P<queue>\d+)_prob(?P<prob>\d+)_run(?P<run>\d+)\.txt"
)

rows = []

for filename in os.listdir(DATA_DIR):

    if not filename.endswith(".txt"):
        continue

    match = FILENAME_PATTERN.match(filename)

    if not match:
        print(f"Skipping filename: {filename}")
        continue

    params = match.groupdict()
    mean = float("0." + params["mean"])

    file_path = os.path.join(DATA_DIR, filename)

    # -------------------------
    # Initialize totals
    # -------------------------
    tr1_total = 0
    rs1_total = 0

    tr2_total = 0
    rs2_total = 0

    tr_total = 0
    rs_total = 0

    with open(file_path, "r") as f:

        for line in f:

            pairs = re.findall(r"(\w+):\s*(\d+)", line)

            if not pairs:
                continue

            pairs = {k: int(v) for k, v in pairs}

            tr1_total += pairs.get("TotalReceivedDist1", 0)
            rs1_total += pairs.get("ReceivedSuccessfullyDist1", 0)

            tr2_total += pairs.get("TotalReceivedDist2", 0)
            rs2_total += pairs.get("ReceivedSuccessfullyDist2", 0)

            tr_total += pairs.get("TotalReceived", 0)
            rs_total += pairs.get("ReceivedSuccessfully", 0)

    # -------------------------
    # Compute PLR after summing
    # -------------------------
    plr_dist1 = (tr1_total - rs1_total) / tr1_total if tr1_total > 0 else 0
    plr_dist2 = (tr2_total - rs2_total) / tr2_total if tr2_total > 0 else 0
    plr_total = (tr_total - rs_total) / tr_total if tr_total > 0 else 0

    prob = int(params["prob"]) / 1000

    row = [
        params["rate"],
        mean,
        int(params["ncars"]),
        int(params["pkt"]),
        int(params["queue"]),
        prob,
        int(params["run"]),
        plr_dist1,
        plr_dist2,
        plr_total
    ]

    rows.append(row)

print(len(rows), "rows parsed successfully.")

# -------------------------
# Write CSV
# -------------------------
with open(OUTPUT_CSV, "w", newline="") as f:

    writer = csv.writer(f)

    writer.writerow([
        "rate",
        "mean_interarrival",
        "nCars",
        "packet_size_bytes",
        "queue_size",
        "prob",
        "run",
        "PLR_Dist1",
        "PLR_Dist2",
        "PLR_Total"
    ])

    writer.writerows(rows)

print(f"✅ CSV written to {OUTPUT_CSV}")
print(f"Total rows written: {len(rows)}")