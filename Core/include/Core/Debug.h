#pragma once
#include <string>
#include <vector>
#include <mutex>

namespace Nova
{
    enum class LogLevel { Log, Info, Warn, Error };

    struct LogEntry
    {
        LogLevel    level;
        std::string message;
    };

    class Debug
    {
    public:
        static void Log(const char* format, ...);
        static void LogInfo(const char* format, ...);
        static void LogWarn(const char* format, ...);
        static void LogError(const char* format, ...);
        static void LogErrorAndExit(const char* format, ...);

        // Console panel access
        static const std::vector<LogEntry>& GetEntries() { return s_entries; }
        static void Clear() { s_entries.clear(); }

    private:
        static void pushEntry(LogLevel level, const char* format, va_list args);
        static std::vector<LogEntry> s_entries;
        static std::mutex            s_mutex;
    };
}