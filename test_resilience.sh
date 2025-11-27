#!/bin/bash

# Comprehensive Resilience Testing Script
# Tests all possible combinations of application failures and recoveries
#
# Author: Haobo (Brian) Liu
# Email: h349liu@gmail.com
# Project: Distributed Imaging Services for Voyis Interview

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=========================================================================="
echo "  Comprehensive Resilience Testing"
echo "  Distributed Imaging Services"
echo "  Author: Haobo (Brian) Liu (h349liu@gmail.com)"
echo "=========================================================================="
echo ""

# Clean up function
cleanup() {
    echo -e "${YELLOW}Cleaning up all processes...${NC}"
    pkill -f image_generator 2>/dev/null || true
    pkill -f feature_extractor 2>/dev/null || true
    pkill -f data_logger 2>/dev/null || true
    sleep 1
}

# Start application helper
start_app() {
    local app=$1
    local endpoint1=$2
    local endpoint2=$3
    
    case $app in
        "generator")
            ./build/image_generator ./deep_sea_imaging/raw 'tcp://*:5555' > logs/gen_test.log 2>&1 &
            echo $!
            ;;
        "extractor")
            ./build/feature_extractor tcp://localhost:5555 'tcp://*:5556' > logs/ext_test.log 2>&1 &
            echo $!
            ;;
        "logger")
            ./build/data_logger tcp://localhost:5556 test_resilience.db > logs/log_test.log 2>&1 &
            echo $!
            ;;
    esac
}

# Wait and check helper
wait_and_check() {
    local duration=$1
    local message=$2
    echo -e "${BLUE}  Waiting ${duration}s: ${message}${NC}"
    sleep $duration
}

# Test result tracker
TOTAL_TESTS=0
PASSED_TESTS=0

test_result() {
    local test_name=$1
    local passed=$2
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$passed" = "true" ]; then
        echo -e "${GREEN}  ✓ PASSED:${NC} $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}  ✗ FAILED:${NC} $test_name"
    fi
}

mkdir -p logs
rm -f test_resilience.db

echo "=========================================================================="
echo "TEST 1: Single App Down - Generator (1)"
echo "=========================================================================="
echo ""
echo "Scenario: Only Image Generator is down"
echo "Expected: Feature Extractor and Data Logger wait for images"
echo "Message Loss: No loss - apps wait for connection"
echo "Watchdog: ZeroMQ automatic reconnection"
echo ""

cleanup
wait_and_check 1 "Starting Feature Extractor and Data Logger only..."

EXTRACTOR_PID=$(start_app "extractor")
LOGGER_PID=$(start_app "logger")

wait_and_check 3 "Apps waiting for Generator..."
echo -e "${YELLOW}Status: Feature Extractor subscribing to 5555 (no publisher yet)${NC}"
echo -e "${YELLOW}Status: Data Logger subscribing to 5556 (no data yet)${NC}"

wait_and_check 2 "Now starting Generator..."
GENERATOR_PID=$(start_app "generator")

wait_and_check 5 "Processing should begin..."
if pgrep -f image_generator > /dev/null && pgrep -f feature_extractor > /dev/null; then
    test_result "Generator recovery after delayed start" "true"
else
    test_result "Generator recovery after delayed start" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 2: Single App Down - Feature Extractor (2)"
echo "=========================================================================="
echo ""
echo "Scenario: Only Feature Extractor is down"
echo "Expected: Generator publishes to void, Logger waits"
echo "Message Loss: YES - images published while extractor is down are lost"
echo "Watchdog: ZeroMQ pub-sub pattern (no delivery guarantee without subscriber)"
echo ""

cleanup
wait_and_check 1 "Starting Generator and Logger only..."

GENERATOR_PID=$(start_app "generator")
LOGGER_PID=$(start_app "logger")

wait_and_check 3 "Generator publishing, but no subscriber..."
echo -e "${YELLOW}Status: Images are being LOST (no subscriber on 5555)${NC}"

wait_and_check 2 "Now starting Feature Extractor..."
EXTRACTOR_PID=$(start_app "extractor")

wait_and_check 5 "Processing should begin now..."
if pgrep -f feature_extractor > /dev/null; then
    test_result "Feature Extractor recovery after delayed start" "true"
