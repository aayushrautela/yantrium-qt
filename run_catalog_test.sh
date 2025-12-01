#!/bin/bash

# Simple script to run the QML catalog test

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=== Running QML Catalog Test ==="
echo ""
echo "This will open a window where you can:"
echo "1. Click 'Load Catalogs' to fetch catalog data"
echo "2. View the raw JSON data in the text area"
echo "3. Click 'Save to File' to export it"
echo ""

# Build the app first
if [ ! -f "build/Yantrium" ]; then
    echo "Building application..."
    ./build.sh
fi

# Run the QML test using qmlscene or the app
if command -v qmlscene &> /dev/null; then
    echo "Running with qmlscene..."
    qmlscene test_catalogs.qml
elif [ -f "build/Yantrium" ]; then
    echo "Note: To run the QML test, you can:"
    echo "1. Temporarily change MainApp.qml to load test_catalogs.qml"
    echo "2. Or use qmlscene if available: qmlscene test_catalogs.qml"
    echo ""
    echo "Alternatively, run the C++ test: ./test_catalogs.sh"
else
    echo "ERROR: Could not find qmlscene or built application"
    exit 1
fi

