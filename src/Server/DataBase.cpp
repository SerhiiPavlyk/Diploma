#include "pch.h"
#include "DataBase.h"
#include "utils.h"
#include <exception>
#include "nlohmann/json.hpp"

DataBase::DataBase()
	//: m_conn("postgres://prod_dtv7_user:7e9NQLVQih3xr3tlkJLCbUFqgQxQLBJS@dpg-cnn2qpev3ddc73fmmh90-a.frankfurt-postgres.render.com/prod_dtv7")
{
}

std::string g_block_type_id = "block_type_id";
std::string g_back_up_type_id = "back_up_type_id";
std::string g_back_up_config = "back_up";
std::string g_block_config = "block_file";
std::string g_back_up_disks = "back_up_disks";

bool DataBase::CheckUserData(const std::string& email, const std::string& password, std::string& userName)
{
	if (m_conn.is_open())
	{
		pqxx::work txn(m_conn);

		// Execute SELECT query to retrieve all rows from a table
		pqxx::result result = txn.exec("SELECT email, password, user_name FROM public.\"Users\"");

		// Iterate over the result set
		for (const auto& row : result)
		{
			if (email == row.at("email").c_str()
				&& password == row.at("password").c_str())
			{
				userName = row.at("user_name").c_str();
				txn.commit();
				return true;
			}
		}

		// Commit the transaction
		txn.commit();

	}
	else
	{
		std::cerr << "Failed to connect to database" << std::endl;
	}

	return false;
}

bool DataBase::CheckUserData(const std::string& userName)
{
	if (m_conn.is_open())
	{
		pqxx::work txn(m_conn);

		// Execute SELECT query to retrieve all rows from a table
		pqxx::result result = txn.exec("SELECT user_name FROM public.\"Users\"");
		// Commit the transaction
		txn.commit();
		// Iterate over the result set
		for (const auto& row : result)
		{
			if (userName == utils::toLower(row.at("user_name").c_str()))
				return true;
		}

	}
	else
	{
		std::cerr << "Failed to connect to database" << std::endl;
	}

	return false;
}


bool DataBase::GetSupportedFormats(std::string& supportedFormats, const std::string& type_id)
{
	using json = nlohmann::json;
	if (m_conn.is_open())
	{
		pqxx::work txn(m_conn);

		// Execute SELECT query to retrieve all rows from a table
		pqxx::result result = txn.exec("SELECT id, extension FROM public.\"SupportedFormats\"");
		// Commit the transaction
		txn.commit();
		json supportedFormatsJSON;

		// Iterate over the result set
		for (const auto& row : result)
		{
			json obj = {
				{"extension", utils::toLower(row.at("extension").c_str())},
				{type_id, utils::toLower(row.at("id").c_str())}
			};
			supportedFormatsJSON.push_back(obj);
		}


		supportedFormats = supportedFormatsJSON.dump();
		return true;
	}
	else
	{
		std::cerr << "Failed to connect to database" << std::endl;
	}

	return false;
}

void mergeJsonArrays(nlohmann::json& inputJSON, const nlohmann::json& ToCompareJson)
{
	// Iterate through each object in jsonArray2
	for (auto obj2 : ToCompareJson)
	{
		bool found = false;

		// Check if the extension exists in jsonArray1
		for (const auto& obj1 : inputJSON)
		{
			if (obj1["extension"] == obj2["extension"])
			{
				found = true;
				break;
			}
		}

		// If the extension doesn't exist in jsonArray1, add it
		if (!found)
		{
			obj2["action"] = "allow";
			inputJSON.push_back(obj2);
		}
	}
	nlohmann::json res;
	res["config"] = inputJSON;
	inputJSON = res;
}

