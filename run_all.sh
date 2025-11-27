#!/bin/bash

# Run all three applications in separate terminals
# This script demonstrates starting the distributed system

echo "=================================="
echo "Starting Distributed Imaging Services"
echo "=================================="
echo ""

# Check if executables exist
if [ ! -f "build/image_generator" ] || [ ! -f "build/feature_extractor" ] || [ ! -f "build/data_logger" ]; then
    echo "ERROR: Executables not found. Please run build.sh first."
    exit 1
fi

# Check if we're using tmux or starting in background
USE_TMUX=false
if command -v tmux &> /dev/null; then
    echo "Found tmux. Applications will run in tmux panes."
    echo "Press Ctrl+B then Q to see pane numbers."
    echo "Press Ctrl+B then X to close a pane."
    echo ""
    USE_TMUX=true
else
    echo "tmux not found. Applications will run in background."
    echo "Use 'pkill image_generator' etc. to stop them."
    echo ""
fi

# Create logs directory
mkdir -p logs

if [ "$USE_TMUX" = true ]; then
    # Start with tmux
    SESSION_NAME="imaging_services"
    
    # Kill existing session if it exists
    tmux kill-session -t $SESSION_NAME 2>/dev/null || true
    
    # Create new session and split into 3 panes
    tmux new-session -d -s $SESSION_NAME
    tmux split-window -h
    tmux split-window -v
    tmux select-pane -t 0
    
    # Run applications in each pane
    tmux send-keys -t $SESSION_NAME:0.0 "./build/data_logger tcp://localhost:5556 imaging_data.db 2>&1 | tee logs/data_logger.log" C-m
    sleep 1
    tmux send-keys -t $SESSION_NAME:0.1 "./build/feature_extractor tcp://localhost:5555 'tcp://*:5556' 2>&1 | tee logs/feature_extractor.log" C-m
    sleep 1
    tmux send-keys -t $SESSION_NAME:0.2 "./build/image_generator ./deep_sea_imaging/raw 'tcp://*:5555' 2>&1 | tee logs/image_generator.log" C-m
    
    # Attach to session
    echo "Attaching to tmux session..."
    echo "Press Ctrl+B then D to detach (apps keep running)"
    echo "Press Ctrl+C in any pane to stop that application"
    sleep 2
    tmux attach-session -t $SESSION_NAME
else
    # Start in background
    echo "Starting Data Logger..."
    ./build/data_logger tcp://localhost:5556 imaging_data.db > logs/data_logger.log 2>&1 &
    DATA_LOGGER_PID=$!
    echo "Data Logger PID: $DATA_LOGGER_PID"
    sleep 1
    
    echo "Starting Feature Extractor..."
    ./build/feature_extractor tcp://localhost:5555 'tcp://*:5556' > logs/feature_extractor.log 2>&1 &
    FEATURE_EXTRACTOR_PID=$!
    echo "Feature Extractor PID: $FEATURE_EXTRACTOR_PID"
    sleep 1
    
    echo "Starting Image Generator..."
    ./build/image_generator ./deep_sea_imaging/raw 'tcp://*:5555' > logs/image_generator.log 2>&1 &
    IMAGE_GENERATOR_PID=$!
    echo "Image Generator PID: $IMAGE_GENERATOR_PID"
    
    echo ""
    echo "All applications started!"
    echo "Logs are in the logs/ directory"
    echo ""
    echo "To stop all applications, run:"
    echo "  kill $IMAGE_GENERATOR_PID $FEATURE_EXTRACTOR_PID $DATA_LOGGER_PID"
    echo ""
    echo "Or use: pkill -f 'image_generator|feature_extractor|data_logger'"
fi
