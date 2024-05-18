#pragma once

#define _NO_CRT_STDIO_INLINE

#define DISK_TAG 'disk'

#define DIRECT_DISK_PREFIX L"\\Device\\Vdisk"

#define IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_READWRITE CTL_CODE(MOUNTDEVCONTROLTYPE, 1, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define CORE_MNT_DISPATCHER        0x8001

#define SECTOR_SIZE 512

#define IOCTL_FILE_DISK_OPEN_FILE \
   CTL_CODE(FILE_DEVICE_DISK, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_FILE_DISK_CLOSE_FILE \
	CTL_CODE(FILE_DEVICE_DISK, 0x805, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_FILE_DISK_GET_FREE_ID  \
  CTL_CODE(FILE_DEVICE_DISK, 0x806, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_FILE_DISK_GET_ALL_DISK  \
  CTL_CODE(FILE_DEVICE_DISK, 0x808, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_FILE_DISK_GET_AMOUNT_OF_MOUNTED_DISKS  \
  CTL_CODE(FILE_DEVICE_DISK, 0x809, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY    CTL_CODE(MOUNTDEVCONTROLTYPE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define TOC_DATA_TRACK   0x04

#define MAX_DISK 20

#include <ntifs.h> 
#include <winsmcrd.h>
#include <handleapi.h>
#include <wdm.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <mountdev.h>
#include <ntdef.h> // Include the necessary header for OBJECT_ATTRIBUTES
#include <ntddvol.h>
#include <ntstrsafe.h>
#include <wdmsec.h>

NTSYSAPI NTSTATUS NTAPI ZwOpenProcessToken(
	IN HANDLE       ProcessHandle,
	IN ACCESS_MASK  DesiredAccess,
	OUT PHANDLE     TokenHandle
);

NTSYSAPI NTSTATUS NTAPI ZwAdjustPrivilegesToken(
	IN HANDLE               TokenHandle,
	IN BOOLEAN              DisableAllPrivileges,
	IN PTOKEN_PRIVILEGES    NewState,
	IN ULONG                BufferLength,
	OUT PTOKEN_PRIVILEGES   PreviousState OPTIONAL,
	OUT PULONG              ReturnLength
);

typedef struct TypeMountDisks {
	ULONG32 amount;
}MountDisksAmount, * PMountDisksAmount;

typedef struct _DEVICE_EXTENSION {
	UNICODE_STRING              device_name;
	HANDLE                      file_handle;
	UNICODE_STRING              file_name;
	ULONG                       device_ID;
	LARGE_INTEGER               file_size;
	PSECURITY_CLIENT_CONTEXT    security_client_context;//////
	LIST_ENTRY                  list_head;
	BOOLEAN                     media_in_device;
	KSPIN_LOCK                  list_lock;
	KEVENT                      request_event;
	PVOID                       thread_pointer;
	BOOLEAN                     terminate_thread;
	UNICODE_STRING				password;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;


typedef struct DiskParam {
	LARGE_INTEGER		Size;
	wchar_t				Letter;
	USHORT				FileNameLength;
	wchar_t				FileName[MAX_PATH];
	wchar_t				password[100];
	USHORT				PasswordLength;
} DISK_PARAMETERS, * PDISK_PARAMETERS;

typedef struct DiskParamR {
	LARGE_INTEGER		Size;
	wchar_t				Letter;
	USHORT				FileNameLength;
	wchar_t				FileName[MAX_PATH];
} Response, * PResponse;