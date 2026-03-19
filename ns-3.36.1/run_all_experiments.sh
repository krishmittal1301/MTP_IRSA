#!/bin/bash

SIM_TIME=40
MAX_DISTANCE=300
OUT_DIR="logs"
mkdir -p $OUT_DIR

RATES=("1Mbps" "20Mbps" "100Mbps")
MEANS=(0.001 0.005 0.01 0.05 0.1 0.5)
DISTANCES=(3 4 5 6 7 8 9 10)
PACKET_SIZES=(64 125 512)
QUEUE_SIZES=(50 100 500 1000 5000)

run_one() {
  RATE=$1
  MEAN=$2
  DIST=$3
  PKT=$4
  QSIZE=$5

  NNODES=$((MAX_DISTANCE / DIST))

  RATE_TAG=${RATE/Mbps/M}
  MEAN_TAG=$(printf "%.3f" $MEAN | sed 's/\.//')
  EXP_TAG="rate${RATE_TAG}_mean${MEAN_TAG}_dist${DIST}_pkt${PKT}_q${QSIZE}"

  echo "Running $EXP_TAG (nodes=$NNODES)"

  ./build/src/spectrum/examples/ns3.36.1-adhoc-aloha-ideal-phy-default \
    --nNodes=$NNODES \
    --distance=$DIST \
    --simTime=$SIM_TIME \
    --queueSize=$QSIZE \
    --rate=$RATE \
    --packetSize=$PKT \
    --mean=$MEAN \
    --delayLog=$OUT_DIR/delay_${EXP_TAG}.txt \
    --lossLog=$OUT_DIR/loss_${EXP_TAG}.txt \
    --macDelayLog=$OUT_DIR/mac_${EXP_TAG}.txt
}

export -f run_one
export SIM_TIME MAX_DISTANCE OUT_DIR

./parallel -j 10 run_one ::: \
  "${RATES[@]}" ::: \
  "${MEANS[@]}" ::: \
  "${DISTANCES[@]}" ::: \
  "${PACKET_SIZES[@]}" ::: \
  "${QUEUE_SIZES[@]}"
