# VoiceQwik - Lightweight P2P Voice Chat

A minimal, resource-efficient peer-to-peer voice chat application for Windows, designed for gaming scenarios where Discord's resource usage is excessive.

## Features

- **Direct P2P Connections** - No server required, peer-to-peer audio streaming over UDP
- **Lightweight** - Minimal GUI and limited functionality to reduce CPU/memory usage
- **Low Latency** - Optimized for gaming (target ~50ms latency)
- **Windows Only** - Optimized for Windows (8.1 and later)
- **2-4 Participants** - Perfect for small gaming groups
- **Native C++** - Built entirely in C++ with Windows APIs

## System Requirements

- **OS**: Windows 8.1 or later (64-bit)
- **Audio**: Working microphone and speakers
- **Network**: Stable internet connection with forwarding capability (see below)

## Building VoiceQwik

### Prerequisites

1. **Visual Studio 2022** with C++ Development Tools
2. Windows SDK 10.0 or later
3. Git (optional, for version control)

### Build Steps

1. Open `VoiceQwik.sln` in Visual Studio 2022
2. Select "Release" configuration and "x64" platform
3. Build → Build Solution (Ctrl+Shift+B)
4. The executable will be in `bin\Release\VoiceQwik.exe`

### For Debug Build

1. Select "Debug" configuration
2. Build → Build Solution
3. The executable will be in `bin\Debug\VoiceQwik.exe`

## Usage

### Starting VoiceQwik

1. Run `VoiceQwik.exe`
2. Select the number of participants (2-4)
3. Your connection info will be displayed (IP:Port)

### Joining/Creating a Room

**Option A: Manual IP Exchange (Simplest)**
1. Share your connection info (IP:Port) with peers
2. Each peer enters your IP:Port in "Remote Peer IP:Port" field
3. Click "Connect"
4. Once all expected participants connect, audio streaming begins

**Option B: Local Network**
- If on the same network, use the same process with local IP

### During Call

- **Mute** - Press M or click Mute button to toggle microphone
- **Volume** - Use volume slider to adjust playback level
- **Status** - Check connection status in the Status field

### Ending Call

- Simply close the application or disconnect peers

## Network Configuration

### Port Forwarding (For Remote Peers)

If peers are on different networks, you need **port forwarding**:

1. Log into your router's admin panel
2. Forward port 5000 (or your configured port) to your computer's local IP
3. Share your **public IP** with peers (use whatismyipaddress.com)
4. Peers connect to your public IP:port

### Firewall

Ensure Windows Firewall allows VoiceQwik:
1. Windows Defender Firewall → Allow an app through firewall
2. Add VoiceQwik.exe for both Private and Public networks

## Audio Configuration

### Microphone Selection

Currently uses default microphone. To change:
1. Windows Settings → Sound → Input devices
2. Set desired microphone as default

### Speaker Selection

Currently uses default speakers. To change:
1. Windows Settings → Sound → Output devices
2. Set desired speakers as default

## Troubleshooting

### Application Won't Start
- Check Windows version (requires 8.1+)
- Verify you have audio devices (microphone/speakers)
- Check `VoiceQwik.log` for error messages

### Can't Connect to Peers
- Verify peer IP address is correct
- Check firewall rules for VoiceQwik
- Ensure port 5000 is not blocked by router/ISP
- Test port forwarding with online port checker

### Audio Issues
- Verify microphone/speakers are properly connected
- Check Windows Sound Settings
- Try different audio devices
- Check system volume levels

### High CPU Usage
- Reduce number of participants
- Ensure you have no background processes consuming CPU
- Try disabling other audio applications

## Architecture

```
VoiceQwik/
├── include/
│   ├── audio/
│   │   └── WasapiAudioEngine.h      # WASAPI audio capture/playback
│   ├── networking/
│   │   ├── PeerNetwork.h             # P2P connection management
│   │   └── AudioStreamer.h           # RTP audio streaming
│   ├── gui/
│   │   └── GuiWindow.h               # Minimal Win32 GUI
│   └── utils/
│       ├── Common.h                  # Common definitions
│       └── Logger.h                  # Logging utilities
├── src/
│   ├── main.cpp                      # Main application loop
│   ├── audio/
│   │   └── WasapiAudioEngine.cpp
│   ├── networking/
│   │   ├── PeerNetwork.cpp
│   │   └── AudioStreamer.cpp
│   ├── gui/
│   │   └── GuiWindow.cpp
│   └── utils/
│       └── Logger.cpp
├── VoiceQwik.vcxproj                 # Visual Studio project file
├── VoiceQwik.sln                     # Visual Studio solution
└── README.md
```

## Technical Details

### Audio Processing
- **Sample Rate**: 48 kHz
- **Channels**: Mono (for reduced bandwidth)
- **Bit Depth**: 16-bit PCM
- **Buffer Size**: 10ms
- **Codec**: Uncompressed PCM (minimal CPU overhead)

### Networking
- **Protocol**: TCP for connections, UDP for audio
- **Audio Transport**: RTP (Real-time Transport Protocol)
- **Latency Target**: ~50ms
- **Bandwidth**: ~80 kbps per participant

### Resource Optimization
- **CPU**: Multi-threaded design with blocking audio I/O
- **Memory**: Minimal GUI, small audio buffers
- **Network**: Only active when peers connected, silence detection
- **UI**: Lightweight Win32 API (not .NET or web framework)

## Limitations

- Windows only (for now)
- 2-4 participants maximum (by design for simplicity)
- Manual IP entry for connection
- No server-based features (chat, recording, etc.)
- No encryption (for minimal overhead; add if needed)

## Future Improvements

- [ ] Automatic UPnP port mapping
- [ ] STUN server integration for NAT traversal
- [ ] Opus audio codec for better compression
- [ ] Text chat
- [ ] Voice activity detection (VAD)
- [ ] Audio recording
- [ ] Settings GUI
- [ ] Persistent connection list
- [ ] macOS/Linux support

## License

This project is provided as-is for personal use.

## Contributing

To contribute:
1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## Support

For issues or questions:
1. Check `VoiceQwik.log` for error details
2. Review Troubleshooting section
3. Verify system requirements

---

**VoiceQwik** - Minimal voice communication for gamers
