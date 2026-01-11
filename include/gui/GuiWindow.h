#ifndef VOICEQWIK_GUI_WINDOW_H
#define VOICEQWIK_GUI_WINDOW_H

#include <utils/Common.h>
#include <windows.h>
#include <string>
#include <mutex>

class GuiWindow {
public:
    static GuiWindow& GetInstance();

    bool Create(HINSTANCE hInstance);
    void Show();
    void Hide();
    void Update();
    bool IsRunning() const;

    // Getters
    int GetSelectedParticipantCount() const;
    bool IsMuted() const;
    std::string GetConnectionString() const;
    std::string GetRemotePeerIP() const;
    bool TryPopConnectRequest(std::string& remotePeer);

    // Setters
    void SetConnectionStatus(const std::string& status);
    void SetParticipantCount(int count);
    void SetMuted(bool muted);

private:
    GuiWindow();
    ~GuiWindow();

    GuiWindow(const GuiWindow&) = delete;
    GuiWindow& operator=(const GuiWindow&) = delete;

    HWND hwnd;
    HINSTANCE hInstance;
    std::atomic<bool> running;

    // Control handles
    HWND participantCombo;
    HWND statusText;
    HWND connectionInfoEdit;
    HWND remotePeerEdit;
    HWND connectButton;
    HWND muteButton;
    HWND volumeSlider;

    int selectedParticipants;
    bool isMuted;

    std::atomic<bool> connectRequested;
    std::string requestedPeer;
    mutable std::mutex connectMutex;

    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void UpdateConnectionInfo();
};

#endif // VOICEQWIK_GUI_WINDOW_H
