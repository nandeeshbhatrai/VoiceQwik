#include <audio/WasapiAudioEngine.h>
#include <utils/Logger.h>
#include <functiondiscoverykeys_devpkey.h>

WasapiAudioEngine& WasapiAudioEngine::GetInstance() {
    static WasapiAudioEngine instance;
    return instance;
}

WasapiAudioEngine::WasapiAudioEngine()
    : deviceEnumerator(nullptr), captureDevice(nullptr), playbackDevice(nullptr),
      captureClient(nullptr), playbackClient(nullptr), captureControl(nullptr),
      playbackControl(nullptr), captureEvent(nullptr), playbackEvent(nullptr),
      captureRunning(false), playbackRunning(false) {
}

WasapiAudioEngine::~WasapiAudioEngine() {
    Shutdown();
}

bool WasapiAudioEngine::Initialize() {
    LOG_INFO("Initializing WASAPI Audio Engine");

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to initialize COM");
        return false;
    }

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), (void**)&deviceEnumerator);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to create device enumerator");
        CoUninitialize();
        return false;
    }

    // Get default capture device
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &captureDevice);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get default capture device");
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }

    // Get default playback device
    hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &playbackDevice);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get default playback device");
        captureDevice->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }

    // Activate capture client
    hr = captureDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                (void**)&captureClient);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to activate capture client");
        playbackDevice->Release();
        captureDevice->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }

    // Activate playback client
    hr = playbackDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                 (void**)&playbackClient);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to activate playback client");
        captureClient->Release();
        playbackDevice->Release();
        captureDevice->Release();
        deviceEnumerator->Release();
        CoUninitialize();
        return false;
    }

    LOG_INFO("WASAPI Audio Engine initialized successfully");
    return true;
}

void WasapiAudioEngine::Shutdown() {
    LOG_INFO("Shutting down WASAPI Audio Engine");

    StopCapture();
    StopPlayback();

    if (captureControl) {
        captureControl->Release();
        captureControl = nullptr;
    }
    if (playbackControl) {
        playbackControl->Release();
        playbackControl = nullptr;
    }
    if (captureClient) {
        captureClient->Release();
        captureClient = nullptr;
    }
    if (playbackClient) {
        playbackClient->Release();
        playbackClient = nullptr;
    }
    if (captureDevice) {
        captureDevice->Release();
        captureDevice = nullptr;
    }
    if (playbackDevice) {
        playbackDevice->Release();
        playbackDevice = nullptr;
    }
    if (deviceEnumerator) {
        deviceEnumerator->Release();
        deviceEnumerator = nullptr;
    }

    CoUninitialize();
}

bool WasapiAudioEngine::StartCapture() {
    if (captureRunning) return true;

    HRESULT hr = InitializeAudioClient(captureClient, true);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to initialize capture client");
        return false;
    }

    captureEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!captureEvent) {
        LOG_ERROR("Failed to create capture event");
        return false;
    }

    hr = captureClient->SetEventHandle(captureEvent);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to set capture event handle");
        CloseHandle(captureEvent);
        return false;
    }

    hr = captureClient->GetService(__uuidof(IAudioCaptureClient), (void**)&captureControl);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get audio capture client service");
        CloseHandle(captureEvent);
        return false;
    }

    hr = captureClient->Start();
    if (FAILED(hr)) {
        LOG_ERROR("Failed to start capture");
        captureControl->Release();
        CloseHandle(captureEvent);
        return false;
    }

    captureRunning = true;
    captureThread = std::thread(&WasapiAudioEngine::CaptureThreadProc, this);

    LOG_INFO("Capture started successfully");
    return true;
}

void WasapiAudioEngine::StopCapture() {
    if (!captureRunning) return;

    captureRunning = false;

    if (captureThread.joinable()) {
        captureThread.join();
    }

    if (captureClient) {
        captureClient->Stop();
    }

    if (captureEvent) {
        CloseHandle(captureEvent);
        captureEvent = nullptr;
    }

    LOG_INFO("Capture stopped");
}

bool WasapiAudioEngine::GetCaptureBuffer(AudioBuffer& buffer) {
    // This would be called from the networking module
    // For now, placeholder for integration
    return true;
}

