#ifndef VOICEQWIK_WASAPI_AUDIO_ENGINE_H
#define VOICEQWIK_WASAPI_AUDIO_ENGINE_H

#include <utils/Common.h>
#include <audioclient.h>
#include <comdef.h>
#include <Objbase.h>
#include <mmdeviceapi.h>

class WasapiAudioEngine {
public:
    static WasapiAudioEngine& GetInstance();

    bool Initialize();
    void Shutdown();

    // Capture operations
    bool StartCapture();
    void StopCapture();
    bool GetCaptureBuffer(AudioBuffer& buffer);

    // Playback operations
    bool StartPlayback();
    void StopPlayback();
    bool QueuePlaybackBuffer(const AudioBuffer& buffer);

    // Device management
    bool EnumerateAudioDevices();
    bool SelectCaptureDevice(int deviceIndex);
    bool SelectPlaybackDevice(int deviceIndex);

private:
    WasapiAudioEngine();
    ~WasapiAudioEngine();

    WasapiAudioEngine(const WasapiAudioEngine&) = delete;
    WasapiAudioEngine& operator=(const WasapiAudioEngine&) = delete;

    // COM objects
    IMMDeviceEnumerator* deviceEnumerator;
    IMMDevice* captureDevice;
    IMMDevice* playbackDevice;
    IAudioClient* captureClient;
    IAudioClient* playbackClient;
    IAudioCaptureClient* captureControl;
    IAudioRenderClient* playbackControl;

    HANDLE captureEvent;
    HANDLE playbackEvent;

    std::thread captureThread;
    std::thread playbackThread;
    std::atomic<bool> captureRunning;
    std::atomic<bool> playbackRunning;

    std::queue<AudioBuffer> playbackQueue;
    std::mutex playbackQueueMutex;
    std::condition_variable playbackCV;

    void CaptureThreadProc();
    void PlaybackThreadProc();
    HRESULT InitializeAudioClient(IAudioClient* client, bool isCapture);
};

#endif // VOICEQWIK_WASAPI_AUDIO_ENGINE_H
