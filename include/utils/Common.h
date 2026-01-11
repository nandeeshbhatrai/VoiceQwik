#ifndef VOICEQWIK_COMMON_H
#define VOICEQWIK_COMMON_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

// Windows headers
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mmsystem.h>
#include <audioclient.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

// Application constants
constexpr uint16_t AUDIO_SAMPLE_RATE = 48000;
constexpr uint16_t AUDIO_CHANNELS = 1;  // Mono for lower bandwidth
constexpr uint16_t AUDIO_BITS_PER_SAMPLE = 16;
constexpr uint32_t AUDIO_BUFFER_SIZE = 480;  // 10ms at 48kHz
constexpr uint32_t RTP_PAYLOAD_TYPE = 111;  // Arbitrary for raw audio
constexpr uint16_t DEFAULT_AUDIO_PORT = 5000;

// Max participants
constexpr int MAX_PARTICIPANTS = 4;
constexpr int MIN_PARTICIPANTS = 2;

// Network timeouts (ms)
constexpr int CONNECTION_TIMEOUT = 5000;
constexpr int PEER_TIMEOUT = 10000;

// Typedefs
using AudioBuffer = std::vector<int16_t>;
using PeerID = uint32_t;

// Logging macros
#define LOG_INFO(msg) Logger::GetInstance().Log(LogLevel::INFO, msg)
#define LOG_ERROR(msg) Logger::GetInstance().Log(LogLevel::ERROR, msg)
#define LOG_DEBUG(msg) Logger::GetInstance().Log(LogLevel::DEBUG, msg)
#define LOG_WARNING(msg) Logger::GetInstance().Log(LogLevel::WARNING, msg)

#endif // VOICEQWIK_COMMON_H
