#pragma once
namespace YAPT
{
	enum class LogSeverity
	{
		LOG_INFO,
		LOG_WARNING,
		LOG_ERROR,
		LOG_CRITICAL_ERROR
	};

	void Log(LogSeverity severity, size_t length, const char* formatStr, ...);
	
#define MAXDEBUGMSGLEN 1028

#define YAPT_LOG_DEBUG(message,...) YAPT::Log(YAPT::LogSeverity::LOG_INFO, MAXDEBUGMSGLEN, message, ##__VA_ARGS__);
#define YAPT_LOG_WARNING(message,...) YAPT::Log(YAPT::LogSeverity::LOG_WARNING, MAXDEBUGMSGLEN, message, ##__VA_ARGS__);
#define YAPT_LOG_ERROR(message,...) YAPT::Log(YAPT::LogSeverity::LOG_ERROR, MAXDEBUGMSGLEN, message, ##__VA_ARGS__);
#define YAPT_LOG_FATAL_ERROR(message,...) YAPT::Log(YAPT::LogSeverity::LOG_CRITICAL_ERROR, MAXDEBUGMSGLEN, message, ##__VA_ARGS__);

#define YAPT_LOG_DEBUG_W_LENGTH(message,messageLength,...) YAPT::Log(YAPT::LogSeverity::LOG_INFO, messageLength, message, ##__VA_ARGS__);
#define YAPT_LOG_WARNING_W_LENGTH(message,messageLength,...) YAPT::Log(YAPT::LogSeverity::LOG_WARNING, messageLength, message, ##__VA_ARGS__);
#define YAPT_LOG_ERROR_W_LENGTH(message,messageLength,...) YAPT::Log(YAPT::LogSeverity::LOG_ERROR, messageLength, message, ##__VA_ARGS__);
#define YAPT_LOG_FATAL_ERROR_W_LENGTH(message,messageLength,...) YAPT::Log(YAPT::LogSeverity::LOG_CRITICAL_ERROR, messageLength, message, ##__VA_ARGS__);

}

