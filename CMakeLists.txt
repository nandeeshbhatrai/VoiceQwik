cmake_minimum_required(VERSION 3.15)
project(VoiceQwik VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform-specific settings
if(NOT WIN32)
    message(FATAL_ERROR "VoiceQwik only supports Windows at this time")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(VOICEQWIK_SOURCES
    src/main.cpp
    src/audio/WasapiAudioEngine.cpp
    src/networking/PeerNetwork.cpp
    src/networking/AudioStreamer.cpp
    src/gui/GuiWindow.cpp
    src/utils/Logger.cpp
)

set(VOICEQWIK_HEADERS
    include/audio/WasapiAudioEngine.h
    include/networking/PeerNetwork.h
    include/networking/AudioStreamer.h
    include/gui/GuiWindow.h
    include/utils/Logger.h
    include/utils/Common.h
)

# Create executable
add_executable(VoiceQwik ${VOICEQWIK_SOURCES} ${VOICEQWIK_HEADERS})

# Windows libraries
target_link_libraries(VoiceQwik
    ws2_32           # Winsock2
    user32           # Windows GUI
    gdi32            # Graphics Device Interface
    comdlg32         # Common dialogs
    mmsystem         # Multimedia
    mmdevapi         # WASAPI
    winmm            # Windows Multimedia
    iphlpapi         # IP Helper
    dwmapi           # Desktop Window Manager
)

# Compiler options for optimization
if(MSVC)
    # Release build optimizations
    target_compile_options(VoiceQwik PRIVATE
        $<$<CONFIG:Release>:/O2 /GL /Gy>
    )
    target_link_options(VoiceQwik PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
    
    # Disable warnings for Windows API usage
    target_compile_definitions(VoiceQwik PRIVATE
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        UNICODE
        _UNICODE
    )
endif()

# Set up subsystem for GUI (Windows, not Console)
if(MSVC)
    set_target_properties(VoiceQwik PROPERTIES
        WIN32_EXECUTABLE $<$<CONFIG:Release>:ON>
    )
endif()
