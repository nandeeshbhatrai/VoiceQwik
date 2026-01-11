#include <gui/GuiWindow.h>
#include <utils/Logger.h>
#include <networking/PeerNetwork.h>
#include <sstream>
#include <commctrl.h>

// Window class name
static const wchar_t CLASS_NAME[] = L"VoiceQwikWindow";

// Control IDs
enum ControlID {
    IDC_PARTICIPANT_COMBO = 1000,
    IDC_STATUS_TEXT = 1001,
    IDC_CONNECTION_INFO = 1002,
    IDC_REMOTE_PEER_EDIT = 1003,
    IDC_CONNECT_BUTTON = 1004,
    IDC_MUTE_BUTTON = 1005,
    IDC_VOLUME_SLIDER = 1006
};

GuiWindow& GuiWindow::GetInstance() {
    static GuiWindow instance;
    return instance;
}

GuiWindow::GuiWindow()
    : hwnd(nullptr), hInstance(nullptr), running(false),
      participantCombo(nullptr), statusText(nullptr), connectionInfoEdit(nullptr),
      remotePeerEdit(nullptr), connectButton(nullptr), muteButton(nullptr),
      volumeSlider(nullptr), selectedParticipants(2), isMuted(false) {
}

GuiWindow::~GuiWindow() {
    if (hwnd) {
        DestroyWindow(hwnd);
    }
}

bool GuiWindow::Create(HINSTANCE instance) {
    LOG_INFO("Creating GUI Window");

    hInstance = instance;

    // Register window class
    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        LOG_ERROR("Failed to register window class");
        return false;
    }

    // Create window
    hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"VoiceQwik - Lightweight P2P Voice Chat",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 500,
        nullptr, nullptr,
        hInstance, this
    );

    if (!hwnd) {
        LOG_ERROR("Failed to create window");
        return false;
    }

    CreateControls();
    running = true;

    LOG_INFO("GUI Window created successfully");
    return true;
}

void GuiWindow::Show() {
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}

void GuiWindow::Hide() {
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }
}

void GuiWindow::Update() {
    if (!hwnd) return;

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT) {
            running = false;
        }
    }
}

bool GuiWindow::IsRunning() const {
    return running;
}

int GuiWindow::GetSelectedParticipantCount() const {
    return selectedParticipants;
}

bool GuiWindow::IsMuted() const {
    return isMuted;
}

std::string GuiWindow::GetConnectionString() const {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        return "0.0.0.0:5000";
    }

    addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) {
        return "0.0.0.0:5000";
    }

    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);

    char ipStr[INET_ADDRSTRLEN];
    InetNtopA(AF_INET, &addr->sin_addr, ipStr, INET_ADDRSTRLEN);

    freeaddrinfo(result);

    return std::string(ipStr) + ":5000";
}

std::string GuiWindow::GetRemotePeerIP() const {
    if (!remotePeerEdit) return "";

    wchar_t buffer[256];
    GetWindowText(remotePeerEdit, buffer, sizeof(buffer) / sizeof(buffer[0]));

    // Convert wide string to regular string
    char narrowBuffer[256];
    WideCharToMultiByte(CP_ACP, 0, buffer, -1, narrowBuffer, sizeof(narrowBuffer), nullptr, nullptr);

    return std::string(narrowBuffer);
}

void GuiWindow::SetConnectionStatus(const std::string& status) {
    if (!statusText) return;

    wchar_t wstatus[512];
    MultiByteToWideChar(CP_ACP, 0, status.c_str(), -1, wstatus, sizeof(wstatus) / sizeof(wstatus[0]));
    SetWindowText(statusText, wstatus);
}

void GuiWindow::SetParticipantCount(int count) {
    selectedParticipants = count;
    LOG_INFO("Participant count set to: " + std::to_string(count));
}

void GuiWindow::SetMuted(bool muted) {
    isMuted = muted;
    LOG_INFO("Mute set to: " + std::string(muted ? "true" : "false"));
}

