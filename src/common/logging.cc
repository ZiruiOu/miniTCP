#include "logging.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>

namespace minitcp {
char LogMessage::kLogLevelName[6][10] = {"Trace",   "Debug", "Info",
                                         "Warning", "Error", "Fatal"};

static std::mutex write_mutex_;

LogMessage::LogMessage(const char* filename, int lineno, LogLevel severity)
    : filename_(filename), lineno_(lineno), severity_(severity) {}

LogMessage::~LogMessage() { ShowLoggingMessage(); }

void LogMessage::ShowLoggingMessage() {
    auto now = std::chrono::system_clock::now();
    std::time_t as_time_t = std::chrono::system_clock::to_time_t(now);

    auto durations = now.time_since_epoch();
    auto durations_as_seconds =
        std::chrono::duration_cast<std::chrono::seconds>(durations);

    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
        durations - durations_as_seconds);

    char time_buffer[40];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
             localtime(&as_time_t));

    std::ostream& out_stream =
        (severity_ <= MINITCP_SEVERITY) ? std::cout : std::cerr;

    {
        std::scoped_lock lock(write_mutex_);
        out_stream << "[" << time_buffer << "." << std::setw(6)
                   << std::setfill('0') << microseconds.count() << ": "
                   << LogMessage::kLogLevelName[static_cast<int32_t>(severity_)]
                   << " " << filename_ << ": " << lineno_ << "] " << str();
    }
}

FatalLogMessage::FatalLogMessage(const char* filename, int lineno)
    : LogMessage(filename, lineno, LogLevel::kFatal) {}

FatalLogMessage::~FatalLogMessage() {
    ShowLoggingMessage();
    abort();
}

}  // namespace minitcp