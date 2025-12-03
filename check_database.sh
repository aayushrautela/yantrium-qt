#!/bin/bash
# Script to check Yantrium database contents

DB_PATH="$HOME/.local/share/Yantrium/yantrium.db"

if [ ! -f "$DB_PATH" ]; then
    echo "Database not found at: $DB_PATH"
    exit 1
fi

echo "=== Database Location ==="
echo "$DB_PATH"
echo ""

echo "=== Watch History Summary ==="
sqlite3 "$DB_PATH" "SELECT type, COUNT(*) as count FROM watch_history GROUP BY type;"
echo ""

echo "=== Total Records ==="
sqlite3 "$DB_PATH" "SELECT COUNT(*) as total FROM watch_history;"
echo ""

echo "=== Recent Watch History (Last 10) ==="
sqlite3 -header -column "$DB_PATH" "SELECT title, type, season, episode, watchedAt FROM watch_history ORDER BY watchedAt DESC LIMIT 10;"
echo ""

echo "=== Sync Tracking ==="
sqlite3 -header -column "$DB_PATH" "SELECT * FROM sync_tracking;"
echo ""

echo "=== Sample Movies (5) ==="
sqlite3 -header -column "$DB_PATH" "SELECT title, year, watchedAt FROM watch_history WHERE type='movie' ORDER BY watchedAt DESC LIMIT 5;"
echo ""

echo "=== Sample TV Episodes (5) ==="
sqlite3 -header -column "$DB_PATH" "SELECT title, season, episode, episodeTitle, watchedAt FROM watch_history WHERE type='tv' ORDER BY watchedAt DESC LIMIT 5;"
echo ""

echo "=== Episodes with Epoch Time (missing watched_at) ==="
sqlite3 -header -column "$DB_PATH" "SELECT title, season, episode, watchedAt FROM watch_history WHERE type='tv' AND watchedAt = '1970-01-01T01:00:00' LIMIT 10;"

