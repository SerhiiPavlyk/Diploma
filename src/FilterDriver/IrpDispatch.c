#include "FsFilter.h"
#include "Vector.h"
#include <minwindef.h>
#include <ntifs.h>
#include <ntddk.h>
///////////////////////////////////////////////////////////////////////////////////////////////////
// PassThrough IRP Handler
#define MEMORY_TAG 'MyT'

extern DYNAMIC_UNICODE_STRING_ARRAY* g_blocked_files;
extern DYNAMIC_UNICODE_STRING_ARRAY* g_backup_files;

extern UNICODE_STRING* g_blocked_extensions[ARRAY_SIZE];
extern UNICODE_STRING* g_backup_extensions[ARRAY_SIZE];


NTSTATUS FsFilterDispatchPassThrough(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP           Irp
)
{
	PIO_STACK_LOCATION	ioStack;
	ioStack = IoGetCurrentIrpStackLocation(Irp);
	if (ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FILE_SET_BACKUP_EXTENSIONS)
	{
		PFSFILTER_EXTENSIONS response = (PFSFILTER_EXTENSIONS)Irp->AssociatedIrp.SystemBuffer;
		// Copy strings
		for (int i = 0; i < ARRAY_SIZE; ++i)
		{
			UNICODE_STRING srcString = *response->_extensions[i];
			if (g_backup_extensions[i] == NULL)
			{
				// Allocate memory for the destination string
				g_backup_extensions[i] = (UNICODE_STRING*)ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), 'Tag'); // You may need to adjust the pool type and tag
			}

			if (g_backup_extensions[i] != NULL)
			{
				// Initialize the destination string
				RtlInitUnicodeString(g_backup_extensions[i], NULL);

				// Allocate memory for the buffer of the destination string
				g_backup_extensions[i]->Buffer = (WCHAR*)ExAllocatePoolWithTag(NonPagedPool, srcString.Length, 'Tag'); // You may need to adjust the pool type and tag

				if (g_backup_extensions[i]->Buffer != NULL)
				{
					// Copy the content of the source string to the destination string
					RtlCopyUnicodeString(g_backup_extensions[i], &srcString);
					// Copy the buffer and length of the source string to the destination string
					RtlCopyMemory(g_backup_extensions[i]->Buffer, srcString.Buffer, srcString.Length);
					// Correct the Length field
					g_backup_extensions[i]->Length = srcString.Length;

				}
				else
				{
					// Memory allocation failed for the buffer of the destination string
					// Handle error, free previously allocated memory if needed
					ExFreePoolWithTag(g_blocked_extensions[i], 'Tag');
					g_blocked_extensions[i] = NULL;
				}
			}
			else
			{
				// Handle case where source string is NULL
				g_blocked_extensions[i] = NULL;
			}
		}
		return STATUS_SUCCESS;
		//FsFilterDispatchClose(DeviceObject, Irp);
	}

	if (ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FILE_GET_BACKUP_FILES)
	{
		ULONG last_index = g_backup_files->count;
		if (last_index == 0)
		{
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}
		PWCHAR outputBuffer = Irp->AssociatedIrp.SystemBuffer;
		ULONG real_index = last_index - 1;
		ULONG out_size = g_backup_files->strings[real_index]->Length;
			// Copy the buffer and length of the source string to the destination string
		RtlCopyBytes(outputBuffer, g_backup_files->strings[real_index]->Buffer, out_size);
		
		if (g_backup_files->strings[real_index] != NULL)
		{
			ExFreePoolWithTag(g_backup_files->strings[real_index], 'Tag');

		}
		--g_backup_files->count;
		Irp->IoStatus.Status = STATUS_SUCCESS;

		Irp->IoStatus.Information = out_size;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(pDevExt->AttachedToDeviceObject, Irp);
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// IRP_MJ_CREATE IRP Handler

// Function to copy filename to dynamic array
NTSTATUS CopyFilenameToDynamicArray(UNICODE_STRING* fileName, PDYNAMIC_UNICODE_STRING_ARRAY dynamicArray)
{
	// Check if the file object is valid
	if (!fileName || !dynamicArray)
	{
		return STATUS_INVALID_PARAMETER;
	}

	// Allocate memory for the filename
	WCHAR* buffer = ExAllocatePoolWithTag(NonPagedPool, fileName->Length + sizeof(WCHAR), 'Tag');
	if (!buffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Copy the filename to the buffer
	RtlCopyMemory(buffer, fileName->Buffer, fileName->Length);
	buffer[fileName->Length / sizeof(WCHAR)] = L'\0'; // Null-terminate the string

	// Add the filename to the dynamic array
	NTSTATUS status = AddStringToDynamicArray(dynamicArray, buffer);

	// Free the buffer
	ExFreePool(buffer);

	return status;
}


BOOLEAN IsSubstring(UNICODE_STRING* str1, UNICODE_STRING* str2)
{
	if (str1->Length > str2->Length)
		return FALSE;

	for (int i = 0; i <= str2->Length - str1->Length; ++i)
	{
		BOOLEAN match = TRUE;
		for (int j = 0; j < str1->Length / sizeof(WCHAR); ++j)
		{
			if (i + j >= str2->Length / sizeof(WCHAR) || str1->Buffer[j] != str2->Buffer[i + j])
			{
				match = FALSE;
				break;
			}
		}
		if (match)
			return TRUE;
	}
	return FALSE;
}

NTSTATUS FsFilterDispatchCreate(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP           Irp
)
{
	PFILE_OBJECT pFileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

	UNICODE_STRING file_name;

	RtlInitUnicodeString(&file_name, NULL); // Initialize with NULL pointer

	file_name.MaximumLength = pFileObject->FileName.MaximumLength;
	file_name.Buffer = ExAllocatePoolWithTag(NonPagedPool, file_name.MaximumLength, 'Tag');

	if (file_name.Buffer != NULL)
	{
		// Copy the file name
		RtlCopyUnicodeString(&file_name, &pFileObject->FileName);
	}

	for (size_t i = 0; i < ARRAY_SIZE; i++)
	{
		if (IsSubstring(g_backup_extensions[i], &file_name))
		{
			BOOL found = FALSE;
			for (size_t j = 0; j < g_backup_files->count; j++)
			{
				if (IsSubstring(g_backup_files->strings[j], &file_name)
					|| RtlCompareUnicodeString(g_backup_files->strings[j], &file_name, TRUE) == 0)
					found = TRUE;
			}

			if (!found)
			{
				DbgPrintEx(0, 0, "%wZ\n", &file_name);
				CopyFilenameToDynamicArray(&file_name, g_backup_files);
				break;
			}
		}

	}

	PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (pDevExt != NULL)
	{
		return FsFilterDispatchPassThrough(DeviceObject, Irp);
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return STATUS_SUCCESS;
}


NTSTATUS FsFilterDispatchControl(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP           Irp
)
{
	PIO_STACK_LOCATION	ioStack;
	ioStack = IoGetCurrentIrpStackLocation(Irp);
	if (ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FILE_SET_BACKUP_EXTENSIONS)
	{
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (pDevExt != NULL)
	{
		return FsFilterDispatchPassThrough(DeviceObject, Irp);
	}
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS FsFilterDispatchClose(
	__in PDEVICE_OBJECT DeviceObject,
	__in PIRP           Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);

	PFSFILTER_DEVICE_EXTENSION pDevExt = (PFSFILTER_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (pDevExt != NULL)
	{
		return FsFilterDispatchPassThrough(DeviceObject, Irp);
	}
	IoSkipCurrentIrpStackLocation(Irp);
	return STATUS_SUCCESS;
}

