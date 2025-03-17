#pragma once

#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <sstream>

namespace Cosmos
{
	inline void OutputsToTerminal(const char* file, int line, const char* msg, ...)
	{
		constexpr unsigned int LOG_MAX_SIZE = 1024;
		constexpr unsigned int LOG_MAX_ENTRIES_SIZE = 128;

		char buffer[LOG_MAX_SIZE];
		va_list args;
		va_start(args, msg);
		vsnprintf(buffer, LOG_MAX_SIZE, msg, args);
		va_end(args);

		time_t now = time(NULL);
		char buf[40];
		struct tm tstruct = *localtime(&now);
		strftime(buf, sizeof(buf), "%X", &tstruct); // format: HH:MM:SS

		std::ostringstream oss;
		oss << "[" << buf << "]";
		oss << "[" << file << " - " << line << "]";
		oss << ": " << buffer;

		printf("%s\n", oss.str().c_str());
	}
}


#define LOG_TO_TERMINAL(...)										\
{																	\
	Cosmos::OutputsToTerminal( __FILE__, __LINE__, __VA_ARGS__);	\
}


#define LOG_ASSERT(x, ...)											\
{																	\
	if(!(x))														\
	{																\
		Cosmos::OutputsToTerminal(__FILE__, __LINE__, __VA_ARGS__);	\
		std::abort();												\
	}																\
}