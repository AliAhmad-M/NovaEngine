#include <Core/Debug.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

using namespace Nova;

std::vector<LogEntry> Debug::s_entries;
std::mutex            Debug::s_mutex;

void Debug::pushEntry(LogLevel level, const char* format, va_list args)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, args);

    std::lock_guard<std::mutex> lock(s_mutex);
    s_entries.push_back({ level, std::string(buf) });

    if (s_entries.size() > 500)
        s_entries.erase(s_entries.begin());
}

void Debug::Log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    pushEntry(LogLevel::Log, format, args);
    va_end(args);

    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");
}

void Debug::LogInfo(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    pushEntry(LogLevel::Info, format, args);
    va_end(args);

    va_start(args, format);
    fprintf(stdout, "[INFO]: ");
    vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");
}

void Debug::LogWarn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    pushEntry(LogLevel::Warn, format, args);
    va_end(args);

    va_start(args, format);
    fprintf(stderr, "[WARN]: ");
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void Debug::LogError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    pushEntry(LogLevel::Error, format, args);
    va_end(args);

    va_start(args, format);
    fprintf(stderr, "[ERROR]: ");
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

void Debug::LogErrorAndExit(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    pushEntry(LogLevel::Error, format, args);
    va_end(args);

    va_start(args, format);
    fprintf(stderr, "[ERROR]: ");
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}