bool WasapiAudioEngine::StartPlayback() {
    if (playbackRunning) return true;

    HRESULT hr = InitializeAudioClient(playbackClient, false);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to initialize playback client");
        return false;
    }

    playbackEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!playbackEvent) {
        LOG_ERROR("Failed to create playback event");
        return false;
    }

    hr = playbackClient->SetEventHandle(playbackEvent);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to set playback event handle");
        CloseHandle(playbackEvent);
        return false;
    }

    hr = playbackClient->GetService(__uuidof(IAudioRenderClient), (void**)&playbackControl);
    if (FAILED(hr)) {
        LOG_ERROR("Failed to get audio render client service");
        CloseHandle(playbackEvent);
        return false;
    }

    hr = playbackClient->Start();
    if (FAILED(hr)) {
        LOG_ERROR("Failed to start playback");
        playbackControl->Release();
        CloseHandle(playbackEvent);
        return false;
    }

    playbackRunning = true;
    playbackThread = std::thread(&WasapiAudioEngine::PlaybackThreadProc, this);

    LOG_INFO("Playback started successfully");
    return true;
}

void WasapiAudioEngine::StopPlayback() {
    if (!playbackRunning) return;

    playbackRunning = false;

    if (playbackThread.joinable()) {
        playbackThread.join();
    }

    if (playbackClient) {
        playbackClient->Stop();
    }

    if (playbackEvent) {
        CloseHandle(playbackEvent);
        playbackEvent = nullptr;
    }

    LOG_INFO("Playback stopped");
}

bool WasapiAudioEngine::QueuePlaybackBuffer(const AudioBuffer& buffer) {
    {
        std::lock_guard<std::mutex> lock(playbackQueueMutex);
        playbackQueue.push(buffer);
    }
    playbackCV.notify_one();
    return true;
}

bool WasapiAudioEngine::EnumerateAudioDevices() {
    LOG_INFO("Enumerating audio devices");
    // TODO: Implement device enumeration
    return true;
}

bool WasapiAudioEngine::SelectCaptureDevice(int deviceIndex) {
    LOG_INFO("Selecting capture device: " + std::to_string(deviceIndex));
    // TODO: Implement device selection
    return true;
}

bool WasapiAudioEngine::SelectPlaybackDevice(int deviceIndex) {
    LOG_INFO("Selecting playback device: " + std::to_string(deviceIndex));
    // TODO: Implement device selection
    return true;
}

HRESULT WasapiAudioEngine::InitializeAudioClient(IAudioClient* client, bool isCapture) {
    WAVEFORMATEX format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = AUDIO_CHANNELS;
    format.nSamplesPerSec = AUDIO_SAMPLE_RATE;
    format.wBitsPerSample = AUDIO_BITS_PER_SAMPLE;
    format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;

    REFERENCE_TIME hnsRequestedDuration = 100000; // 10ms

    HRESULT hr = client->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
        hnsRequestedDuration,
        0,
        &format,
        nullptr
    );

    return hr;
}

void WasapiAudioEngine::CaptureThreadProc() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (captureRunning) {
        DWORD flags = 0;
        uint32_t packetLength;

        HRESULT hr = captureControl->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) break;

        while (packetLength > 0 && captureRunning) {
            uint8_t* buffer = nullptr;
            uint32_t numFrames;

            hr = captureControl->GetBuffer(&buffer, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            // Process captured audio here
            // Buffer contains PCM audio data

            hr = captureControl->ReleaseBuffer(numFrames);
            if (FAILED(hr)) break;

            hr = captureControl->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) break;
        }

        WaitForSingleObject(captureEvent, INFINITE);
    }

    CoUninitialize();
}

void WasapiAudioEngine::PlaybackThreadProc() {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (playbackRunning) {
        {
            std::unique_lock<std::mutex> lock(playbackQueueMutex);
            playbackCV.wait(lock, [this] { return !playbackQueue.empty() || !playbackRunning; });

            if (!playbackRunning) break;

            if (!playbackQueue.empty()) {
                AudioBuffer buffer = playbackQueue.front();
                playbackQueue.pop();

                uint32_t numFrames = buffer.size() / AUDIO_CHANNELS;
                int16_t* renderBuffer = nullptr;

                HRESULT hr = playbackControl->GetBuffer(numFrames, (BYTE**)&renderBuffer);
                if (SUCCEEDED(hr)) {
                    std::memcpy(renderBuffer, buffer.data(), buffer.size() * sizeof(int16_t));
                    playbackControl->ReleaseBuffer(numFrames, 0);
                }
            }
        }

        WaitForSingleObject(playbackEvent, INFINITE);
    }

    CoUninitialize();
}
