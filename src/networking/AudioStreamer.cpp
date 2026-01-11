#include <networking/AudioStreamer.h>
#include <networking/PeerNetwork.h>
#include <utils/Logger.h>
#include <cstring>
#include <ctime>

AudioStreamer& AudioStreamer::GetInstance() {
    static AudioStreamer instance;
    return instance;
}

AudioStreamer::AudioStreamer()
    : audioSocket(INVALID_SOCKET), audioPort(DEFAULT_AUDIO_PORT),
      receiving(false), rtpSequence(0), rtpTimestamp(0) {
    
    // Generate random SSRC
    srand((unsigned int)time(nullptr));
    rtpSSRC = rand();
}

AudioStreamer::~AudioStreamer() {
    Shutdown();
}

bool AudioStreamer::Initialize() {
    LOG_INFO("Initializing Audio Streamer");

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        LOG_ERROR("WSAStartup failed for AudioStreamer: " + std::to_string(result));
        return false;
    }

    if (!CreateAudioSocket(DEFAULT_AUDIO_PORT)) {
        WSACleanup();
        return false;
    }

    // Start receiving loop on a non-blocking socket so shutdown stays responsive
    receiving = true;
    receiverThread = std::thread(&AudioStreamer::ReceiverThreadProc, this);

    LOG_INFO("Audio Streamer initialized successfully");
    return true;
}

void AudioStreamer::Shutdown() {
    LOG_INFO("Shutting down Audio Streamer");

    receiving = false;

    if (receiverThread.joinable()) {
        receiverThread.join();
    }

    CloseAudioSocket();
    WSACleanup();
}

bool AudioStreamer::CreateAudioSocket(uint16_t port) {
    audioSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (audioSocket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create audio socket");
        return false;
    }

    // Non-blocking so the receive loop can exit promptly on shutdown
    u_long nonBlocking = 1;
    if (ioctlsocket(audioSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        LOG_ERROR("Failed to set audio socket to non-blocking");
        closesocket(audioSocket);
        audioSocket = INVALID_SOCKET;
        return false;
    }

    // Set socket to reuse address
    int reuseAddr = 1;
    if (setsockopt(audioSocket, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&reuseAddr, sizeof(reuseAddr)) == SOCKET_ERROR) {
        LOG_ERROR("Failed to set SO_REUSEADDR on audio socket");
        closesocket(audioSocket);
        audioSocket = INVALID_SOCKET;
        return false;
    }

    // Set socket buffer sizes for low latency
    int recvBufSize = 128 * 1024;  // 128KB
    int sendBufSize = 128 * 1024;
    setsockopt(audioSocket, SOL_SOCKET, SO_RCVBUF,
               (const char*)&recvBufSize, sizeof(recvBufSize));
    setsockopt(audioSocket, SOL_SOCKET, SO_SNDBUF,
               (const char*)&sendBufSize, sizeof(sendBufSize));

    // Bind socket
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(audioSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        LOG_ERROR("Failed to bind audio socket");
        closesocket(audioSocket);
        audioSocket = INVALID_SOCKET;
        return false;
    }

    audioPort = port;

    LOG_INFO("Audio socket created and bound to port " + std::to_string(port));
    return true;
}

void AudioStreamer::CloseAudioSocket() {
    if (audioSocket != INVALID_SOCKET) {
        closesocket(audioSocket);
        audioSocket = INVALID_SOCKET;
    }
}

bool AudioStreamer::SendAudioToPeers(const AudioBuffer& buffer) {
    if (audioSocket == INVALID_SOCKET) {
        return false;
    }

    // Build RTP packet
    RTPHeader header{};
    BuildRTPHeader(header);

    // Construct packet: RTP header + audio data
    size_t packetSize = sizeof(header) + buffer.size() * sizeof(int16_t);
    uint8_t* packet = new uint8_t[packetSize];

    std::memcpy(packet, &header, sizeof(header));
    std::memcpy(packet + sizeof(header), buffer.data(), buffer.size() * sizeof(int16_t));

    // Send to all connected peers
    const auto& peers = PeerNetwork::GetInstance().GetPeers();
    for (const auto& peer : peers) {
        sockaddr_in peerAddr{};
        peerAddr.sin_family = AF_INET;
        peerAddr.sin_port = htons(peer.audioPort);
        inet_pton(AF_INET, peer.ipAddress.c_str(), &peerAddr.sin_addr);

        int result = sendto(audioSocket, (const char*)packet, (int)packetSize, 0,
                          (const sockaddr*)&peerAddr, sizeof(peerAddr));

        if (result == SOCKET_ERROR) {
            int error = WSAGetLastError();
            LOG_ERROR("Failed to send audio to peer " + std::to_string(peer.id) +
                     ": " + std::to_string(error));
        }
    }

    delete[] packet;
    return true;
}

bool AudioStreamer::ReceiveAudioFromPeer(PeerID peerId, AudioBuffer& buffer) {
    std::lock_guard<std::mutex> lock(queuesMutex);
    auto it = receiveQueues.find(peerId);
    if (it == receiveQueues.end() || it->second.empty()) {
        return false;
    }

    buffer = it->second.front();
    it->second.pop();
    return true;
}

void AudioStreamer::ReceiverThreadProc() {
    const size_t MAX_PACKET_SIZE = sizeof(RTPHeader) + AUDIO_BUFFER_SIZE * sizeof(int16_t) * 4;
    uint8_t* recvBuffer = new uint8_t[MAX_PACKET_SIZE];

    while (receiving) {
        sockaddr_in senderAddr{};
        int senderAddrLen = sizeof(senderAddr);

        int bytesReceived = recvfrom(audioSocket, (char*)recvBuffer, (int)MAX_PACKET_SIZE, 0,
                                    (sockaddr*)&senderAddr, &senderAddrLen);

        if (bytesReceived == SOCKET_ERROR) {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                LOG_ERROR("recvfrom failed: " + std::to_string(error));
            }
            Sleep(1);
            continue;
        }

        if (bytesReceived > (int)sizeof(RTPHeader)) {
            // Extract audio data (skip RTP header)
            size_t audioDataSize = bytesReceived - sizeof(RTPHeader);
            int16_t* audioData = (int16_t*)(recvBuffer + sizeof(RTPHeader));

            // Get sender's peer ID (would need to match IP)
            char senderIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &senderAddr.sin_addr, senderIP, INET_ADDRSTRLEN);

            // Find peer by IP
            const auto& peers = PeerNetwork::GetInstance().GetPeers();
            PeerID senderId = 0;
            for (const auto& peer : peers) {
                if (peer.ipAddress == senderIP) {
                    senderId = peer.id;
                    break;
                }
            }

            if (senderId > 0) {
                std::lock_guard<std::mutex> lock(queuesMutex);
                AudioBuffer audioBuffer(audioData, audioData + audioDataSize / sizeof(int16_t));
                receiveQueues[senderId].push(audioBuffer);
            }
        }
    }

    delete[] recvBuffer;
}

void AudioStreamer::BuildRTPHeader(RTPHeader& header) {
    // V=2, P=0, X=0, CC=0
    header.vpxcc = 0x80;
    
    // M=0, PT=111
    header.mpt = RTP_PAYLOAD_TYPE & 0x7F;
    
    header.seq = htons(++rtpSequence);
    header.timestamp = htonl(rtpTimestamp);
    header.ssrc = htonl(rtpSSRC);
    
    rtpTimestamp += AUDIO_BUFFER_SIZE;
}
