#include "PostDataParser.h"
#include "pch.h"

#include "PostDataParser.h"

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