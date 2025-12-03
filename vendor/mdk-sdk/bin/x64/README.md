# Windows MDK SDK Binary Files

Place the following Windows DLL files in this directory:

- `mdk.dll` - Main MDK library DLL
- Any other required MDK DLLs (e.g., `mdk-braw.dll`, `mdk-r3d.dll`, etc.)

The CMakeLists.txt will automatically copy these DLLs to the build output directory during the build process.