bool DataBase::GetUserBackupRules(const std::string& userName, std::string& supportedFormats)
{
	using json = nlohmann::json;
	if (m_conn.is_open())
	{
		pqxx::work txn(m_conn);

		// Execute SELECT query to retrieve all rows from a table
		pqxx::result result = txn.exec("SELECT user_name, config FROM public.\"Users\"");
		// Commit the transaction
		txn.commit();
		json supportedFormatsJSON, supportedBack_upJSON;

		json out_json;

		// Iterate over the result set
		for (const auto& row : result)
		{
			if (userName == utils::toLower(row.at("user_name").c_str()))
			{
				auto userConfig = utils::toLower(row.at("config").c_str());
				if (userConfig != "null")
				{
					supportedFormatsJSON = json::parse(userConfig);
					supportedFormatsJSON = supportedFormatsJSON[g_back_up_config];
				}
				std::string SupportedFormats;
				GetSupportedFormats(SupportedFormats, g_back_up_type_id);
				supportedBack_upJSON = json::parse(SupportedFormats);
				mergeJsonArrays(supportedFormatsJSON, supportedBack_upJSON);
				out_json = supportedFormatsJSON;
				out_json[g_back_up_disks] = json::parse(userConfig)[g_back_up_disks];
				supportedFormats = out_json.dump();
				return true;
			}
		}
	}
	else
	{
		std::cerr << "Failed to connect to database" << std::endl;
	}

	return false;
}

bool DataBase::GetUserBlockRules(const std::string& userName, std::string& supportedFormats)
{
	using json = nlohmann::json;
	if (m_conn.is_open())
	{
		pqxx::work txn(m_conn);

		// Execute SELECT query to retrieve all rows from a table
		pqxx::result result = txn.exec("SELECT user_name, config FROM public.\"Users\"");
		// Commit the transaction
		txn.commit();
		json supportedFormatsJSON, supportedBack_upJSON;


		// Iterate over the result set
		for (const auto& row : result)
		{
			if (userName == utils::toLower(row.at("user_name").c_str()))
			{
				auto userConfig = utils::toLower(row.at("config").c_str());
				if (userConfig != "null")
				{
					supportedFormatsJSON = json::parse(userConfig);
					supportedFormatsJSON = supportedFormatsJSON[g_block_config];
				}
				std::string SupportedFormats;
				GetSupportedFormats(SupportedFormats, g_block_type_id);
				supportedBack_upJSON = json::parse(SupportedFormats);
				mergeJsonArrays(supportedFormatsJSON, supportedBack_upJSON);
				supportedFormats = supportedFormatsJSON.dump();
				return true;
			}
		}
	}
	else
	{
		std::cerr << "Failed to connect to database" << std::endl;
	}

	return false;
}

bool DataBase::SaveConfig(const std::string& userName, const nlohmann::json& config)
{
	using json = nlohmann::json;
	


	try
	{
		if (m_conn.is_open())
		{

			std::string script = "UPDATE public.\"Users\" SET config = '<CONFIG>' WHERE user_name = '<USERNAME>' ";
			std::string config_value;

			// Define regular expressions for placeholders
			std::regex config_regex("<CONFIG>");
			std::regex username_regex("<USERNAME>");
			json userConfig;
			GetUserConfig(userName, userConfig);
			if (config["config"].at(0).contains(g_block_type_id))
			{
				userConfig[g_block_config] = config["config"];
			}
			else if (config["config"].at(0).contains(g_back_up_type_id))
			{
				userConfig[g_back_up_config] = config["config"];
				userConfig["back_up_disks"] = config["back_up_disks"];
			}
			else
			{
				std::cout << "Save error" << std::endl;
			}
			config_value = userConfig.dump();
			// Perform replacements
			script = std::regex_replace(script, config_regex, config_value);
			script = std::regex_replace(script, username_regex, userName);
			pqxx::work txn(m_conn);
			// Execute SELECT query to retrieve all rows from a table
			pqxx::result result = txn.exec(script);

			txn.commit();

			return true;
		}
		else
		{
			std::cerr << "Failed to connect to database" << std::endl;
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Failed. " << ex.what() << std::endl;
	}
	return false;
}

bool DataBase::GetUserConfig(const std::string& userName, nlohmann::json& config)
{
	using json = nlohmann::json;
	if (m_conn.is_open())
	{
		pqxx::work txn(m_conn);

		// Execute SELECT query to retrieve all rows from a table
		pqxx::result result = txn.exec("SELECT user_name, config FROM public.\"Users\"");
		// Commit the transaction
		txn.commit();

		// Iterate over the result set
		for (const auto& row : result)
		{
			if (userName == utils::toLower(row.at("user_name").c_str()))
			{
				auto userConfig = utils::toLower(row.at("config").c_str());
				if (userConfig != "null")
				{
					config = json::parse(userConfig);
					return true;
				}
			}
		}
	}
	else
	{
		std::cerr << "Failed to connect to database" << std::endl;
	}

	return false;
}