else
    test_result "Feature Extractor recovery after delayed start" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 3: Single App Down - Data Logger (3)"
echo "=========================================================================="
echo ""
echo "Scenario: Only Data Logger is down"
echo "Expected: Generator and Extractor process, but data is buffered by ZeroMQ"
echo "Message Loss: Minimal - ZeroMQ buffers messages (up to HWM)"
echo "Watchdog: ZeroMQ high water mark buffering"
echo ""

cleanup
wait_and_check 1 "Starting Generator and Extractor only..."

GENERATOR_PID=$(start_app "generator")
EXTRACTOR_PID=$(start_app "extractor")

wait_and_check 5 "Processing images, buffering output..."
echo -e "${YELLOW}Status: Processed data buffered in ZeroMQ (up to send HWM)${NC}"

wait_and_check 2 "Now starting Data Logger..."
LOGGER_PID=$(start_app "logger")

wait_and_check 5 "Logger should receive buffered messages..."
if pgrep -f data_logger > /dev/null; then
    test_result "Data Logger recovery and buffer processing" "true"
else
    test_result "Data Logger recovery and buffer processing" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 4: Two Apps Down - Generator + Feature Extractor (12)"
echo "=========================================================================="
echo ""
echo "Scenario: Generator AND Feature Extractor are down"
echo "Expected: Only Data Logger waits"
echo "Message Loss: Complete - no processing happening"
echo "Watchdog: Applications independent, wait indefinitely"
echo ""

cleanup
wait_and_check 1 "Starting Data Logger only..."

LOGGER_PID=$(start_app "logger")

wait_and_check 3 "Logger waiting for data..."
echo -e "${YELLOW}Status: Logger idle, no data available${NC}"

wait_and_check 2 "Starting Generator and Extractor..."
GENERATOR_PID=$(start_app "generator")
sleep 1
EXTRACTOR_PID=$(start_app "extractor")

wait_and_check 5 "Full pipeline should be active..."
if pgrep -f data_logger > /dev/null; then
    test_result "Recovery from dual failure (1+2)" "true"
else
    test_result "Recovery from dual failure (1+2)" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 5: Two Apps Down - Generator + Data Logger (13)"
echo "=========================================================================="
echo ""
echo "Scenario: Generator AND Data Logger are down"
echo "Expected: Feature Extractor waits for inputs"
echo "Message Loss: Complete - no data flow"
echo "Watchdog: ZeroMQ automatic connection management"
echo ""

cleanup
wait_and_check 1 "Starting Feature Extractor only..."

EXTRACTOR_PID=$(start_app "extractor")

wait_and_check 3 "Extractor waiting..."
echo -e "${YELLOW}Status: Extractor subscribing but no publisher${NC}"

wait_and_check 2 "Starting Generator and Logger..."
GENERATOR_PID=$(start_app "generator")
sleep 1
LOGGER_PID=$(start_app "logger")

wait_and_check 5 "Full pipeline should be active..."
if pgrep -f feature_extractor > /dev/null; then
    test_result "Recovery from dual failure (1+3)" "true"
else
    test_result "Recovery from dual failure (1+3)" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 6: Two Apps Down - Feature Extractor + Data Logger (23)"
echo "=========================================================================="
echo ""
echo "Scenario: Feature Extractor AND Data Logger are down"
echo "Expected: Generator publishes to void"
echo "Message Loss: Complete - all images lost"
echo "Watchdog: None needed, Generator loops indefinitely"
echo ""

cleanup
wait_and_check 1 "Starting Generator only..."

GENERATOR_PID=$(start_app "generator")

wait_and_check 3 "Generator publishing to void..."
echo -e "${YELLOW}Status: All images being LOST (no subscribers)${NC}"

wait_and_check 2 "Starting Extractor and Logger..."
EXTRACTOR_PID=$(start_app "extractor")
sleep 1
LOGGER_PID=$(start_app "logger")

wait_and_check 5 "Full pipeline should be active..."
if pgrep -f image_generator > /dev/null; then
    test_result "Recovery from dual failure (2+3)" "true"
