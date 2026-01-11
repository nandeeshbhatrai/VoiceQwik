#include <utils/Common.h>
#include <utils/Logger.h>
#include <audio/WasapiAudioEngine.h>
#include <networking/PeerNetwork.h>
#include <networking/AudioStreamer.h>
#include <gui/GuiWindow.h>
#include <thread>
#include <chrono>

class VoiceQwikApplication {
public:
    bool Initialize(HINSTANCE hInstance) {
        LOG_INFO("=== VoiceQwik Application Starting ===");

        // Initialize logger
        Logger::GetInstance().SetLogFile("VoiceQwik.log");

        // Initialize audio engine
        if (!WasapiAudioEngine::GetInstance().Initialize()) {
            LOG_ERROR("Failed to initialize WASAPI Audio Engine");
            return false;
        }

        // Initialize networking
        if (!PeerNetwork::GetInstance().Initialize(MAX_PARTICIPANTS)) {
            LOG_ERROR("Failed to initialize Peer Network");
            return false;
        }

        // Initialize audio streamer
        if (!AudioStreamer::GetInstance().Initialize()) {
            LOG_ERROR("Failed to initialize Audio Streamer");
            return false;
        }

        // Create GUI
        if (!GuiWindow::GetInstance().Create(hInstance)) {
            LOG_ERROR("Failed to create GUI Window");
            return false;
        }

        // Seed participant expectation from GUI selection (defaults to 2)
        PeerNetwork::GetInstance().SetExpectedParticipants(
            GuiWindow::GetInstance().GetSelectedParticipantCount());

        LOG_INFO("Application initialized successfully");
        return true;
    }

    void Run() {
        LOG_INFO("Application is running");

        GuiWindow::GetInstance().Show();

        // Start WASAPI capture and playback
        WasapiAudioEngine::GetInstance().StartCapture();
        WasapiAudioEngine::GetInstance().StartPlayback();

        // Start listening for incoming connections
        PeerNetwork::GetInstance().StartListening(DEFAULT_AUDIO_PORT);

        // Main application loop
        while (GuiWindow::GetInstance().IsRunning()) {
            GuiWindow::GetInstance().Update();

            // Handle connect requests coming from the UI
            std::string remotePeer;
            if (GuiWindow::GetInstance().TryPopConnectRequest(remotePeer)) {
                std::string ip;
                uint16_t port = DEFAULT_AUDIO_PORT;
                if (ParseHostPort(remotePeer, ip, port)) {
                    GuiWindow::GetInstance().SetConnectionStatus("Connecting to " + ip + ":" + std::to_string(port) + "...");
                    if (PeerNetwork::GetInstance().ConnectToPeer(ip, port)) {
                        GuiWindow::GetInstance().SetConnectionStatus("Connected to " + ip + ":" + std::to_string(port));
                    } else {
                        GuiWindow::GetInstance().SetConnectionStatus("Failed to connect to " + ip + ":" + std::to_string(port));
                    }
                } else {
                    GuiWindow::GetInstance().SetConnectionStatus("Invalid address. Use IP:port");
                }
            }

            // Process audio if connections are ready
            if (PeerNetwork::GetInstance().IsAllPeersConnected()) {
                ProcessAudio();
                GuiWindow::GetInstance().SetConnectionStatus("In call");
            }

            // Small sleep to reduce CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        LOG_INFO("Application main loop ended");
    }

    void Shutdown() {
        LOG_INFO("Shutting down application");

        WasapiAudioEngine::GetInstance().Shutdown();
        PeerNetwork::GetInstance().Shutdown();
        AudioStreamer::GetInstance().Shutdown();

        LOG_INFO("=== VoiceQwik Application Ended ===");
    }

private:
    // Very small helper: parse "ip:port" with default port fallback
    bool ParseHostPort(const std::string& input, std::string& ip, uint16_t& port) {
        if (input.empty()) return false;

        auto colonPos = input.find(':');
        if (colonPos == std::string::npos) {
            ip = input;
        } else {
            ip = input.substr(0, colonPos);
            try {
                int parsedPort = std::stoi(input.substr(colonPos + 1));
                if (parsedPort > 0 && parsedPort < 65536) {
                    port = static_cast<uint16_t>(parsedPort);
                }
            } catch (...) {
                return false;
            }
        }

        sockaddr_in addr{};
        return inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) == 1;
    }

    void ProcessAudio() {
        // Get audio from microphone
        AudioBuffer capturedAudio;
        
        // In a full implementation, would capture from WASAPI here
        // For now, this is a placeholder for the audio processing loop
        
        // Send to peers if we have audio data
        // AudioStreamer::GetInstance().SendAudioToPeers(capturedAudio);
        
        // Receive audio from peers and queue for playback
        auto& peers = PeerNetwork::GetInstance().GetPeers();
        for (const auto& peer : peers) {
            AudioBuffer receivedAudio;
            if (AudioStreamer::GetInstance().ReceiveAudioFromPeer(peer.id, receivedAudio)) {
                if (!GuiWindow::GetInstance().IsMuted()) {
                    WasapiAudioEngine::GetInstance().QueuePlaybackBuffer(receivedAudio);
                }
            }
        }
    }
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    VoiceQwikApplication app;

    if (!app.Initialize(hInstance)) {
        MessageBox(nullptr, L"Failed to initialize VoiceQwik", L"Error", MB_ICONERROR);
        return 1;
    }

    try {
        app.Run();
    } catch (const std::exception& ex) {
        std::string msg = std::string("Application error: ") + ex.what();
        LOG_ERROR(msg);
        MessageBoxA(nullptr, msg.c_str(), "Error", MB_ICONERROR);
    } catch (...) {
        LOG_ERROR("Unknown application error");
        MessageBox(nullptr, L"Unknown application error", L"Error", MB_ICONERROR);
    }

    app.Shutdown();
    return 0;
}
