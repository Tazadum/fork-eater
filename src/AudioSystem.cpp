#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioSystem.h"
#include "Logger.h"
#include <cstring>
#include <cmath>

AudioSystem::AudioSystem()
    : m_decoderPtr(nullptr)
    , m_devicePtr(nullptr)
    , m_initialized(false)
    , m_audioLoaded(false)
    , m_isPlaying(false)
    , m_playbackSpeed(1.0f)
    , m_currentAudioTime(0.0f)
    , m_seekRequested(false)
    , m_seekTargetTime(0.0f) {
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::initialize() {
    if (m_initialized) return true;

    ma_device* pDevice = new ma_device();
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.sampleRate        = 44100;
    deviceConfig.dataCallback      = [](ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
        AudioSystem* pAudioSystem = (AudioSystem*)pDevice->pUserData;
        if (pAudioSystem) {
            pAudioSystem->readFrames(pOutput, frameCount);
        }
    };
    deviceConfig.pUserData         = this;

    ma_result result = ma_device_init(NULL, &deviceConfig, pDevice);
    if (result != MA_SUCCESS) {
        LOG_ERROR("Failed to initialize audio device: error {}", (int)result);
        delete pDevice;
        return false;
    }

    m_devicePtr = pDevice;

    // Start device immediately - will play silence until audio is loaded and playing
    result = ma_device_start(pDevice);
    if (result != MA_SUCCESS) {
        LOG_ERROR("Failed to start audio device: error {}", (int)result);
        ma_device_uninit(pDevice);
        delete pDevice;
        m_devicePtr = nullptr;
        return false;
    }

    m_initialized = true;
    LOG_INFO("AudioSystem initialized successfully.");
    return true;
}

void AudioSystem::shutdown() {
    unloadAudio();

    if (m_devicePtr) {
        ma_device* pDevice = (ma_device*)m_devicePtr;
        ma_device_stop(pDevice);
        ma_device_uninit(pDevice);
        delete pDevice;
        m_devicePtr = nullptr;
    }

    m_initialized = false;
    LOG_INFO("AudioSystem shut down.");
}

bool AudioSystem::loadAudio(const std::string& filePath) {
    unloadAudio();

    if (!m_initialized) {
        if (!initialize()) {
            return false;
        }
    }

    ma_decoder* pDecoder = new ma_decoder();
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);

    ma_result result = ma_decoder_init_file(filePath.c_str(), &decoderConfig, pDecoder);
    if (result != MA_SUCCESS) {
        LOG_ERROR("Failed to load audio file '{}': error {}", filePath, (int)result);
        delete pDecoder;
        return false;
    }

    m_decoderPtr = pDecoder;
    m_audioLoaded = true;
    m_loadedPath = filePath;
    m_currentAudioTime.store(0.0f);
    m_seekRequested.store(true);
    m_seekTargetTime.store(0.0f);

    LOG_SUCCESS("Loaded audio file: '{}'", filePath);
    return true;
}

void AudioSystem::unloadAudio() {
    m_audioLoaded = false;
    m_loadedPath.clear();
    m_isPlaying.store(false);
    m_currentAudioTime.store(0.0f);

    if (m_decoderPtr) {
        // To prevent race conditions on the audio callback thread during deletion,
        // we stop the device, delete the decoder, and then resume the device.
        ma_device* pDevice = (ma_device*)m_devicePtr;
        if (pDevice) {
            ma_device_stop(pDevice);
        }

        ma_decoder* pDecoder = (ma_decoder*)m_decoderPtr;
        ma_decoder_uninit(pDecoder);
        delete pDecoder;
        m_decoderPtr = nullptr;

        if (pDevice) {
            ma_device_start(pDevice);
        }
    }
}

void AudioSystem::seekTo(float timeSeconds) {
    if (!m_audioLoaded) return;
    m_seekTargetTime.store(timeSeconds);
    m_seekRequested.store(true);
    m_currentAudioTime.store(timeSeconds);
}

void AudioSystem::setPlaying(bool playing) {
    m_isPlaying.store(playing);
}

void AudioSystem::setPlaybackSpeed(float speed) {
    m_playbackSpeed.store(speed);
}

void AudioSystem::readFrames(void* pOutput, unsigned int frameCount) {
    if (!m_audioLoaded || !m_decoderPtr) {
        memset(pOutput, 0, frameCount * 2 * sizeof(float));
        return;
    }

    ma_decoder* pDecoder = (ma_decoder*)m_decoderPtr;

    // Handle any requested seeks
    if (m_seekRequested.exchange(false)) {
        float targetTime = m_seekTargetTime.load();
        ma_uint64 targetFrame = (ma_uint64)(targetTime * 44100.0f);
        ma_decoder_seek_to_pcm_frame(pDecoder, targetFrame);
    }

    if (m_isPlaying.load() && m_playbackSpeed.load() == 1.0f) {
        ma_uint64 framesRead = 0;
        ma_result result = ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);
        
        if (result != MA_SUCCESS && result != MA_AT_END) {
            memset(pOutput, 0, frameCount * 2 * sizeof(float));
            return;
        }

        if (framesRead < frameCount) {
            float* pOutputFloat = (float*)pOutput;
            memset(pOutputFloat + framesRead * 2, 0, (frameCount - framesRead) * 2 * sizeof(float));
        }

        ma_uint64 cursor = 0;
        if (ma_decoder_get_cursor_in_pcm_frames(pDecoder, &cursor) == MA_SUCCESS) {
            m_currentAudioTime.store((float)cursor / 44100.0f);
        }
    } else {
        memset(pOutput, 0, frameCount * 2 * sizeof(float));
    }
}
