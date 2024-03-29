#pragma once
#include <pqxx/pqxx>

#include "nlohmann/json_fwd.hpp"

class DataBase
{
public:

    DataBase();

    bool CheckUserData(const std::string& email, const std::string& password, std::string& userName);
    bool CheckUserData(const std::string& userName);
    bool GetSupportedFormats(std::string& supportedFormats);
    bool GetUserBackupRules(const std::string& userName, std::string& supportedFormats);
    bool SaveConfig(const std::string& userName, const nlohmann::json& config);

private:
    bool GetUserConfig(const std::string& userName, nlohmann::json& config);
private:
    // Establish a connection to the PostgreSQL database
    pqxx::connection m_conn;
};

