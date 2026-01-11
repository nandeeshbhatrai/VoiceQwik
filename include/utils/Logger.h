#ifndef VOICEQWIK_LOGGER_H
#define VOICEQWIK_LOGGER_H

#ifdef ERROR
#undef ERROR
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <chrono>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& GetInstance() {
        static Logger instance;
        return instance;
    }

    void Log(LogLevel level, const std::string& message);
    void SetLogFile(const std::string& filename);

private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream logFile;
    std::mutex logMutex;

    std::string LevelToString(LogLevel level) const;
};

#endif // VOICEQWIK_LOGGER_H
