#!/bin/bash

# Output file for the graph
echo "Distance,Sent,Received" > results.csv

start=20000000000
end=30000000000
step=10000000

for (( i=$start; i<=$end; i+=$step ))
do
    echo "Running simulation for distance: $i meters..."
    
    # 1. Run simulation (we don't capture output here anymore)
    ./ns3 run src/spectrum/examples/adhoc-aloha-ideal-phy.cc -- --nNodes=2 --distance=$i --simTime=10 --queueSize=2000 > /dev/null 2>&1
    
    # 2. Extract Sent and Received from the FILE 'lossLog.txt'
    # We take the last occurrence in case the file appends
    sent=$(grep "Sent:" lossLog.txt | tail -1 | awk '{print $2}')
    received=$(grep "Received:" lossLog.txt | tail -1 | awk '{print $4}')
    
    # 3. Handle empty values (if grep fails)
    if [ -z "$sent" ]; then sent=0; fi
    if [ -z "$received" ]; then received=0; fi

    # 4. Save to CSV
    echo "$i,$sent,$received" >> results.csv
    
    # Optional: Clear the log file so it doesn't grow too big
    # > lossLog.txt 
done

echo "Done! Data saved in results.csv"