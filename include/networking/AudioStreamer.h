#ifndef VOICEQWIK_AUDIO_STREAMER_H
#define VOICEQWIK_AUDIO_STREAMER_H

#include <utils/Common.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>

// Simple RTP header for audio
struct RTPHeader {
    uint8_t vpxcc;
    uint8_t mpt;
    uint16_t seq;
    uint32_t timestamp;
    uint32_t ssrc;
    uint32_t csrc[15];
};

class AudioStreamer {
public:
    static AudioStreamer& GetInstance();

    bool Initialize();
    void Shutdown();

    // Send audio to peers
    bool SendAudioToPeers(const AudioBuffer& buffer);

    // Receive audio from peer
    bool ReceiveAudioFromPeer(PeerID peerId, AudioBuffer& buffer);

    // Socket management
    bool CreateAudioSocket(uint16_t port);
    void CloseAudioSocket();

private:
    AudioStreamer();
    ~AudioStreamer();

    AudioStreamer(const AudioStreamer&) = delete;
    AudioStreamer& operator=(const AudioStreamer&) = delete;

    SOCKET audioSocket;
    uint16_t audioPort;

    std::thread receiverThread;
    std::atomic<bool> receiving;

    std::map<PeerID, std::queue<AudioBuffer>> receiveQueues;
    std::map<PeerID, sockaddr_in> peerAddresses;
    std::mutex queuesMutex;

    uint16_t rtpSequence;
    uint32_t rtpTimestamp;
    uint32_t rtpSSRC;

    void ReceiverThreadProc();
    void BuildRTPHeader(RTPHeader& header);
};

#endif // VOICEQWIK_AUDIO_STREAMER_H
