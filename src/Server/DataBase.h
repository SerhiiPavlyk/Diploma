#pragma once
#include <pqxx/pqxx>

#include "nlohmann/json_fwd.hpp"


struct IDataBase
{
    virtual ~IDataBase() = default;

    virtual bool CheckUserData(const std::string & email, const std::string & password, std::string & userName) = 0;
    virtual bool CheckUserData(const std::string & userName) = 0;
    virtual bool GetSupportedFormats(std::string & supportedFormats, const std::string & type_id) = 0;
    virtual bool GetUserBackupRules(const std::string & userName, std::string & supportedFormats) = 0;
    virtual bool GetUserBlockRules(const std::string & userName, std::string & supportedFormats) = 0;

    virtual bool SaveConfig(const std::string & userName, const nlohmann::json & config) = 0;

private:
    virtual bool GetUserConfig(const std::string & userName, nlohmann::json & config) = 0;
};


class DataBase
    : public IDataBase
{
public:

    DataBase();
    virtual ~DataBase() = default;

    virtual bool CheckUserData(const std::string& email, const std::string& password, std::string& userName) override;
    virtual bool CheckUserData(const std::string& userName);
    virtual bool GetSupportedFormats(std::string& supportedFormats, const std::string& type_id) override;
    virtual bool GetUserBackupRules(const std::string& userName, std::string& supportedFormats) override;
    virtual bool GetUserBlockRules(const std::string& userName, std::string& supportedFormats) override;

    virtual bool SaveConfig(const std::string& userName, const nlohmann::json& config) override;

private:
    virtual bool GetUserConfig(const std::string& userName, nlohmann::json& config) override;
private:
    // Establish a connection to the PostgreSQL database
    pqxx::connection m_conn;
};

