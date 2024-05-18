#pragma once

//#define DEBUG

#include <fstream>
class Logger 
{
public:
    Logger(std::ofstream& outputStream);
    void Error(const std::string& message);
    void Debug(const std::string& message);

private:
    void log(const std::string& message);

private:
    std::ofstream& output_stream_;
    std::mutex mutex_;
};