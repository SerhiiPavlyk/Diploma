#pragma once
#include <string>
#include <vector>

constexpr auto DiskFilePrefix = L"\\??\\";

struct file_for_scan_data
{
	int m_id;
	std::string m_block_extension;
};

struct back_up_disk_data
{
	wchar_t m_letter;
	std::uint32_t m_disk_size;
	std::wstring m_password;
	std::wstring m_file_path = DiskFilePrefix;
};

struct back_up
{
	std::vector< std::unique_ptr<file_for_scan_data>> m_block_extensions;
	std::vector< std::unique_ptr<back_up_disk_data>> m_disks;
};

struct config
{
	std::string m_email;
	std::string m_password;
	std::unique_ptr<back_up> m_back_up_config;

};

std::string wstring2string(const std::wstring& from);


std::string wstring2string(const wchar_t* from, size_t size);


std::wstring string2wstring(const std::string& from);

struct ReadRegistryError : public std::runtime_error
{
	ReadRegistryError(const std::string& message)
		: std::runtime_error(message)
	{

	}
};

void SetDataToRegister(const std::string& value, const std::string& valueName);

void GetDataFromRegister(std::string& value, const std::string& valueName);

void EditRegistryValue(const std::string& valueName, const std::string& newValue);