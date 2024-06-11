#pragma once

#include <sstream>
#include <vector>

namespace Cosmos
{
	class Logger
	{
	public:

		typedef enum Severity
		{
			Trace = 0,
			Info,
			Todo,
			Warn,
			Error,
			Assert,

			MAX_SEVERITY
		} Severity;

		struct ConsoleMessage
		{
			Severity severity;
			std::string message;
		};

	public:

		// constructor
		Logger() = default;

		// destructor
		~Logger();

		// returns the logger
		static Logger& GetInstance();

		// logs to a ostringstream object
		template<class T>
		inline Logger& operator<<(const T& other)
		{
			mOutput << other << std::endl;
			return *this;
		}

		// returns the stored console messages
		inline std::vector<ConsoleMessage>& GetMessages() { return mConsoleMessages; }

	public:

		// outputs a message to os's terminal
		void ToTerminal(Severity severity, const char* file, int line, const char* msg, ...);

		// outputs a message to a file
		void ToFile(Severity severity, const char* path, const char* file, int line, const char* msg, ...);

		// translates severity level to readable text
		const char* SeverityToConstChar(Severity severity);

	private:

		std::ostringstream mOutput;
		std::vector<ConsoleMessage> mConsoleMessages;
	};
}

// asserts are enabled on any build
#define COSMOS_ASSERT(x, ...)																							\
{																														\
	if(!(x))																											\
	{																													\
		Cosmos::Logger::GetInstance().ToTerminal(Cosmos::Logger::Severity::Assert, __FILE__, __LINE__, __VA_ARGS__);	\
		std::abort();																									\
	}																													\
}

// macros to facilitate using logging (only enabled on debug)
#if defined(ENGINE_DEBUG) || defined(EDITOR_DEBUG)

#define COSMOS_LOG(severity, ...)																\
{																								\
	Cosmos::Logger::GetInstance().ToTerminal(severity, __FILE__, __LINE__, __VA_ARGS__);		\
	if (severity == Cosmos::Logger::Severity::Assert) std::abort();								\
}

#define COSMOS_LOG_FILE(severity, filepath, ...)												\
{																								\
	Cosmos::Logger::GetInstance().ToFile(severity, filepath, __FILE__, __LINE__, __VA_ARGS__);	\
	if(severity == Cosmos::Logger::Severity::Assert) std::abort();								\
}

#else

#define COSMOS_LOG(severity, filepath, ...);
#define COSMOS_LOG_FILE(severity, ...);

#endif