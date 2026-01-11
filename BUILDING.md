# VoiceQwik - Build & Compilation Guide

## Quick Start Build

### Option 1: Build with Visual Studio 2022 IDE (Recommended)

1. **Open the Solution**
   - Navigate to `d:\Developments\VoiceQwik`
   - Double-click `VoiceQwik.sln`
   - Visual Studio 2022 will open

2. **Configure Build**
   - Top toolbar: Select **Release | x64**
   - (Debug | x64 for debug builds with symbols)

3. **Build the Project**
   - Menu: **Build → Build Solution** (or Ctrl+Shift+B)
   - Wait for build to complete
   - Check Output window for any errors

4. **Run the Application**
   - Menu: **Debug → Start Without Debugging** (Ctrl+F5)
   - Or navigate to `bin\Release\VoiceQwik.exe` and double-click

### Option 2: Command Line Build (MSBuild)

```batch
cd d:\Developments\VoiceQwik
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" VoiceQwik.sln /p:Configuration=Release /p:Platform=x64
```

The executable will be at: `bin\Release\VoiceQwik.exe`

### Option 3: Developer Command Prompt

1. Open **Developer Command Prompt for VS 2022**
2. Navigate to project:
   ```
   cd d:\Developments\VoiceQwik
   ```
3. Build:
   ```
   msbuild VoiceQwik.sln /p:Configuration=Release /p:Platform=x64
   ```

## Project Structure

```
VoiceQwik/
├── include/                          # Header files (.h)
│   ├── audio/
│   │   └── WasapiAudioEngine.h
│   ├── gui/
│   │   └── GuiWindow.h
│   ├── networking/
│   │   ├── AudioStreamer.h
│   │   └── PeerNetwork.h
│   └── utils/
│       ├── Common.h
│       └── Logger.h
│
├── src/                              # Source files (.cpp)
│   ├── main.cpp                      # Entry point
│   ├── audio/
│   │   └── WasapiAudioEngine.cpp
│   ├── gui/
│   │   └── GuiWindow.cpp
│   ├── networking/
│   │   ├── AudioStreamer.cpp
│   │   └── PeerNetwork.cpp
│   └── utils/
│       └── Logger.cpp
│
├── bin/                              # Output binaries (generated)
│   ├── Debug/
│   │   └── VoiceQwik.exe
│   └── Release/
│       └── VoiceQwik.exe
│
├── build/                            # Intermediate files (generated)
│   ├── Debug/
│   └── Release/
│
├── VoiceQwik.sln                     # Visual Studio Solution
├── VoiceQwik.vcxproj                 # Visual Studio Project
├── README.md                         # Usage guide
└── BUILDING.md                       # This file
```

## Dependencies

All dependencies are part of the Windows SDK:
- **WinSock2** (ws2_32.lib) - Network sockets
- **WASAPI** (mmdevapi.lib, winmm.lib) - Audio capture/playback
- **Win32 API** (user32.lib, gdi32.lib) - GUI components
- **Windows Multimedia** (winmm.lib) - Low-level audio
- **IP Helper** (iphlpapi.lib) - Network utilities

**Note**: No external third-party libraries required! Everything is built-in Windows APIs.

## Build Configurations

### Release Build
- **Optimization**: O2 (Maximum Speed)
- **Debug Info**: None
- **Size**: Smallest
- **Performance**: Best
- **Use for**: Production/shipping

### Debug Build
- **Optimization**: None
- **Debug Info**: Full symbols
- **Size**: Larger
- **Performance**: Slower
- **Use for**: Development/debugging

## System Requirements for Building

- **OS**: Windows 10 or later (to develop for 8.1+)
- **Visual Studio**: 2022 (Community, Professional, or Enterprise)
- **Windows SDK**: 10.0 or higher
- **C++ Workload**: Required in VS installer

### Installing Visual Studio 2022

