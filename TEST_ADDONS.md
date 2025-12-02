# Testing the Addon System

## Quick Start

### Option 1: Use the Test Screen (Recommended)

1. **Temporarily modify `src/main.cpp`** to load the test screen:
   - Find line 57: `const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));`
   - Change it to: `const QUrl url(QStringLiteral("qrc:/qml/TestLauncher.qml"));`

2. **Rebuild and run:**
   ```bash
   cd build && ninja && ./Yantrium
   ```

3. **Test the addon system:**
   - Enter a manifest URL (or use quick install buttons)
   - Click "Install" to install an addon
   - Watch the status messages for success/errors
   - Use "Refresh Count" to see how many addons are installed

### Option 2: Test via Database

After running the app and installing addons, check the database:

```bash
# View all addons
sqlite3 ~/.local/share/Yantrium/yantrium.db "SELECT id, name, version, enabled FROM addons;"

# View full addon details
sqlite3 ~/.local/share/Yantrium/yantrium.db "SELECT * FROM addons;"
```

## Example Addon URLs

### Working Test Addons:

1. **Cinemeta** (Metadata provider):
   ```
   https://v3-cinemeta.strem.io/manifest.json
   ```

2. **OpenSubtitles** (Subtitle provider):
   ```
   https://opensubtitles-v3.strem.io/manifest.json
   ```

3. **Torrentio** (Stream provider - may require configuration):
   ```
   https://torrentio.strem.fun/manifest.json
   ```

## What to Test

1. **Installation:**
   - Enter a manifest URL
   - Click "Install"
   - Should see "✓ Installed: [name]" message
   - Addon count should increase

2. **Database Verification:**
   - Check that addon appears in database
   - Verify all fields are populated correctly

3. **Error Handling:**
   - Try an invalid URL
   - Should see error message

4. **Operations** (via database or future UI):
   - Enable/disable addons
   - Update addons (refresh manifest)
   - Remove addons

## Expected Console Output

When installing an addon, you should see:
```
[MAIN] Database initialized successfully
[AddonRepository] Installing addon from: https://...
[AddonClient] Fetching manifest...
[AddonInstaller] Addon installed: [name]
```

## Troubleshooting

- **Database errors:** Check that SQLite is available: `sqlite3 --version`
- **Network errors:** Check internet connection and addon URL validity
- **Build errors:** Make sure Qt SQL module is installed: `qt6-qtbase-sql`
- **QML errors:** Check console for QML warnings

## Next Steps

After verifying the core functionality works:
1. Create proper QML models for addon list display
2. Add UI for enable/disable/remove operations
3. Test catalog, meta, and stream fetching
4. Integrate with the library/stream service




