#!/bin/bash
# Quick test script for addon system
# Usage: ./test_addons.sh

echo "=== Yantrium Addon System Test ==="
echo ""
echo "To test the addon system:"
echo ""
echo "Option 1: Run the test screen directly"
echo "  Modify src/main.cpp line 57 to:"
echo "  const QUrl url(QStringLiteral(\"qrc:/qml/TestLauncher.qml\"));"
echo ""
echo "Option 2: Use the database directly"
echo "  Database location: ~/.local/share/Yantrium/yantrium.db"
echo ""
echo "Option 3: Test via QML console in the app"
echo ""
echo "Example addon URLs to test:"
echo "  - https://v3-cinemeta.strem.io/manifest.json"
echo "  - https://opensubtitles-v3.strem.io/manifest.json"
echo ""
echo "After installing, check database:"
echo "  sqlite3 ~/.local/share/Yantrium/yantrium.db \"SELECT id, name, version, enabled FROM addons;\""
echo ""




