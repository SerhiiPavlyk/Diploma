#include "pch.h"
#include "Utils.h"
#define REG_KEY "SOFTWARE\\SeekAndHideSecure"
std::string wstring2string(const std::wstring& from)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(from);
}

std::string wstring2string(const wchar_t* from, size_t size)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(from, from + size);
}

std::wstring string2wstring(const std::string& from)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.from_bytes(from);
}

void SetDataToRegister(const std::string& value, const std::string& valueName)
{
	HKEY hKey;
	LONG result;

	// Open or create the registry key
	result = RegCreateKeyExA(HKEY_LOCAL_MACHINE, REG_KEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
	if (result != ERROR_SUCCESS)
	{
		throw std::runtime_error("Error creating or opening registry key.");
	}

	// Write the value to the registry
	result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>(value.length()));
	if (result != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		throw std::runtime_error("Error writing to registry.");
	}

	// Close the registry key
	RegCloseKey(hKey);
}

void GetDataFromRegister(std::string& value, const std::string& valueName)
{
	HKEY hKey;

	auto FindInReg = [](HKEY hKey, const char* valueName)
	{
		LONG result;
		// Read a string value from the registry
		const DWORD maxValueSize = 1024;
		char valueBuffer[maxValueSize];
		DWORD dataSize = maxValueSize;
		result = RegQueryValueExA(hKey, valueName, NULL, NULL, reinterpret_cast<LPBYTE>(valueBuffer), &dataSize);
		if (result != ERROR_SUCCESS)
		{

			RegCloseKey(hKey);
			throw ReadRegistryError("Error reading registry value.");
		}

		// Convert the buffer to a string
		return std::string(valueBuffer, dataSize - 1);
	};
	LONG result;
	// Open the registry key
	result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS)
	{
		throw std::runtime_error("There is not a reqKey");
	}

	// Read a email value from the registry
	value = FindInReg(hKey, valueName.c_str());

	// Close the registry key
	RegCloseKey(hKey);

}

void EditRegistryValue(const std::string& valueName, const std::string& newValue)
{
	HKEY hKey;
	LONG result;

	// Open the registry key
	result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, REG_KEY, 0, KEY_READ | KEY_WRITE, &hKey);
	if (result != ERROR_SUCCESS)
	{
		throw std::runtime_error("Error opening registry key.");
	}

	// Read the current value
	DWORD dataSize = 0;
	result = RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, NULL, &dataSize);
	if (result != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		throw std::runtime_error("Error querying registry value.");
	}

	// Allocate memory for the current value
	std::vector<char> valueBuffer(dataSize);

	// Read the current value
	result = RegQueryValueExA(hKey, valueName.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(valueBuffer.data()), &dataSize);
	if (result != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		throw std::runtime_error("Error querying registry value.");
	}

	// Convert the current value to a string
	std::string currentValue(valueBuffer.begin(), valueBuffer.end());

	// If the value is different, update it
	if (currentValue != newValue)
	{
		// Write the new value to the registry
		result = RegSetValueExA(hKey, valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(newValue.c_str()), static_cast<DWORD>(newValue.length()));
		if (result != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			throw std::runtime_error("Error writing to registry.");
		}
	}

	// Close the registry key
	RegCloseKey(hKey);
}
