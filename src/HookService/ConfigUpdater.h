#pragma once

struct config;
class Logger;

class ConfigUpdater
{
public:
	ConfigUpdater(Logger* logger);
	~ConfigUpdater();
	void CreateBackupDisk(const wchar_t letter);
	void UnmockBackupDisk(const wchar_t letter);
	void SendExtensionsToDriver();
	void BackupAllFiles();
	void BlockAllFiles();
	void CreateAllBackupDisks();
	void UnmockAllBackupDisks();
private:
	void Request2ServerUpdateData();
private:
	boost::asio::io_service m_io;
	boost::asio::ssl::context m_ctx;
	boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;
	boost::asio::ip::tcp::resolver m_resolver;
	std::unique_ptr<config> m_config;
	ULONG32 m_deviceNumber;
	std::map<wchar_t, int> m_back_up_disks;
	Logger* m_logger;
};

