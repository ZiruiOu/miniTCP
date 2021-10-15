#ifndef MINITCP_SRC_COMMON_LOGGING_H_
#define MINITCP_SRC_COMMON_LOGGING_H_
#include <iostream>
#include <sstream>

namespace minitcp {
enum LogLevel {
    kTrace = 0,
    kDebug,
    kInfo,
    kWarning,
    kError,
    kFatal,
};

class LogMessage : public std::basic_stringstream<char> {
   public:
    LogMessage(const char* filename, int lineno, LogLevel severity);
    ~LogMessage();

   protected:
    static char kLogLevelName[6][10];
    void ShowLoggingMessage();

   private:
    const char* filename_;
    int lineno_;
    LogLevel severity_;
};

class FatalLogMessage : public LogMessage {
   public:
    FatalLogMessage(const char* filename, int lineno);
    ~FatalLogMessage();
};

#define _MINITCP_LOG_TRACE \
    LogMessage(__FILE__, __LINE__, minitcp::LogLevel::kTrace)
#define _MINITCP_LOG_DEBUG \
    LogMessage(__FILE__, __LINE__, minitcp::LogLevel::kDebug)
#define _MINITCP_LOG_INFO \
    LogMessage(__FILE__, __LINE__, minitcp::LogLevel::kInfo)
#define _MINITCP_LOG_WARNING \
    LogMessage(__FILE__, __LINE__, minitcp::LogLevel::kWarning)
#define _MINITCP_LOG_ERROR \
    LogMessage(__FILE__, __LINE__, minitcp::LogLevel::kError)
#define _MINITCP_LOG_FATAL FatalLogMessage(__FILE__, __LINE__)

#define _LOG(level) _MINITCP_LOG_##level
#define _LOG_RANK(level, rank) _MINITCP_LOG_##level << "[ " << (rank) << "] : "

#define _GET_LOG_NAME(_1, _2, NAME, ...) NAME

#define MINITCP_SEVERITY minitcp::kInfo

#define MINITCP_LOG(...) \
    _GET_LOG_NAME(__VA_ARGS__, _LOG_RANK, _LOG)(__VA_ARGS__)

#define MINITCP_ASSERT(x) \
    if (!(x)) MINITCP_LOG(FATAL) << "check fail " #x << ' '

#define MINITCP_ASSERT_EQ(a, b) MINITCP_ASSERT((a) == (b))
#define MINITCP_ASSERT_NE(a, b) MINITCP_ASSERT((a) != (b))
#define MINITCP_ASSERT_LT(a, b) MINITCP_ASSERT((a) < (b))
#define MINITCP_ASSERT_GT(a, b) MINITCP_ASSERT((a) > (b))

}  // namespace minitcp

#endif  // !MINITCP_SRC_COMMON_LOGGING_H_