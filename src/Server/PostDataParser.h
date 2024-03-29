#pragma once

#include <string>
#include "nlohmann/json_fwd.hpp"

class PostDataParser 
{
public:
    static void CheckLogin(const std::string& data, std::string& login, std::string& password);
    static void CheckUserName(const std::string& data, std::string& userName);
    static void ParseConfig(const std::string& data, nlohmann::json& config);
};
