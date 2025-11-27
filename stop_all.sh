#!/bin/bash

# Stop all running imaging service applications

echo "Stopping all Distributed Imaging Services..."

# Kill by process name
pkill -f image_generator
pkill -f feature_extractor
pkill -f data_logger

# Kill tmux session if it exists
tmux kill-session -t imaging_services 2>/dev/null || true

echo "All applications stopped."
