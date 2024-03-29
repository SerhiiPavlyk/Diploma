#include "PostDataParser.h"
#include "pch.h"

#include "PostDataParser.h"
#include "utils.h"
#include "nlohmann/json.hpp"

void PostDataParser::CheckLogin(const std::string& data, std::string& login, std::string& password)
{
    try
    {
        std::regex emailRegex("\"email\":\"([^\"]+)\"");
        std::regex passwordRegex("\"password\":\"([^\"]+)\"");

        std::smatch emailMatch, passwordMatch;

        if (std::regex_search(data, emailMatch, emailRegex) && std::regex_search(data, passwordMatch, passwordRegex))
        {
            login = utils::toLower( emailMatch[1].str());
            password = utils::toLower(passwordMatch[1].str());
        }
        else
            std::cout << "Error";
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}

void PostDataParser::CheckUserName(const std::string& data, std::string& userName)
{
    try
    {
        std::regex UserNameRegex("\"token\":\"([^\"]+)\"");

        std::smatch UserNameMatch;

        if (std::regex_search(data, UserNameMatch, UserNameRegex))
            userName = utils::toLower(UserNameMatch[1].str());
        else
            std::cout << "Error";
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}

void PostDataParser::ParseConfig(const std::string& data, nlohmann::json& config)
{
    config = nlohmann::json::parse(data);
    config = config["config"];
}
