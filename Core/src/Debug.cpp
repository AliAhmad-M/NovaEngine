#include <Core/Debug.h>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

using namespace Nova;

void Debug::Log(const char* format, ...) 
{
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
}

void Debug::LogInfo(const char* format, ...)
{
	fprintf(stdout, "[INFO]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	fprintf(stdout, "\n");
}

void Debug::LogWarn(const char* format, ...)
{
	fprintf(stderr, "[WARN]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void Debug::LogError(const char* format, ...)
{
	fprintf(stderr, "[ERROR]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
}

void Debug::LogErrorAndExit(const char* format, ...)
{
	fprintf(stderr, "[ERROR]: ");
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "\n");
	exit(1);
}
