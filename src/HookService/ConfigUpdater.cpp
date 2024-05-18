#include "pch.h"
#include "ConfigUpdater.h"
#include "nlohmann/json.hpp"
#include "Utils.h"
#include "driver_utils.h"
#include "Logger.h"


#define PORTAL_HOST_NAME "diploma-7jpg.onrender.com"

#define REGISTRY_FORMATED_DISKS_KEY "FormatedDisks"

const std::uint32_t g_kb = 1024;
const std::uint32_t g_mb = 1024 * g_kb;

ConfigUpdater::ConfigUpdater(Logger* logger)
	: m_io()
	, m_ctx(boost::asio::ssl::context::sslv23)
	, m_resolver(m_io)
	, m_socket(m_io, m_ctx)
	, m_config(nullptr)
	, m_deviceNumber(0)
	, m_logger(logger)
{

	// Set SNI Hostname (many hosts need this to handshake successfully)
	if (!SSL_set_tlsext_host_name(m_socket.native_handle(), PORTAL_HOST_NAME))
	{
		boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
		throw boost::system::system_error{ ec };
	}

	Request2ServerUpdateData();

}

void formatDisk(const wchar_t letter)
{
	wchar_t volume[] = L" :\\";
	volume[0] = letter;
	wchar_t volumeName[MAX_PATH + 1] = { 0 };
	DWORD serialNumber = 0;
	DWORD maxComponentLength = 0;
	DWORD fileSystemFlags = 0;
	wchar_t fileSystemName[MAX_PATH + 1] = { 0 };

	if (!GetVolumeInformation(volume, volumeName, ARRAYSIZE(volumeName), &serialNumber, &maxComponentLength, &fileSystemFlags, fileSystemName, ARRAYSIZE(fileSystemName)))
	{

		// Command to execute
		std::wstring command = L"cmd.exe /C format ";
		command += letter;
		command += L": /FS:NTFS /X /Q /y";

		// Convert command to LPCTSTR
		LPCWSTR lpCommand = command.c_str();

		// CreateProcess variables
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// Initialize STARTUPINFO
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// Create the process
		if (!CreateProcess(NULL,   // No module name (use command line)
			(LPWSTR)lpCommand,     // Command line
			NULL,                  // Process handle not inheritable
			NULL,                  // Thread handle not inheritable
			FALSE,                 // Set handle inheritance to FALSE
			0,                     // No creation flags
			NULL,                  // Use parent's environment block
			NULL,                  // Use parent's starting directory
			&si,                   // Pointer to STARTUPINFO structure
			&pi)                   // Pointer to PROCESS_INFORMATION structure
			)
		{
			throw std::runtime_error("CreateProcess format disk " + letter);
		}

		// Wait until child process exits.
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

	}
}



ConfigUpdater::~ConfigUpdater() = default;

