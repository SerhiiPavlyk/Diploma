#pragma once

#include "DataBase.h"

class DataBaseMock
	: public IDataBase
{
public:
    DataBaseMock();
    virtual ~DataBaseMock() = default;

    virtual bool CheckUserData(const std::string & email, const std::string & password, std::string & userName) override;
    virtual bool CheckUserData(const std::string & userName) override;
    virtual bool GetSupportedFormats(std::string & supportedFormats, const std::string & type_id) override;
    virtual bool GetUserBackupRules(const std::string & userName, std::string & supportedFormats) override;
    virtual bool GetUserBlockRules(const std::string & userName, std::string & supportedFormats) override;

    virtual bool SaveConfig(const std::string & userName, const nlohmann::json & config) override;
private:
    virtual bool GetUserConfig(const std::string& userName, nlohmann::json& config) override;
};

