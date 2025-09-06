#include "pch.h"
#include "driver_utils.h"
#include <shlobj.h>
#include <Windows.h>


#define IOCTL_FILE_DISK_OPEN_FILE \
   CTL_CODE(FILE_DEVICE_DISK, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_FILE_DISK_CLOSE_FILE \
	CTL_CODE(FILE_DEVICE_DISK, 0x805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_FILE_DISK_GET_FREE_ID  \
  CTL_CODE(FILE_DEVICE_DISK, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS)

#define DIRECT_DISK_PREFIX L"\\Device\\Vdisk"

#define NumDisks 25

void DiskMount(ULONG32 DeviceNumber, PDISK_PARAMETERS diskParam)
{
	wchar_t    VolumeName[] = L"\\\\.\\ :";
	wchar_t    DriveName[] = L" :\\";
	wchar_t    DeviceName[255];
	HANDLE  Device;
	DWORD   BytesReturned;

	VolumeName[4] = diskParam->Letter;
	DriveName[0] = diskParam->Letter;

	Device = CreateFile(
		VolumeName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
	);

	if (Device != INVALID_HANDLE_VALUE)
	{
		CloseHandle(Device);
		SetLastError(ERROR_BUSY);
		throw MountError("DISK is BUSY");
	}

	const wchar_t    DriverName[] = DriverName_;

	Device = CreateFile(DriverName,
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
	if (!DeviceIoControl(Device, IOCTL_FILE_DISK_GET_FREE_ID, NULL, 0, (PVOID)response.get(),
		sizeof(MountDisksAmount), &BytesReturned, NULL))
	{
		CloseHandle(Device);
		throw std::exception("Error sending IOCTL: %d\n", GetLastError());
	}

	CloseHandle(Device);
	DeviceNumber = response.get()->amount;
	swprintf(DeviceName, DIRECT_DISK_PREFIX L"%u", DeviceNumber);
	if (!DefineDosDevice(DDD_RAW_TARGET_PATH, &VolumeName[4], DeviceName))
	{
		throw std::runtime_error("Failed to register the disk");
	}

	Device = CreateFile(
		VolumeName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL
	);

	if (Device == INVALID_HANDLE_VALUE)
	{
		DefineDosDevice(DDD_REMOVE_DEFINITION, &VolumeName[4], NULL);
		throw std::runtime_error("Can't open disk!");
	}

	if (!DeviceIoControl(
		Device,
		IOCTL_FILE_DISK_OPEN_FILE,
		diskParam,
		sizeof(DiskParam) + diskParam->FileNameLength - 1,
		NULL,
		0,
		&BytesReturned,
		NULL))
	{
		DefineDosDevice(DDD_REMOVE_DEFINITION, &VolumeName[4], NULL);
		CloseHandle(Device);
		throw std::runtime_error("Mount disk UNSUCCESSFULL!");
	}
	CloseHandle(Device);
	SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, DriveName, NULL);

}

void DiskUnmount(const wchar_t Letter)
{
	wchar_t    VolumeName[] = L"\\\\.\\ :";
	wchar_t    DriveName[] = L" :\\";
	HANDLE  Device;
	DWORD   BytesReturned;

	VolumeName[4] = Letter;
	DriveName[0] = Letter;

	//open disk
	Device = CreateFile(VolumeName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_NO_BUFFERING,
		NULL);

	if (INVALID_HANDLE_VALUE == Device)
	{
		throw std::runtime_error("Cannot open disk for unmount");
	}

	//1. lock access to the virtual disk file system
	if (!DeviceIoControl(Device,
		FSCTL_LOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&BytesReturned,
		NULL))
	{
		CloseHandle(Device);
		throw std::runtime_error("Failed to lock access to the virtual disk file system");
	}

	//2. close the file from which the virtual disk was created
	if (!DeviceIoControl(Device,
		IOCTL_FILE_DISK_CLOSE_FILE,
		NULL,
		0,
		NULL,
		0,
		&BytesReturned,
		NULL))
	{
		CloseHandle(Device);
		throw std::runtime_error("Failed to close file in virtual disk");
	}

	//3. dismount virtual disk
	if (!DeviceIoControl(Device,
		FSCTL_DISMOUNT_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&BytesReturned,
		NULL))
	{
		CloseHandle(Device);
		throw std::runtime_error("Failed to unmount virtual disk");
	}

	//4. unlock access to the virtual disk file system
	if (!DeviceIoControl(Device,
		FSCTL_UNLOCK_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&BytesReturned,
		NULL))
	{
		CloseHandle(Device);
		throw std::runtime_error("Failed to unlock access to the virtual disk file system");
	}

	CloseHandle(Device);

	if (!DefineDosDevice(DDD_REMOVE_DEFINITION, &VolumeName[4], NULL))
	{
		throw std::runtime_error("Failed to remove definition");
	}

	SHChangeNotify(SHCNE_DRIVEREMOVED, SHCNF_PATH, DriveName, NULL);

}

UNICODE_STRING* stringToUnicode(const std::string& str)
{
	// Convert std::string to LPCSTR
	LPCSTR narrowStr = str.c_str();

	// Calculate the size required for the wide character buffer
	int wideSize = MultiByteToWideChar(CP_ACP, 0, narrowStr, -1, nullptr, 0);

	// Allocate memory for the wide character buffer
	wchar_t* wideBuffer = new wchar_t[wideSize];

	// Convert narrow string to wide string
	MultiByteToWideChar(CP_ACP, 0, narrowStr, -1, wideBuffer, wideSize);

	// Create UNICODE_STRING structure
	UNICODE_STRING* unicodeStr = new UNICODE_STRING;
	unicodeStr->Buffer = wideBuffer;
	unicodeStr->Length = static_cast<USHORT>(wcslen(wideBuffer) * sizeof(wchar_t));
	unicodeStr->MaximumLength = static_cast<USHORT>(wideSize * sizeof(wchar_t));

	return unicodeStr;
}

// Function to free memory allocated for UNICODE_STRING
void freeUnicodeString(UNICODE_STRING* unicodeStr)
{
	delete[] unicodeStr->Buffer;
	delete unicodeStr;
}


void SendBackUpExtensions(std::vector<std::string>& back_up_extensions)
{
	HANDLE  Device;
	DWORD   BytesReturned;

	const wchar_t    DriverName[] = DriverFSName_;

	Device = CreateFile(DriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (Device == INVALID_HANDLE_VALUE)
	{
		throw std::exception("Error opening device: %d\n", GetLastError());
	}

	std::unique_ptr<FSFILTER_EXTENSIONS>response = std::make_unique <FSFILTER_EXTENSIONS>();
	size_t i = 0;
	for (; i < back_up_extensions.size(); i++)
	{
		response->_extensions[i] = stringToUnicode(back_up_extensions[i]);
	}
	for (; i < ARRAY_SIZE; i++)
	{
		response->_extensions[i] = stringToUnicode(".default");
	}
	if (!DeviceIoControl(Device, IOCTL_FILE_SET_BACKUP_EXTENSIONS,
		(PVOID)response.get(), sizeof(FSFILTER_EXTENSIONS),
		NULL,
		0, &BytesReturned, NULL))
	{
		CloseHandle(Device);
		throw std::exception("Error sending IOCTL: %d\n", GetLastError());
	}

	CloseHandle(Device);

}

void SendBlockExtensions(std::vector<std::string>& block_extensions)
{
	HANDLE  Device;
	DWORD   BytesReturned;

	const wchar_t    DriverName[] = DriverFSName_;

	Device = CreateFile(DriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (Device == INVALID_HANDLE_VALUE)
	{
		throw std::exception("Error opening device: %d\n", GetLastError());
	}

	std::unique_ptr<FSFILTER_EXTENSIONS>response = std::make_unique <FSFILTER_EXTENSIONS>();
	size_t i = 0;
	for (; i < block_extensions.size(); i++)
	{
		response->_extensions[i] = stringToUnicode(block_extensions[i]);
	}
	for (; i < ARRAY_SIZE; i++)
	{
		response->_extensions[i] = stringToUnicode(".default");
	}
	if (!DeviceIoControl(Device, IOCTL_FILE_SET_BLOCK_EXTENSIONS,
		(PVOID)response.get(), sizeof(FSFILTER_EXTENSIONS),
		NULL,
		0, &BytesReturned, NULL))
	{
		CloseHandle(Device);
		throw std::exception("Error sending IOCTL: %d\n", GetLastError());
	}

	CloseHandle(Device);

}

// Structure to hold the dynamic array
typedef struct _DYNAMIC_UNICODE_STRING_ARRAY {
	UNICODE_STRING** strings;
	ULONG count;
	ULONG capacity;
} DYNAMIC_UNICODE_STRING_ARRAY, * PDYNAMIC_UNICODE_STRING_ARRAY;

// Define IOCTL input/output structures
typedef struct _MY_UNICODE_STRING {
	UNICODE_STRING str;
} FILE_PATH, * PFILE_PATH;

void GetBackupFilesList(std::vector<std::wstring>& backup_files)
{
	HANDLE  Device;
	DWORD   BytesReturned;

	const wchar_t    DriverName[] = DriverFSName_;

	Device = CreateFile(DriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (Device == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("GetBackupFilesList. Error opening device");
	}

	do
	{
		WCHAR response_buffer[MAX_PATH];
		ZeroMemory(response_buffer, sizeof(response_buffer));

		if (!DeviceIoControl(Device, IOCTL_FILE_GET_BACKUP_FILES,
			NULL, 0,
			&response_buffer,
			sizeof(response_buffer), &BytesReturned, NULL))
		{
			CloseHandle(Device);
			throw std::runtime_error("GetBackupFilesList. Error sending IOCTL");
		}
		if (BytesReturned == 0)
		{
			continue;
		}

		std::wstring str(response_buffer);
		backup_files.push_back(str);
	} while (BytesReturned != 0);

		CloseHandle(Device);
}

void GetBlockFilesList(std::vector<std::wstring>& blocked_files)
{
	HANDLE  Device;
	DWORD   BytesReturned;

	const wchar_t    DriverName[] = DriverFSName_;

	Device = CreateFile(DriverName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (Device == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("GetBackupFilesList. Error opening device");
	}

	do
	{
		WCHAR response_buffer[MAX_PATH];
		ZeroMemory(response_buffer, sizeof(response_buffer));

		if (!DeviceIoControl(Device, IOCTL_FILE_GET_BLOCKED_FILES,
			NULL, 0,
			&response_buffer,
			sizeof(response_buffer), &BytesReturned, NULL))
		{
			CloseHandle(Device);
			throw std::runtime_error("GetBackupFilesList. Error sending IOCTL");
		}
		if (BytesReturned == 0)
		{
			continue;
		}

		std::wstring str(response_buffer);
		blocked_files.push_back(str);
	} while (BytesReturned != 0);

	CloseHandle(Device);

}