#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>

#include <algorithm>

#include <networking/PeerNetwork.h>
#include <utils/Logger.h>


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

PeerNetwork& PeerNetwork::GetInstance() {
    static PeerNetwork instance;
    return instance;
}

PeerNetwork::PeerNetwork()
    : maxParticipants(MAX_PARTICIPANTS), expectedParticipants(2),
      listeningSocket(INVALID_SOCKET), listening(false) {
}

PeerNetwork::~PeerNetwork() {
    Shutdown();
}

bool PeerNetwork::Initialize(int maxPeers) {
    LOG_INFO("Initializing Peer Network");

    maxParticipants = std::min(maxPeers, MAX_PARTICIPANTS);
    maxParticipants = std::max(maxParticipants, MIN_PARTICIPANTS);

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        LOG_ERROR("WSAStartup failed: " + std::to_string(result));
        return false;
    }

    LOG_INFO("Peer Network initialized successfully");
    return true;
}

void PeerNetwork::Shutdown() {
    LOG_INFO("Shutting down Peer Network");

    StopListening();

    {
        std::lock_guard<std::mutex> lock(peersMutex);
        peers.clear();
        socketToPeerMap.clear();
    }

    WSACleanup();
}

bool PeerNetwork::StartListening(uint16_t port) {
    if (listening) return true;

    LOG_INFO("Starting to listen for peer connections on port " + std::to_string(port));

    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listeningSocket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create listening socket");
        return false;
    }

    // Set socket to non-blocking
    u_long nonBlocking = 1;
    if (ioctlsocket(listeningSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        LOG_ERROR("Failed to set socket to non-blocking");
        closesocket(listeningSocket);
        return false;
    }

    // Allow address reuse
    int reuseAddr = 1;
    if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&reuseAddr, sizeof(reuseAddr)) == SOCKET_ERROR) {
        LOG_ERROR("Failed to set SO_REUSEADDR");
        closesocket(listeningSocket);
        return false;
    }

    // Bind socket
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listeningSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        LOG_ERROR("Failed to bind listening socket");
        closesocket(listeningSocket);
        return false;
    }

    // Listen
    if (listen(listeningSocket, maxParticipants - 1) == SOCKET_ERROR) {
        LOG_ERROR("Failed to listen on socket");
        closesocket(listeningSocket);
        return false;
    }

    listening = true;
    acceptThread = std::thread(&PeerNetwork::AcceptThreadProc, this);

    LOG_INFO("Listening started successfully");
    return true;
}

void PeerNetwork::StopListening() {
    if (!listening) return;

    listening = false;

    if (acceptThread.joinable()) {
        acceptThread.join();
    }

    if (listeningSocket != INVALID_SOCKET) {
        closesocket(listeningSocket);
        listeningSocket = INVALID_SOCKET;
    }

    LOG_INFO("Listening stopped");
}

bool PeerNetwork::ConnectToPeer(const std::string& peerIP, uint16_t port) {
    LOG_INFO("Attempting to connect to peer: " + peerIP + ":" + std::to_string(port));

    SOCKET peerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (peerSocket == INVALID_SOCKET) {
        LOG_ERROR("Failed to create peer socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, peerIP.c_str(), &addr.sin_addr);

    if (connect(peerSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        int error = WSAGetLastError();
        LOG_ERROR("Failed to connect to peer: " + std::to_string(error));
        closesocket(peerSocket);
        return false;
    }

    PeerID peerId = GeneratePeerID();

    {
        std::lock_guard<std::mutex> lock(peersMutex);
        PeerInfo peerInfo{};
        peerInfo.id = peerId;
        peerInfo.ipAddress = peerIP;
        peerInfo.audioPort = port;
        peerInfo.connected = true;
        peerInfo.lastHeartbeat = std::chrono::steady_clock::now();
        peers.push_back(peerInfo);
        socketToPeerMap[peerSocket] = peerId;
    }

    LOG_INFO("Connected to peer " + std::to_string(peerId));
    return true;
}

int PeerNetwork::GetConnectedPeersCount() const {
    std::lock_guard<std::mutex> lock(peersMutex);
    int count = 0;
    for (const auto& peer : peers) {
        if (peer.connected) count++;
    }
    return count;
}

bool PeerNetwork::IsConnected() const {
    std::lock_guard<std::mutex> lock(peersMutex);
    return !peers.empty() && peers[0].connected;
}

bool PeerNetwork::IsAllPeersConnected() const {
    std::lock_guard<std::mutex> lock(peersMutex);
    if (peers.size() != expectedParticipants - 1) return false;
    
    for (const auto& peer : peers) {
        if (!peer.connected) return false;
    }
    return true;
}

const std::vector<PeerInfo>& PeerNetwork::GetPeers() const {
    return peers;
}

void PeerNetwork::SetExpectedParticipants(int count) {
    expectedParticipants = std::min(count, MAX_PARTICIPANTS);
    expectedParticipants = std::max(expectedParticipants, MIN_PARTICIPANTS);
    LOG_INFO("Expected participants set to: " + std::to_string(expectedParticipants));
}

int PeerNetwork::GetExpectedParticipants() const {
    return expectedParticipants;
}

void PeerNetwork::AcceptThreadProc() {
    while (listening) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(listeningSocket, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            Sleep(100);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(peersMutex);

            // Check if we've reached the participant limit
            if (peers.size() >= (size_t)(expectedParticipants - 1)) {
                LOG_WARNING("Maximum participants reached, rejecting connection");
                closesocket(clientSocket);
                continue;
            }

            PeerID peerId = GeneratePeerID();
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, ip_str, INET_ADDRSTRLEN);

            PeerInfo peerInfo{};
            peerInfo.id = peerId;
            peerInfo.ipAddress = std::string(ip_str);
            peerInfo.audioPort = ntohs(clientAddr.sin_port);
            peerInfo.connected = true;
            peerInfo.lastHeartbeat = std::chrono::steady_clock::now();

            peers.push_back(peerInfo);
            socketToPeerMap[clientSocket] = peerId;

            LOG_INFO("Accepted connection from peer " + std::to_string(peerId) +
                    " at " + peerInfo.ipAddress);
        }
    }
}

PeerID PeerNetwork::GeneratePeerID() {
    static PeerID nextId = 1;
    return nextId++;
}

void PeerNetwork::RemovePeer(PeerID id) {
    std::lock_guard<std::mutex> lock(peersMutex);
    auto it = std::find_if(peers.begin(), peers.end(),
                          [id](const PeerInfo& p) { return p.id == id; });
    if (it != peers.end()) {
        LOG_INFO("Removing peer " + std::to_string(id));
        peers.erase(it);
    }
}

void PeerNetwork::CheckPeerHeartbeats() {
    auto now = std::chrono::steady_clock::now();
    std::vector<PeerID> deadPeers;

    {
        std::lock_guard<std::mutex> lock(peersMutex);
        for (auto& peer : peers) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - peer.lastHeartbeat).count();
            if (elapsed > PEER_TIMEOUT) {
                deadPeers.push_back(peer.id);
            }
        }
    }

    for (PeerID id : deadPeers) {
        RemovePeer(id);
    }
}
