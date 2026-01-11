#include <utils/Logger.h>
#include <iostream>

Logger::Logger() {
    // Initialize console for debug output
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::SetLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filename, std::ios::app);
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char timeBuf[100];
    ctime_s(timeBuf, sizeof(timeBuf), &time);
    
    std::string levelStr = LevelToString(level);
    std::stringstream ss;
    ss << "[" << timeBuf << "] [" << levelStr << "] " << message;
    std::string output = ss.str();

    // Console output
    std::cout << output << std::endl;

    // File output
    if (logFile.is_open()) {
        logFile << output << std::endl;
        logFile.flush();
    }
}

std::string Logger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}
