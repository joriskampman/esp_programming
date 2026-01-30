#!/bin/bash
set +H

echo "=== Building for SMART network ==="
echo ""

# Generate the config file
./generate_wifi_config.sh

echo ""
echo "Starting build..."
echo ""

# Build the project
clear && idf.py build flash monitor -p /dev/ttyACM0