#include "FileWatcher.h"

#include <windows.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <unordered_map>

namespace {

std::wstring toWide(const std::string& path) {
    if (path.empty()) {
        return {};
    }
    int size = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring wide(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wide[0], size);
    return wide;
}

std::wstring fileNameFromPath(const std::wstring& path) {
    const auto pos = path.find_last_of(L"/\\");
    if (pos == std::wstring::npos) {
        return path;
    }
    return path.substr(pos + 1);
}

struct DirectoryWatchState {
    HANDLE hDir = INVALID_HANDLE_VALUE;
    HANDLE hEvent = nullptr;
    OVERLAPPED overlapped{};
    std::vector<uint8_t> buffer;
    std::wstring directory;
    bool readPending = false;
};

std::unordered_map<std::wstring, DirectoryWatchState> g_dirWatches;
std::mutex g_dirWatchMutex;

} // namespace

FileWatcher::FileWatcher()
    : m_running(false)
    , m_stopEvent(nullptr)
    , m_nextWatchId(1) {}

FileWatcher::~FileWatcher() {
    stop();
}

bool FileWatcher::platformStart() {
    HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!stopEvent) {
        return false;
    }
    m_stopEvent = stopEvent;
    return true;
}

void FileWatcher::platformStop() {
    if (m_stopEvent) {
        SetEvent(static_cast<HANDLE>(m_stopEvent));
    }

    std::lock_guard<std::mutex> lock(g_dirWatchMutex);
    for (auto& pair : g_dirWatches) {
        auto& state = pair.second;
        if (state.hDir != INVALID_HANDLE_VALUE) {
            CancelIoEx(state.hDir, &state.overlapped);
            CloseHandle(state.hDir);
            state.hDir = INVALID_HANDLE_VALUE;
        }
        if (state.hEvent) {
            CloseHandle(state.hEvent);
            state.hEvent = nullptr;
        }
    }
    g_dirWatches.clear();

    if (m_stopEvent) {
        CloseHandle(static_cast<HANDLE>(m_stopEvent));
        m_stopEvent = nullptr;
    }
}

bool FileWatcher::platformAddWatch(const std::string& filePath,
                                   FileChangedCallback callback,
                                   int& outWatchId) {
    namespace fs = std::filesystem;

    fs::path path(filePath);
    std::wstring widePath = toWide(fs::absolute(path).string());
    if (widePath.empty()) {
        return false;
    }

    std::wstring dirPath = widePath;
    const auto sep = dirPath.find_last_of(L"/\\");
    if (sep == std::wstring::npos) {
        return false;
    }
    dirPath.resize(sep);

    std::wstring fileName = fileNameFromPath(widePath);
    if (fileName.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_dirWatchMutex);
    auto& state = g_dirWatches[dirPath];
    if (state.hDir == INVALID_HANDLE_VALUE) {
        state.directory = dirPath;
        state.buffer.resize(64 * 1024);
        state.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!state.hEvent) {
            return false;
        }
        ZeroMemory(&state.overlapped, sizeof(state.overlapped));
        state.overlapped.hEvent = state.hEvent;

        state.hDir = CreateFileW(
            dirPath.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);
        if (state.hDir == INVALID_HANDLE_VALUE) {
            CloseHandle(state.hEvent);
            state.hEvent = nullptr;
            g_dirWatches.erase(dirPath);
            return false;
        }
    }

    outWatchId = m_nextWatchId++;
    WatchInfo info;
    info.watchDescriptor = outWatchId;
    info.filePath = filePath;
    info.callback = callback;

    {
        std::lock_guard<std::mutex> watchLock(m_watchMutex);
        m_watches[filePath] = info;
        m_watchDescriptorToPath[outWatchId] = filePath;
    }

    if (!state.readPending && m_running) {
        DWORD bytesReturned = 0;
        ResetEvent(state.hEvent);
        BOOL ok = ReadDirectoryChangesW(
            state.hDir,
            state.buffer.data(),
            static_cast<DWORD>(state.buffer.size()),
            FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
                FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            &state.overlapped,
            nullptr);
        if (ok || GetLastError() == ERROR_IO_PENDING) {
            state.readPending = true;
        }
    }

    return true;
}

