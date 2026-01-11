#ifndef VOICEQWIK_PEER_NETWORK_H
#define VOICEQWIK_PEER_NETWORK_H

#include <utils/Common.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <map>
#include <optional>

struct PeerInfo {
    PeerID id;
    std::string ipAddress;
    uint16_t audioPort;
    bool connected;
    std::chrono::steady_clock::time_point lastHeartbeat;
};

class PeerNetwork {
public:
    static PeerNetwork& GetInstance();

    bool Initialize(int maxParticipants);
    void Shutdown();

    // Server mode (room host)
    bool StartListening(uint16_t port = DEFAULT_AUDIO_PORT);
    void StopListening();

    // Client mode (join room)
    bool ConnectToPeer(const std::string& peerIP, uint16_t port);

    // Connection status
    int GetConnectedPeersCount() const;
    bool IsConnected() const;
    bool IsAllPeersConnected() const;
    const std::vector<PeerInfo>& GetPeers() const;

    // Set expected participant count
    void SetExpectedParticipants(int count);
    int GetExpectedParticipants() const;

private:
    PeerNetwork();
    ~PeerNetwork();

    PeerNetwork(const PeerNetwork&) = delete;
    PeerNetwork& operator=(const PeerNetwork&) = delete;

    int maxParticipants;
    int expectedParticipants;
    std::vector<PeerInfo> peers;
    std::map<SOCKET, PeerID> socketToPeerMap;

    SOCKET listeningSocket;
    std::thread acceptThread;
    std::atomic<bool> listening;

    mutable std::mutex peersMutex;

    void AcceptThreadProc();
    PeerID GeneratePeerID();
    void RemovePeer(PeerID id);
    void CheckPeerHeartbeats();
};

#endif // VOICEQWIK_PEER_NETWORK_H