void ConfigUpdater::CreateBackupDisk(const wchar_t letter)
{
	try
	{
		std::unique_ptr<DISK_PARAMETERS>diskParam = std::make_unique<DISK_PARAMETERS>();
		wchar_t DriverName[] = DriverName_;
		HANDLE Device = CreateFile(DriverName,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			COPY_FILE_NO_BUFFERING,
			NULL);
		if (Device == INVALID_HANDLE_VALUE)
		{
			throw std::exception("Error opening device: %d\n", GetLastError());
		}

		std::unique_ptr< MountDisksAmount>response = std::make_unique <MountDisksAmount>();

		*response.get() = { 0 };
		DWORD BytesReturned;
		if (!DeviceIoControl(Device, IOCTL_FILE_DISK_GET_AMOUNT_OF_MOUNTED_DISKS, NULL, 0, (PVOID)response.get(),
			sizeof(MountDisksAmount), &BytesReturned, NULL))
		{
			throw std::exception("Error sending IOCTL: %d\n", GetLastError());
		}

		CloseHandle(Device);
		m_deviceNumber = response.get()->amount;
		auto it = m_back_up_disks.find(letter);
		int disk_index = it->second;
		diskParam->Letter = m_config->m_back_up_config->m_disks[disk_index]->m_letter;

		diskParam->Size.QuadPart = _atoi64(std::to_string(m_config->m_back_up_config->m_disks[disk_index]->m_disk_size).c_str());

		wcscpy(diskParam->FileName, DiskFilePrefix);
		wcscat(diskParam->FileName, m_config->m_back_up_config->m_disks[disk_index]->m_file_path.c_str());
		diskParam->FileNameLength = static_cast<USHORT>(wcslen(diskParam->FileName));

		wcscpy(diskParam->password, m_config->m_back_up_config->m_disks[disk_index]->m_password.c_str());
		diskParam->PasswordLength = static_cast<USHORT>(wcslen(diskParam->password));

		DiskMount(m_deviceNumber, diskParam.get());
		m_deviceNumber++;

		std::string formated_disks;

		std::wstring letter_str_w;
		letter_str_w += letter;
		std::string letter_str = wstring2string(letter_str_w);
		try
		{
			GetDataFromRegister(formated_disks, REGISTRY_FORMATED_DISKS_KEY);
		}
		catch (const ReadRegistryError&)
		{
			formatDisk(letter);
			
			SetDataToRegister(letter_str, REGISTRY_FORMATED_DISKS_KEY);
			return;
		}
		if (formated_disks.find(letter_str) == std::string::npos)
		{
			formatDisk(letter);
			formated_disks += letter_str;
			EditRegistryValue(REGISTRY_FORMATED_DISKS_KEY, formated_disks);
		}
	}
	
	catch (const MountError&)
	{
		std::wstring letter_str;
		letter_str += letter;
		m_logger->Debug("Failed to mount disk "
			+ wstring2string(letter_str) + " .Current disk already exist.");
	}

}

void ConfigUpdater::CreateAllBackupDisks()
{
	for (size_t i = 0; i < m_config->m_back_up_config->m_disks.size(); i++)
	{
		CreateBackupDisk(m_config->m_back_up_config->m_disks[i]->m_letter);
	}
}

void ConfigUpdater::UnmockAllBackupDisks()
{
	for (size_t i = 0; i < m_config->m_back_up_config->m_disks.size(); i++)
	{
		UnmockBackupDisk(m_config->m_back_up_config->m_disks[i]->m_letter);

	}
	
}

void ConfigUpdater::UnmockBackupDisk(const wchar_t letter)
{
	DiskUnmount(letter);
	std::wstring log = L"Unmock Drive: ";
	log += letter;
	m_logger->Debug(wstring2string(log));
}

void ConfigUpdater::SendExtensionsToDriver()
{
	std::vector<std::string> vec;
	for (size_t i = 0; i < m_config->m_back_up_config->m_block_extensions.size(); i++)
	{
		vec.push_back(m_config->m_back_up_config->m_block_extensions[i]->m_block_extension);
	}
	SendBackUpExtensions(vec);
}

std::wstring extractFileName(const std::wstring& fullPath)
{
	// Find the position of the last directory separator
	size_t lastSeparatorPos = fullPath.find_last_of(L"\\");

	// If no separator found, return the full path
	if (lastSeparatorPos == std::string::npos)
	{
		return fullPath;
	}

	// Extract the file name part
	return fullPath.substr(lastSeparatorPos + 1);
}

void ConfigUpdater::BackupAllFiles()
{
	try
	{
		std::vector<std::wstring> backup_files_path;

		GetBackupFilesList(backup_files_path);

		for (size_t i = 0; i < backup_files_path.size(); i++)
		{
			std::wstring path = L"C:" + backup_files_path[i];
			std::ifstream inputfile(path, std::ios::binary | std::ios::in);
			std::wstring out_path;
			out_path += std::toupper(m_config->m_back_up_config->m_disks[0]->m_letter);
			out_path += L':';
			out_path += L'\\';
			out_path += extractFileName(backup_files_path[i]);
			std::ofstream outputfile(out_path, std::ios::binary | std::ios::out);
			boost::iostreams::copy(inputfile, outputfile);
		}
	}
	catch (const std::exception& ex)
	{
		m_logger->Error(ex.what());
	}
}

std::string removeTrailingCrLf(const std::string & input)
{
	std::string result = input;
	// Check if the string ends with "\r\n\r\n"
	if (result.size() >= 4 && result.substr(result.size() - 4) == "\r\n\r\n")
	{
		// Remove the last 4 characters
		result.erase(result.size() - 4);
	}
	return result;
}