void FileWatcher::platformRemoveWatch(int watchId) {
    std::lock_guard<std::mutex> lock(m_watchMutex);
    auto it = m_watchDescriptorToPath.find(watchId);
    if (it == m_watchDescriptorToPath.end()) {
        return;
    }
    m_watches.erase(it->second);
    m_watchDescriptorToPath.erase(it);
}

void FileWatcher::platformClearWatches() {
    {
        std::lock_guard<std::mutex> lock(m_watchMutex);
        m_watches.clear();
        m_watchDescriptorToPath.clear();
    }
    std::lock_guard<std::mutex> lock(g_dirWatchMutex);
    for (auto& pair : g_dirWatches) {
        auto& state = pair.second;
        if (state.hDir != INVALID_HANDLE_VALUE) {
            CancelIoEx(state.hDir, &state.overlapped);
            CloseHandle(state.hDir);
            state.hDir = INVALID_HANDLE_VALUE;
        }
        if (state.hEvent) {
            CloseHandle(state.hEvent);
            state.hEvent = nullptr;
        }
        state.readPending = false;
    }
    g_dirWatches.clear();
}

bool FileWatcher::start() {
    if (m_running) {
        return true;
    }
    if (!platformStart()) {
        std::cerr << "Failed to initialize file watcher" << std::endl;
        return false;
    }
    m_running = true;
    m_watchThread = std::thread(&FileWatcher::watchThread, this);
    return true;
}

void FileWatcher::stop() {
    if (!m_running) {
        return;
    }
    m_running = false;
    platformStop();
    if (m_watchThread.joinable()) {
        m_watchThread.join();
    }
    {
        std::lock_guard<std::mutex> lock(m_watchMutex);
        m_watches.clear();
        m_watchDescriptorToPath.clear();
    }
}

bool FileWatcher::addWatch(const std::string& filePath, FileChangedCallback callback) {
    if (!m_running) {
        std::cerr << "FileWatcher not started" << std::endl;
        return false;
    }
    removeWatch(filePath);
    int watchId = 0;
    if (!platformAddWatch(filePath, callback, watchId)) {
        std::cerr << "Failed to add watch for: " << filePath << std::endl;
        return false;
    }
    return true;
}

void FileWatcher::removeWatch(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_watchMutex);
    auto it = m_watches.find(filePath);
    if (it == m_watches.end()) {
        return;
    }
    platformRemoveWatch(it->second.watchDescriptor);
}

void FileWatcher::clearWatches() {
    platformClearWatches();
}

void FileWatcher::dispatchFileEvent(const std::string& filePath, unsigned long action) {
    WatchInfo info;
    bool hasInfo = false;
    {
        std::lock_guard<std::mutex> lock(m_watchMutex);
        auto it = m_watches.find(filePath);
        if (it != m_watches.end()) {
            info = it->second;
            hasInfo = true;
        }
    }
    if (!hasInfo) {
        return;
    }

    bool shouldNotify = false;
    if (action == FILE_ACTION_MODIFIED || action == FILE_ACTION_ADDED) {
        shouldNotify = true;
    }
    if (action == FILE_ACTION_RENAMED_OLD_NAME || action == FILE_ACTION_REMOVED) {
        rearmWatch(info);
        shouldNotify = true;
    }
    if (action == FILE_ACTION_RENAMED_NEW_NAME) {
        shouldNotify = true;
    }

    if (shouldNotify && info.callback) {
        info.callback(info.filePath);
    }
}

