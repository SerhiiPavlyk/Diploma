#include "pch.h"
#include "Logger.h"

Logger::Logger(std::ofstream& outputStream)
    : output_stream_(outputStream)
{
}

void Logger::Error(const std::string& message)
{
    log("Error. " + message);
}

void Logger::Debug(const std::string& message)
{
    log("Debug. " + message);
}

void Logger::log(const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    output_stream_ << message << std::endl;
}