std::string removeQuotes(const std::string & value)
{
	std::string result = value;

	// Check if the string starts and ends with double quotes
	if (result.size() >= 2 && result.front() == '"' && result.back() == '"')
	{
		// Erase the first and last characters (double quotes)
		result.erase(0, 1);
		result.erase(result.size() - 1);
	}

	return result;
}
std::wstring removeQuotes(const std::wstring & value)
{
	std::wstring result = value;

	// Check if the string starts and ends with double quotes
	if (result.size() >= 2 && result.front() == '"' && result.back() == '"')
	{
		// Erase the first and last characters (double quotes)
		result.erase(0, 1);
		result.erase(result.size() - 1);
	}

	return result;
}

void ConfigUpdater::Request2ServerUpdateData()
{
#ifdef DEBUG
	while (!IsDebuggerPresent())
	{
		Sleep(1000);
	}
#endif
	using tcp = boost::asio::ip::tcp;
	namespace http = boost::beast::http;

	tcp::resolver::query query(PORTAL_HOST_NAME, "https");
	tcp::resolver::iterator iter = m_resolver.resolve(query);
	tcp::resolver::iterator end; // End marker.

	// Connect to the server
	boost::asio::connect(m_socket.next_layer(), iter, end);
	m_socket.handshake(boost::asio::ssl::stream_base::client);

	// HTTP request

	std::string userEmail;
	std::string userPassword;
	GetDataFromRegister(userEmail, "UserEmail");
	GetDataFromRegister(userPassword, "UserPassword");

	// Set the body content
	std::string body = "{\"email\":\"" + userEmail + "\",\"password\":\"" + userPassword + "\"}";

	std::ostringstream request_stream;
	request_stream << "POST " << "/ServiceUserConfig" << " HTTP/1.1\r\n";
	request_stream << "Host: " << PORTAL_HOST_NAME << "\r\n";
	request_stream << "User-Agent: BoostBeastClient/1.0\r\n";
	request_stream << "Content-Type: application/json\r\n";
	request_stream << "Content-Length: " << body.length() << "\r\n";
	request_stream << "\r\n";
	request_stream << body;

	std::string request_string = request_stream.str();

	// Send the request
	boost::asio::write(m_socket, boost::asio::buffer(request_string));

	// HTTP response
	boost::beast::flat_buffer buffer;
	buffer.prepare(4 * 1024);
	http::response<http::dynamic_body> res;
	http::read(m_socket, buffer, res);




	std::string config_string = removeTrailingCrLf(boost::beast::buffers_to_string(res.body().data()));
	using nlohmann::json;
	json config_json;
	config_json = json::parse(config_string);
	if (config_json["Option"] != "Allow")
	{
		throw std::runtime_error("Not Allowed");
	}
	m_config.reset(new config());
	m_config->m_email = std::move(userEmail);
	m_config->m_password = std::move(userPassword);
	std::unique_ptr<back_up> back_up_data = std::make_unique<back_up>();

	int i = 0;
	for (const auto element : config_json["back_up_disks"])
	{
		if (!element["active"].get<bool>())
			continue;
		std::unique_ptr<back_up_disk_data> data(new back_up_disk_data());

		data->m_letter = static_cast<wchar_t>(std::tolower(removeQuotes(element["letter"].dump())[0]));

		data->m_disk_size = std::stoi(removeQuotes(element["disk_size"].dump())) * g_mb;
		data->m_password = removeQuotes(string2wstring(element["password"].dump()));
		data->m_file_path = removeQuotes(string2wstring(element["file_path"].dump()));

		m_back_up_disks.emplace(std::pair<wchar_t, int>{ data->m_letter, i});
		++i;
		back_up_data->m_disks.emplace_back(std::move(data));
	}
	for (const auto element : config_json["config"])
	{
		if (!element["action"].get<bool>())
			continue;
		std::unique_ptr <file_for_scan_data> data(new file_for_scan_data());
		data->m_block_extension = removeQuotes(element["extension"].dump());
		data->m_id = std::stoi(removeQuotes(element["back_up_type_id"].dump()));


		back_up_data->m_block_extensions.emplace_back(std::move(data));
	}

	m_config->m_back_up_config = std::move(back_up_data);

}
