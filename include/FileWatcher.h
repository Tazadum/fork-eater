#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

class FileWatcher {
public:
    using FileChangedCallback = std::function<void(const std::string&)>;

    FileWatcher();
    ~FileWatcher();

    bool start();
    void stop();
    bool addWatch(const std::string& filePath, FileChangedCallback callback);
    void removeWatch(const std::string& filePath);
    void clearWatches();
    bool isWatching() const;

private:
    struct WatchInfo {
        int watchDescriptor;
        std::string filePath;
        FileChangedCallback callback;
    };

    std::atomic<bool> m_running;
    std::thread m_watchThread;
    std::unordered_map<std::string, WatchInfo> m_watches;
    std::unordered_map<int, std::string> m_watchDescriptorToPath;
    mutable std::mutex m_watchMutex;

    void watchThread();
    void rearmWatch(const WatchInfo& info);

#ifdef _WIN32
    void* m_stopEvent;
    int m_nextWatchId;
    bool platformStart();
    void platformStop();
    bool platformAddWatch(const std::string& filePath, FileChangedCallback callback, int& outWatchId);
    void platformRemoveWatch(int watchId);
    void platformClearWatches();
    void dispatchFileEvent(const std::string& filePath, unsigned long action);
#else
    int m_inotifyFd;
#endif
};
