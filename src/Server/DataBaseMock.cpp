#include "pch.h"
#include "DataBaseMock.h"
#include "utils.h"
#include <exception>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

DataBaseMock::DataBaseMock()
{

}

bool DataBaseMock::CheckUserData(const std::string& email, const std::string& password, std::string& userName)
{
	return true;
}

bool DataBaseMock::CheckUserData(const std::string& userName)
{
	return true;
}


bool DataBaseMock::GetSupportedFormats(std::string& supportedFormats, const std::string& type_id)
{
	
	json supportedFormatsJSON;
	json obj = {
		{"extension", utils::toLower(".pdf")},
		{type_id, utils::toLower("1")}
	};
	supportedFormatsJSON.push_back(obj);
	obj = {
		{"extension", utils::toLower(".exe")},
		{type_id, utils::toLower("2")}
			};
	supportedFormatsJSON.push_back(obj);

	obj = {
		{"extension", utils::toLower(".docx")},
		{type_id, utils::toLower("3")}
	};
	supportedFormatsJSON.push_back(obj);

	obj = {
		{"extension", utils::toLower(".xlsx")},
		{type_id, utils::toLower("4")}
	};
	supportedFormatsJSON.push_back(obj);

	supportedFormats = supportedFormatsJSON.dump();

	return true;
}

bool DataBaseMock::GetUserBackupRules(const std::string& userName, std::string& supportedFormats)
{

	supportedFormats = R"({
  "back_up_disks": [
    {
      "active": true,
      "letter": "E",
      "disk_size": "102400",
      "password": "mySecret123",
      "file_path": "C:\\Backups\\diske.vd"
    },
    {
      "active": true,
      "letter": "B",
      "disk_size": "51200",
      "password": "backup123",
      "file_path": "C:\\\\Backups\\\\diskb.vd"
    }
  ],
  "config": [
    {
      "action": true,
      "extension": ".docx",
      "back_up_type_id": "3"
    },
    {
      "action": true,
      "extension": ".xlsx",
      "back_up_type_id": "4"
    }
  ]
})";

	return true;

}
bool DataBaseMock::GetUserBlockRules(const std::string& userName, std::string& supportedFormats)
{
	supportedFormats = R"({
  "config_block": [
    {
      "action": true,
      "extension": ".pdf",
      "block_type_id": "1"
    }
  ]
})";
	return true;
}

bool DataBaseMock::SaveConfig(const std::string& userName, const nlohmann::json& config)
{
	return true;
}

bool DataBaseMock::GetUserConfig(const std::string& userName, nlohmann::json& config)
{
	return true;
}