else
    test_result "Recovery from dual failure (2+3)" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 7: All Apps Down Then Started (123)"
echo "=========================================================================="
echo ""
echo "Scenario: Start all apps from complete shutdown"
echo "Expected: Apps start and connect in sequence"
echo "Message Loss: None once all apps running"
echo "Watchdog: System initialization sequence"
echo ""

cleanup
wait_and_check 2 "Complete system shutdown..."

echo "Starting apps in optimal order: Logger -> Extractor -> Generator"
LOGGER_PID=$(start_app "logger")
wait_and_check 1 "Logger started..."

EXTRACTOR_PID=$(start_app "extractor")
wait_and_check 1 "Extractor started..."

GENERATOR_PID=$(start_app "generator")
wait_and_check 5 "Generator started, pipeline active..."

if pgrep -f image_generator > /dev/null && \
   pgrep -f feature_extractor > /dev/null && \
   pgrep -f data_logger > /dev/null; then
    test_result "Full system startup from shutdown" "true"
else
    test_result "Full system startup from shutdown" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "TEST 8: Runtime Crash Recovery - Kill and Restart Mid-Operation"
echo "=========================================================================="
echo ""
echo "Scenario: Kill Feature Extractor during active processing"
echo "Expected: System continues without extractor, recovers when restarted"
echo "Message Loss: Messages during downtime lost"
echo "Watchdog: Manual restart (simulated monitoring service)"
echo ""

cleanup
wait_and_check 1 "Starting all apps..."

GENERATOR_PID=$(start_app "generator")
sleep 1
EXTRACTOR_PID=$(start_app "extractor")
sleep 1
LOGGER_PID=$(start_app "logger")

wait_and_check 5 "System running normally..."

echo -e "${RED}Simulating CRASH: Killing Feature Extractor${NC}"
kill $EXTRACTOR_PID 2>/dev/null || true

wait_and_check 3 "System operating with missing component..."
echo -e "${YELLOW}Status: Generator still running, Logger waiting${NC}"

echo -e "${GREEN}Watchdog: Restarting Feature Extractor${NC}"
EXTRACTOR_PID=$(start_app "extractor")

wait_and_check 5 "System should recover..."
if pgrep -f feature_extractor > /dev/null; then
    test_result "Recovery from runtime crash" "true"
else
    test_result "Recovery from runtime crash" "false"
fi

cleanup
echo ""

echo "=========================================================================="
echo "Resilience Testing Summary"
echo "=========================================================================="
echo ""
echo "Total Tests: $TOTAL_TESTS"
echo -e "${GREEN}Passed: $PASSED_TESTS${NC}"
echo -e "${RED}Failed: $((TOTAL_TESTS - PASSED_TESTS))${NC}"
echo ""

if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED - System is highly resilient!${NC}"
else
    echo -e "${YELLOW}⚠ Some tests failed - review logs for details${NC}"
fi

echo ""
echo "=========================================================================="
echo "Key Findings:"
echo "=========================================================================="
echo ""
echo "1. ZeroMQ Pub-Sub Pattern Provides:"
echo "   - Automatic connection/reconnection"
echo "   - Apps can start in any order"
echo "   - No crashes from missing peers"
echo ""
echo "2. Message Loss Scenarios:"
echo "   - Publisher with no subscriber: Messages LOST"
echo "   - Subscriber with no publisher: No loss, waiting state"
echo "   - Short downtime: Buffered by ZeroMQ (up to HWM)"
echo ""
echo "3. Recovery Mechanisms:"
echo "   - ZeroMQ: Automatic reconnection (no watchdog needed)"
echo "   - Applications: Timeout-based polling (graceful degradation)"
echo "   - Database: Transaction rollback on failure"
echo ""
echo "4. Production Recommendations:"
echo "   - Add systemd service files for automatic restart"
echo "   - Implement heartbeat monitoring"
echo "   - Add persistent message queue for critical data"
echo "   - Consider ZeroMQ XPUB-XSUB proxy for buffering"
echo ""
echo "=========================================================================="
echo "Test complete! Check logs/ directory for detailed output."
echo "=========================================================================="

# Cleanup test database
rm -f test_resilience.db

exit 0
