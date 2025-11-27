# Distributed Imaging Services - Quick Reference

## Quick Start (3 Commands)

```bash
./build.sh          # Build everything
./run_all.sh        # Start all apps
./stop_all.sh       # Stop all apps
```

## Individual Application Commands

### Image Generator
```bash
# Default (uses ./deep_sea_imaging/raw)
./build/image_generator

# Custom directory
./build/image_generator /path/to/images

# Custom endpoint
./build/image_generator /path/to/images tcp://*:6000
```

### Feature Extractor
```bash
# Default
./build/feature_extractor

# Custom endpoints
./build/feature_extractor tcp://localhost:6000 tcp://*:6001
```

### Data Logger
```bash
# Default
./build/data_logger

# Custom database
./build/data_logger tcp://localhost:5556 custom.db
```

## Database Queries

```bash
# Open database
sqlite3 imaging_data.db

# Count images
SELECT COUNT(*) FROM images;

# Count keypoints
SELECT COUNT(*) FROM keypoints;

# Average keypoints per image
SELECT AVG(kp_count) FROM (
    SELECT COUNT(*) as kp_count 
    FROM keypoints 
    GROUP BY image_id
);

# Images with most features
SELECT i.filename, COUNT(k.id) as kp_count
FROM images i
JOIN keypoints k ON i.id = k.image_id
GROUP BY i.id
ORDER BY kp_count DESC
LIMIT 10;

# Recent images
SELECT filename, datetime(created_at) 
FROM images 
ORDER BY created_at DESC 
LIMIT 5;

# Database size and statistics
.schema
SELECT 
    (SELECT COUNT(*) FROM images) as images,
    (SELECT COUNT(*) FROM keypoints) as keypoints,
    (SELECT COUNT(*) FROM descriptors) as descriptors;
```

## Monitoring

### Check if applications are running
```bash
ps aux | grep -E 'image_generator|feature_extractor|data_logger'
```

### Check network connections
```bash
netstat -tuln | grep -E '5555|5556'
```

### Monitor logs in real-time
```bash
# All logs
tail -f logs/*.log

# Individual logs
tail -f logs/image_generator.log
tail -f logs/feature_extractor.log
tail -f logs/data_logger.log
```

### System resource usage
```bash
top -p $(pgrep -d',' -f 'image_generator|feature_extractor|data_logger')
```

## Troubleshooting

### Ports already in use
```bash
# Find what's using the port
lsof -i :5555
lsof -i :5556

# Kill process using port
kill $(lsof -t -i:5555)
```

### Clean everything and rebuild
```bash
./stop_all.sh
rm -rf build/ logs/ imaging_data.db
./build.sh
```

### Check dependencies
```bash
pkg-config --modversion opencv4
pkg-config --modversion libzmq
pkg-config --modversion sqlite3
```

### Database locked
```bash
# Check if data_logger is running
pgrep -a data_logger

# If hung, force kill
pkill -9 data_logger

# Delete lock if needed
rm imaging_data.db-journal
```

## Performance Tuning

### Adjust Image Generator speed
Edit `src/image_generator/image_publisher.cpp`:
```cpp
// Line ~145 - Adjust sleep duration
std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Change this value
```

### Adjust ZeroMQ buffer sizes
Edit application source files:
```cpp
int sndhwm = 100;  // Send high water mark - increase for more buffering
zmq_setsockopt(publisher_, ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm));
```

### Database optimization
```sql
-- Add this after creating database
PRAGMA synchronous = NORMAL;  -- Faster writes
PRAGMA journal_mode = WAL;    -- Write-ahead logging
PRAGMA cache_size = -64000;   -- 64MB cache
```

## Testing Resilience

### Test 1: Start in random order
```bash
./build/data_logger &
sleep 2
./build/image_generator &
sleep 2
./build/feature_extractor &
```

### Test 2: Kill and restart middle component
```bash
# Kill feature extractor while system running
pkill feature_extractor

# Wait a few seconds
sleep 5

# Restart it
./build/feature_extractor &
# System resumes automatically
```

### Test 3: Simulate data bursts
```bash
# Start data logger and extractor
./build/data_logger &
./build/feature_extractor &

# Wait for them to be ready
sleep 2

# Start generator - it will burst images
./build/image_generator &
```

## Useful One-Liners

```bash
# Count processed images
sqlite3 imaging_data.db "SELECT COUNT(*) FROM images;"

# Get processing rate (images per second)
sqlite3 imaging_data.db "SELECT COUNT(*) * 1.0 / (julianday('now') - julianday(MIN(created_at))) / 86400 FROM images;"

# Delete old data (keep last 24 hours)
sqlite3 imaging_data.db "DELETE FROM images WHERE created_at < datetime('now', '-1 day');"

# Vacuum database to reclaim space
sqlite3 imaging_data.db "VACUUM;"

# Export keypoints to CSV
sqlite3 -header -csv imaging_data.db "SELECT * FROM keypoints LIMIT 100;" > keypoints.csv

# Check log for errors
grep -i error logs/*.log

# Count frames processed
grep "Published frame" logs/image_generator.log | wc -l
```

## Architecture Diagram (ASCII)

```
┌─────────────────┐           ┌──────────────────┐           ┌─────────────────┐
│ Image Generator │           │ Feature Extractor│           │   Data Logger   │
│                 │           │                  │           │                 │
│ ┌─────────────┐ │           │ ┌──────────────┐ │           │ ┌─────────────┐ │
│ │ Read Images │ │           │ │ Receive Data │ │           │ │ Receive Data│ │
│ └──────┬──────┘ │           │ └──────┬───────┘ │           │ └──────┬──────┘ │
│        │        │           │        │         │           │        │        │
│ ┌──────▼──────┐ │  ZMQ PUB  │ ┌──────▼───────┐ │  ZMQ PUB  │ ┌──────▼──────┐ │
│ │ Publish     ├─┼──────────►│ │ SIFT Process │ ├──────────►│ │ SQLite DB   │ │
│ └─────────────┘ │ :5555     │ └──────┬───────┘ │ :5556     │ └─────────────┘ │
│                 │           │        │         │           │                 │
│  Loop Forever   │           │ ┌──────▼───────┐ │           │  Transactional  │
│                 │           │ │ Publish Data │ │           │     Storage     │
└─────────────────┘           │ └──────────────┘ │           └─────────────────┘
                              └──────────────────┘
```

## File Locations

```
/build/                     # Compiled executables
/logs/                      # Runtime logs
imaging_data.db             # SQLite database
/deep_sea_imaging/raw/      # Image source
```

## Environment Variables (Optional)

```bash
# Set default directories
export IMAGE_DIR="./deep_sea_imaging/raw"
export DB_PATH="./imaging_data.db"
export LOG_LEVEL="DEBUG"  # Not implemented yet, but could be added
```

## Git Workflow (if version controlling)

```bash
# Initialize repo
git init
git add .
git commit -m "Initial commit: Distributed Imaging Services"

# Create .gitignore (already included)
# Excludes: build/, logs/, *.db

# Push to GitHub
git remote add origin https://github.com/yourusername/distributed-imaging.git
git push -u origin main
```
