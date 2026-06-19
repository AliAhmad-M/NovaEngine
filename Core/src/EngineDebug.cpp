#include <Core/EngineDebug.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

void EngineDebug::Log(const char* format, ...) 
{
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
}

void EngineDebug::LogInfo(const char* format, ...)
{
	fprintf(stdout, "[INFO]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
}

void EngineDebug::LogWarn(const char* format, ...)
{
	fprintf(stderr, "[WARN]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void EngineDebug::LogError(const char* format, ...) 
{
	fprintf(stderr, "[ERROR]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void EngineDebug::LogErrorAndExit(const char* format, ...) 
{
	fprintf(stderr, "[ERROR]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}
