#pragma once

#include <string>
#include <atomic>

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // Initialize/Shutdown audio device
    bool initialize();
    void shutdown();

    // Load/Unload an MP3 file
    bool loadAudio(const std::string& filePath);
    void unloadAudio();

    // Getters for status
    bool isAudioLoaded() const { return m_audioLoaded; }
    const std::string& getLoadedPath() const { return m_loadedPath; }

    // Control and playback query
    float getCurrentTime() const { return m_currentAudioTime.load(); }
    void seekTo(float timeSeconds);

    void setPlaying(bool playing);
    void setPlaybackSpeed(float speed);

    // Render loop callback interface (internal use only)
    void readFrames(void* pOutput, unsigned int frameCount);

private:
    void* m_decoderPtr; // Pointer to ma_decoder (opaque)
    void* m_devicePtr;  // Pointer to ma_device (opaque)

    bool m_initialized;
    bool m_audioLoaded;
    std::string m_loadedPath;

    std::atomic<bool> m_isPlaying;
    std::atomic<float> m_playbackSpeed;
    std::atomic<float> m_currentAudioTime;
    
    std::atomic<bool> m_seekRequested;
    std::atomic<float> m_seekTargetTime;
};
