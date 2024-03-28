#pragma once

#include <unordered_map>
#include <string>

class PostDataParser {
public:
    static std::unordered_map<std::string, double> Parse(const std::string& postData);
    static void CheckLogin(const std::string Data, std::string& login, std::string& password);
};
