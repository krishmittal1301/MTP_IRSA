#!/bin/bash

SIM_TIME=8
OUT_DIR="logsAnova5"
mkdir -p "$OUT_DIR"

RATES=("3Mbps" "9Mbps" "27Mbps")
MEANS=(0.06 0.028 0.018)          # mean = 1/lambda
PACKET_SIZES=(50 100 200)         # in bytes
QUEUE_SIZES=(100)
NO_OF_CARS=(10 20 30 40 50)
PROBABILTY_OF_SEND=(0.01 0.05 0.1)
RUNS=(1)


run_one() {
  RATE=$1
  MEAN=$2
  PKT=$3
  QSIZE=$4
  NCARS=$5
  PROBABILITY_OF_SEND=$6
  RUN=$7

  RATE_TAG=${RATE/Mbps/M}
  MEAN_TAG=$(printf "%.3f" "$MEAN" | sed 's/\.//')
  NCARS_TAG=$NCARS
  PROB_TAG=$(printf "%.3f" "$PROBABILITY_OF_SEND" | sed 's/\.//')

  EXP_TAG="rate${RATE_TAG}_mean${MEAN_TAG}_ncars${NCARS_TAG}_pkt${PKT}_q${QSIZE}_prob${PROB_TAG}_run${RUN}"

  echo "Running $EXP_TAG"

  ./build/src/spectrum/examples/ns3.36.1-adhoc-aloha-ideal-phy-default \
    --simTime=$SIM_TIME \
    --rate=$RATE \
    --packetSize=$PKT \
    --queueSize=$QSIZE \
    --mean=$MEAN \
    --noOfCars=$NCARS \
    --delayLog=$OUT_DIR/delay_${EXP_TAG}.txt \
    --lossLog=$OUT_DIR/loss_${EXP_TAG}.txt \
    --macDelayLog=$OUT_DIR/mac_${EXP_TAG}.txt \
    --ProbabilityOfSend=$PROBABILITY_OF_SEND \
    --run=$RUN
}

export -f run_one
export SIM_TIME OUT_DIR

./parallel -j 12 run_one ::: \
  "${RATES[@]}" ::: \
  "${MEANS[@]}" ::: \
  "${PACKET_SIZES[@]}" ::: \
  "${QUEUE_SIZES[@]}" ::: \
  "${NO_OF_CARS[@]}" ::: \
  "${PROBABILTY_OF_SEND[@]}" ::: \
  "${RUNS[@]}"
