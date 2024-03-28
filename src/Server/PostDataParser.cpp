#include "PostDataParser.h"
#include "pch.h"

#include "PostDataParser.h"
#include <regex>

std::unordered_map<std::string, double> PostDataParser::Parse(const std::string& postData) {
    std::unordered_map<std::string, double> data;
    size_t pos = 0;
    try {
        while (pos < postData.size()) {
            size_t equalsPos = postData.find('=', pos);
            if (equalsPos == std::string::npos) {
                break;
            }
            std::string key = postData.substr(pos, equalsPos - pos);
            pos = equalsPos + 1;
            size_t ampersandPos = postData.find('&', pos);
            if (ampersandPos == std::string::npos) {
                ampersandPos = postData.size();
            }

            double value = std::stod(postData.substr(pos, ampersandPos - pos));
            data[key] = value;
            pos = ampersandPos + 1;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return data;
    }
    return data;
}

std::string toLower(const std::string& str)
{
    std::string result;
    for (char c : str)
    {
        result += std::tolower(c);
    }
    return result;
}

void PostDataParser::CheckLogin(const std::string Data, std::string& login, std::string& password)
{
    
    try
    {
        std::regex emailRegex("\"email\":\"([^\"]+)\"");
        std::regex passwordRegex("\"password\":\"([^\"]+)\"");

        std::smatch emailMatch, passwordMatch;

        if (std::regex_search(Data, emailMatch, emailRegex) && std::regex_search(Data, passwordMatch, passwordRegex))
        {
            login = toLower( emailMatch[1].str());
            password = toLower(passwordMatch[1].str());
        }
        else
        {
            std::cout << "Error";
        }
       
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}