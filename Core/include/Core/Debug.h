#pragma once

namespace Nova 
{
	class Debug
	{
	public:
		/**
		 * @brief Logs a generic message on the console, adds newline
		 * @param format The C-style format string
		 * @param ... Additional arguments to be formatted
		 */
		static void Log(const char* format, ...);

		/**
		 * @brief Logs an informative message on the console, adds newline
		 * @param format The C-style format string
		 * @param ... Additional arguments to be formatted
		 */
		static void LogInfo(const char* format, ...);

		/**
		 * @brief Logs a non fatal warn message on the console, adds newline
		 * @param format The C-style format string
		 * @param ... Additional arguments to be formatted
		 */
		static void LogWarn(const char* format, ...);

		/**
		 * @brief Logs a fatal error message on the console, adds newline
		 * @param format The C-style format string
		 * @param ... Additional arguments to be formatted
		 */
		static void LogError(const char* format, ...);

		/**
		 * @brief Logs a fatal error message on the console, adds newline, calls exit(1)
		 * @param format The C-style format string
		 * @param ... Additional arguments to be formatted
		 */
		static void LogErrorAndExit(const char* format, ...);
	};
}