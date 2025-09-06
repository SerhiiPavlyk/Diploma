#pragma once

#include <winioctl.h>
#include <SubAuth.h>

typedef struct DiskParam {
    LARGE_INTEGER		Size;
    wchar_t				Letter;
    USHORT				FileNameLength;
    wchar_t				FileName[MAX_PATH];
    wchar_t				password[100];
    USHORT				PasswordLength;
} DISK_PARAMETERS, * PDISK_PARAMETERS;

typedef struct TypeMountDisks {
    ULONG32 amount;
}MountDisksAmount, * PMountDisksAmount;

struct MountError : public std::runtime_error 
{
    MountError(const std::string& message) 
        : std::runtime_error(message) 
    {
    }
};

#define DriverName_ L"\\\\.\\GLOBALROOT\\Device\\VirtualEncryptedDisk"

#define DriverFSName_ L"\\\\.\\GLOBALROOT\\Device\\FSFilter"

#define IOCTL_FILE_DISK_GET_AMOUNT_OF_MOUNTED_DISKS  \
  CTL_CODE(FILE_DEVICE_DISK, 0x809, METHOD_BUFFERED, FILE_READ_ACCESS)

void DiskMount(ULONG32 DeviceNumber, PDISK_PARAMETERS  diskParam);

void DiskUnmount(const wchar_t Letter);

#define IOCTL_FILE_SET_BLOCK_EXTENSIONS \
   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x812, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FILE_SET_BACKUP_EXTENSIONS \
   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x813, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FILE_GET_BLOCKED_FILES \
   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x814, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FILE_GET_BACKUP_FILES \
   CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0x815, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ARRAY_SIZE 10

typedef struct _EXTENSIONS
{
    UNICODE_STRING* _extensions[ARRAY_SIZE];
} FSFILTER_EXTENSIONS, * PFSFILTER_EXTENSIONS;


void SendBackUpExtensions(std::vector<std::string>& back_up_extensions);
void SendBlockExtensions(std::vector<std::string>& block_extensions);
void GetBackupFilesList(std::vector<std::wstring>& back_up_extensions);
void GetBlockFilesList(std::vector<std::wstring>& blocked_files);