#include "FileWatcher.h"

void FileWatcher::rearmWatch(const WatchInfo& info) {
    if (info.filePath.empty() || !info.callback) {
        return;
    }
    addWatch(info.filePath, info.callback);
}

bool FileWatcher::isWatching() const {
    return m_running;
}
