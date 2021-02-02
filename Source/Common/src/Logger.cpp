#include <Common/Logger.h>
#include <iostream>
#include <cstdarg>

#ifdef YAPT_WINDOWS
#include <windows.h>
#endif

namespace YAPT
{

	const char* logSeverityToString(LogSeverity severity)
	{
		switch (severity)
		{
		case LogSeverity::LOG_INFO:
			return "[INFO]: ";
		case LogSeverity::LOG_WARNING:
			return "[WARNING]: ";
		case LogSeverity::LOG_ERROR:
			return "[ERROR]: ";
		case LogSeverity::LOG_CRITICAL_ERROR:
			return "[CRITICAL ERROR]: ";
		default:
			return "";
		}
	}


	void Log(LogSeverity severity, size_t length, const char* formatStr, ...)
	{
		//append severity prefix
		std::string debugStr;
		debugStr.reserve(length);
		
		debugStr += logSeverityToString(severity);
		 
		
		//append rest of the string
		va_list argumentList;
		va_start(argumentList, formatStr);
		int written = vsprintf(&debugStr[0], formatStr, argumentList);
		va_end(argumentList);
		
#ifdef YAPT_WINDOWS
		OutputDebugStringA(debugStr.c_str());
		OutputDebugStringA("\n");
		if (severity == LogSeverity::LOG_CRITICAL_ERROR)
		{
			__debugbreak();
		}
#else
		std::cout << debugStr.c_str() << std::endl;
#endif
	}
	
}
 