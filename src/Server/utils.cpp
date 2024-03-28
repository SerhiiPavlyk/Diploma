#include "pch.h"
#include "utils.h"

std::string utils::toLower(const std::string& str)
{
    std::string result;
    for (char c : str)
        result += std::tolower(c);
    return result;
}
