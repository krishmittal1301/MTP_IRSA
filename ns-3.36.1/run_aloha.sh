#!/bin/bash
# Wrapper script to run adhoc-aloha-ideal-phy with parameters

# Default values
RATE="20Mbps"
MEAN="0.008"
VERBOSE="0"

# If arguments are provided, override defaults
if [ ! -z "$1" ]; then
  RATE=$1
fi

if [ ! -z "$2" ]; then
  MEAN=$2
fi

if [ ! -z "$3" ]; then
  VERBOSE=$3
fi

echo "Running ALOHA simulation with:"
echo "  Rate: $RATE"
echo "  Mean: $MEAN"
echo "  Verbose: $VERBOSE"
echo ""

# Run ns3 simulation
./ns3 run src/spectrum/examples/adhoc-aloha-ideal-phy.cc -- --rate=$RATE --mean=$MEAN --verbose=$VERBOSE