void GuiWindow::CreateControls() {
    // Font for controls
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    int xOffset = 20;
    int yOffset = 20;
    int controlWidth = 200;
    int controlHeight = 25;
    int lineHeight = 35;

    // Label: Select participants
    CreateWindow(L"STATIC", L"Number of Participants:",
                WS_CHILD | WS_VISIBLE, xOffset, yOffset, controlWidth, controlHeight,
                hwnd, (HMENU)0, hInstance, nullptr);

    // Combo box: 2-4 participants
    participantCombo = CreateWindow(L"COMBOBOX", L"",
                                   WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                                   xOffset + controlWidth + 10, yOffset, 100, 100,
                                   hwnd, (HMENU)IDC_PARTICIPANT_COMBO, hInstance, nullptr);

    SendMessage(participantCombo, CB_ADDSTRING, 0, (LPARAM)L"2 People");
    SendMessage(participantCombo, CB_ADDSTRING, 0, (LPARAM)L"3 People");
    SendMessage(participantCombo, CB_ADDSTRING, 0, (LPARAM)L"4 People");
    SendMessage(participantCombo, CB_SETCURSEL, 0, 0);

    yOffset += lineHeight;

    // Label: Connection info
    CreateWindow(L"STATIC", L"Your Connection Info:",
                WS_CHILD | WS_VISIBLE, xOffset, yOffset, controlWidth, controlHeight,
                hwnd, (HMENU)0, hInstance, nullptr);

    connectionInfoEdit = CreateWindow(L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY,
                                     xOffset, yOffset + 25, 400, controlHeight,
                                     hwnd, (HMENU)IDC_CONNECTION_INFO, hInstance, nullptr);
    UpdateConnectionInfo();

    yOffset += lineHeight + 25;

    // Label: Remote peer IP
    CreateWindow(L"STATIC", L"Remote Peer IP:Port:",
                WS_CHILD | WS_VISIBLE, xOffset, yOffset, controlWidth, controlHeight,
                hwnd, (HMENU)0, hInstance, nullptr);

    remotePeerEdit = CreateWindow(L"EDIT", L"",
                                 WS_CHILD | WS_VISIBLE | WS_BORDER,
                                 xOffset, yOffset + 25, 400, controlHeight,
                                 hwnd, (HMENU)IDC_REMOTE_PEER_EDIT, hInstance, nullptr);

    yOffset += lineHeight + 25;

    // Connect button
    connectButton = CreateWindow(L"BUTTON", L"Connect",
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                xOffset, yOffset, 100, controlHeight,
                                hwnd, (HMENU)IDC_CONNECT_BUTTON, hInstance, nullptr);

    yOffset += lineHeight;

    // Status text
    CreateWindow(L"STATIC", L"Status:",
                WS_CHILD | WS_VISIBLE, xOffset, yOffset, controlWidth, controlHeight,
                hwnd, (HMENU)0, hInstance, nullptr);

    statusText = CreateWindow(L"STATIC", L"Idle",
                             WS_CHILD | WS_VISIBLE | SS_LEFT,
                             xOffset, yOffset + 25, 400, controlHeight,
                             hwnd, (HMENU)IDC_STATUS_TEXT, hInstance, nullptr);

    yOffset += lineHeight + 25;

    // Mute button
    muteButton = CreateWindow(L"BUTTON", L"Mute (M)",
                             WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                             xOffset, yOffset, 100, controlHeight,
                             hwnd, (HMENU)IDC_MUTE_BUTTON, hInstance, nullptr);

    // Volume label and slider
    CreateWindow(L"STATIC", L"Volume:",
                WS_CHILD | WS_VISIBLE, xOffset + 120, yOffset, 50, controlHeight,
                hwnd, (HMENU)0, hInstance, nullptr);

    volumeSlider = CreateWindow(TRACKBAR_CLASS, L"",
                               WS_CHILD | WS_VISIBLE,
                               xOffset + 180, yOffset, 200, controlHeight,
                               hwnd, (HMENU)IDC_VOLUME_SLIDER, hInstance, nullptr);

    SendMessage(volumeSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(volumeSlider, TBM_SETPOS, TRUE, 80);
}

void GuiWindow::UpdateConnectionInfo() {
    if (!connectionInfoEdit) return;

    std::string connStr = GetConnectionString();
    wchar_t wconnStr[256];
    MultiByteToWideChar(CP_ACP, 0, connStr.c_str(), -1, wconnStr, sizeof(wconnStr) / sizeof(wconnStr[0]));
    SetWindowText(connectionInfoEdit, wconnStr);
}

LRESULT CALLBACK GuiWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    GuiWindow* pThis = nullptr;

    if (msg == WM_CREATE) {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<GuiWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    } else {
        pThis = reinterpret_cast<GuiWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT GuiWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            running = false;
            PostQuitMessage(0);
            return 0;

        case WM_COMMAND: {
            int controlID = LOWORD(wParam);
            int notificationCode = HIWORD(wParam);

            switch (controlID) {
                case IDC_PARTICIPANT_COMBO:
                    if (notificationCode == CBN_SELCHANGE) {
                        int sel = SendMessage(participantCombo, CB_GETCURSEL, 0, 0);
                        selectedParticipants = sel + 2;
                        PeerNetwork::GetInstance().SetExpectedParticipants(selectedParticipants);
                        LOG_INFO("Selected " + std::to_string(selectedParticipants) + " participants");
                    }
                    break;

                case IDC_CONNECT_BUTTON: {
                    std::string remotePeer = GetRemotePeerIP();
                    LOG_INFO("Attempting to connect to: " + remotePeer);
                    SetConnectionStatus("Connecting to " + remotePeer + "...");
                    // Connection logic will be handled by main application
                    break;
                }

                case IDC_MUTE_BUTTON:
                    isMuted = !isMuted;
                    SetWindowText(muteButton, isMuted ? L"Unmute (M)" : L"Mute (M)");
                    LOG_INFO("Toggled mute: " + std::string(isMuted ? "ON" : "OFF"));
                    break;
            }
            break;
        }

        case WM_KEYDOWN:
            if (wParam == 'M' || wParam == 'm') {
                PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MUTE_BUTTON, 0), 0);
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}