void FileWatcher::watchThread() {
    HANDLE stopEvent = static_cast<HANDLE>(m_stopEvent);

    while (m_running) {
        std::vector<HANDLE> waitHandles;
        std::vector<std::wstring> waitDirs;
        waitHandles.push_back(stopEvent);

        {
            std::lock_guard<std::mutex> lock(g_dirWatchMutex);
            for (auto& pair : g_dirWatches) {
                auto& state = pair.second;
                if (state.hDir == INVALID_HANDLE_VALUE || !state.hEvent) {
                    continue;
                }
                if (!state.readPending) {
                    DWORD bytesReturned = 0;
                    ResetEvent(state.hEvent);
                    BOOL ok = ReadDirectoryChangesW(
                        state.hDir,
                        state.buffer.data(),
                        static_cast<DWORD>(state.buffer.size()),
                        FALSE,
                        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
                            FILE_NOTIFY_CHANGE_LAST_WRITE,
                        &bytesReturned,
                        &state.overlapped,
                        nullptr);
                    if (ok || GetLastError() == ERROR_IO_PENDING) {
                        state.readPending = true;
                    }
                }
                waitHandles.push_back(state.hEvent);
                waitDirs.push_back(pair.first);
            }
        }

        if (waitHandles.size() == 1) {
            Sleep(50);
            continue;
        }

        DWORD result = WaitForMultipleObjects(
            static_cast<DWORD>(waitHandles.size()),
            waitHandles.data(),
            FALSE,
            200);

        if (!m_running) {
            break;
        }

        if (result == WAIT_OBJECT_0) {
            break;
        }

        if (result >= WAIT_OBJECT_0 + 1 &&
            result < WAIT_OBJECT_0 + waitHandles.size()) {
            size_t dirIndex = static_cast<size_t>(result - WAIT_OBJECT_0 - 1);
            std::wstring dirPath = waitDirs[dirIndex];

            DWORD bytesTransferred = 0;
            std::vector<std::wstring> changedNames;
            {
                std::lock_guard<std::mutex> lock(g_dirWatchMutex);
                auto it = g_dirWatches.find(dirPath);
                if (it == g_dirWatches.end()) {
                    continue;
                }
                auto& state = it->second;
                if (!GetOverlappedResult(state.hDir, &state.overlapped,
                                         &bytesTransferred, FALSE)) {
                    continue;
                }
                state.readPending = false;

                size_t offset = 0;
                while (offset < bytesTransferred) {
                    auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                        state.buffer.data() + offset);
                    std::wstring name(info->FileName, info->FileNameLength / sizeof(WCHAR));
                    changedNames.push_back(name);

                    if (info->NextEntryOffset == 0) {
                        break;
                    }
                    offset += info->NextEntryOffset;
                }
            }

            std::vector<std::string> pathsToNotify;
            {
                std::lock_guard<std::mutex> lock(m_watchMutex);
                for (const auto& pair : m_watches) {
                    std::wstring watchWide = toWide(
                        std::filesystem::absolute(std::filesystem::path(pair.first))
                            .string());
                    if (watchWide.empty()) {
                        continue;
                    }
                    std::wstring watchDir = watchWide;
                    const auto sep = watchDir.find_last_of(L"/\\");
                    if (sep != std::wstring::npos) {
                        watchDir.resize(sep);
                    }
                    if (watchDir != dirPath) {
                        continue;
                    }
                    std::wstring watchName = fileNameFromPath(watchWide);
                    for (const auto& changed : changedNames) {
                        if (_wcsicmp(changed.c_str(), watchName.c_str()) == 0) {
                            pathsToNotify.push_back(pair.first);
                            break;
                        }
                    }
                }
            }

            for (const auto& path : pathsToNotify) {
                dispatchFileEvent(path, FILE_ACTION_MODIFIED);
            }

            {
                std::lock_guard<std::mutex> lock(g_dirWatchMutex);
                auto it = g_dirWatches.find(dirPath);
                if (it != g_dirWatches.end() && it->second.hDir != INVALID_HANDLE_VALUE) {
                    DWORD bytesReturned = 0;
                    ResetEvent(it->second.hEvent);
                    ZeroMemory(&it->second.overlapped, sizeof(it->second.overlapped));
                    it->second.overlapped.hEvent = it->second.hEvent;
                    BOOL ok = ReadDirectoryChangesW(
                        it->second.hDir,
                        it->second.buffer.data(),
                        static_cast<DWORD>(it->second.buffer.size()),
                        FALSE,
                        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE |
                            FILE_NOTIFY_CHANGE_LAST_WRITE,
                        &bytesReturned,
                        &it->second.overlapped,
                        nullptr);
                    if (ok || GetLastError() == ERROR_IO_PENDING) {
                        it->second.readPending = true;
                    }
                }
            }
        }
    }
}