1. Download from: https://visualstudio.microsoft.com/vs/
2. Run installer
3. Select **Desktop development with C++** workload
4. Ensure **Windows 10 SDK** is selected
5. Click Install
6. Wait for completion

## Troubleshooting Build Issues

### Issue: "Cannot open include file"
**Solution**: 
- Verify all header files exist in `include/` directory
- Check `VoiceQwik.vcxproj` - Additional Include Directories should be: `$(ProjectDir)include`
- Verify header includes use correct paths with forward slashes

### Issue: "Unresolved external symbol"
**Solution**:
- Check all `.cpp` files are included in VoiceQwik.vcxproj
- Verify Windows library linking: ws2_32.lib, mmdevapi.lib, user32.lib, gdi32.lib, winmm.lib

### Issue: "LNK2001 unresolved external"
**Solution**:
- Open VoiceQwik.vcxproj in text editor
- Verify all 6 source .cpp files are listed under `<ItemGroup><ClCompile>`
- Check library names in Link section

### Issue: WASAPI compilation errors
**Solution**:
- Ensure `#include <audioclient.h>` and related WASAPI headers are present
- These are part of Windows SDK, verify SDK installation
- May need to update Windows SDK version

### Issue: "The project file contains invalid XML"
**Solution**:
- Open VoiceQwik.vcxproj in text editor
- Look for XML syntax errors (mismatched tags, special characters)
- Verify all quotes and brackets are matched

## Building from Scratch (If Project Corrupted)

If the project files get corrupted:

1. **Verify source files exist**:
   ```
   dir /s src\*.cpp
   dir /s include\*.h
   ```

2. **Create new Visual Studio project** (if needed):
   - File → New → Project → C++ Console Application
   - Configure as described in architecture

3. **Add existing files**:
   - Project → Add Existing Item → Select .cpp and .h files

4. **Set include path**:
   - Project → Properties → C/C++ → General
   - Additional Include Directories: `$(ProjectDir)include`

5. **Link libraries**:
   - Project → Properties → Linker → Input
   - Additional Dependencies: 
     ```
     ws2_32.lib;mmdevapi.lib;user32.lib;gdi32.lib;winmm.lib;iphlpapi.lib;dwmapi.lib;comdlg32.lib
     ```

## Performance Optimization Flags

For absolute minimum resource usage, add to Release configuration:

In `VoiceQwik.vcxproj`, under Release configuration `<ClCompile>`:
```xml
<Optimization>MaxSpeed</Optimization>
<InlineFunctionExpansion>AnySuitable</InlineFunctionExpansion>
<FavorSizeOrSpeed>Speed</FavorSizeOrSpeed>
<OmitFramePointers>true</OmitFramePointers>
```

## Creating Standalone Executable

After building Release:

1. Navigate to: `bin\Release\`
2. Copy `VoiceQwik.exe` to desired location
3. No additional DLLs needed (all dependencies are system DLLs)
4. Can run on any Windows 8.1+ system with Visual C++ Runtime

### Distributing VoiceQwik

To create an installer or distribution package:

1. **Copy executable**: `bin\Release\VoiceQwik.exe`
2. **Create shortcut** (for Start Menu/Desktop)
3. **Package with installer** (NSIS, WiX, or InnoSetup)
4. **Include README** and usage instructions
5. **Test on clean Windows VM** to verify compatibility

## Testing the Build

After successful build:

```batch
# Navigate to output directory
cd d:\Developments\VoiceQwik\bin\Release

# Run executable
VoiceQwik.exe

# Expected: Application window opens with GUI controls
```

Check `VoiceQwik.log` in the same directory as exe for any runtime errors.

## Next Steps

1. **Build and run** VoiceQwik.exe
2. **Select participant count** (2-4 people)
3. **Share connection info** with peers
4. **Connect to peers** by entering their IP:Port
5. **Start talking** once all peers connected!

---

For additional help, see README.md or check VoiceQwik.log for errors.
