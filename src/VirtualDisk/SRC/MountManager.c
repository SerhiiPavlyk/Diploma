#include "pch.h"
#include "MountManager.h"

void MountManagerInit(PDRIVER_OBJECT DriverObject)
{
	DataOfMountManager.DriverObject = DriverObject;
	DataOfMountManager.gMountedDiskCount = 0;
	DataOfMountManager.amountOfMountedDisk.amount = 0;
	for (UINT32 i = 0; i < MAX_DISK; i++)
	{
		DataOfMountManager.listOfDisks[i].Letter = L'-';
	}
}

NTSTATUS FileDiskCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = FILE_OPENED;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS MountManagerCreateDevice()
{
	UNICODE_STRING      device_name;
	NTSTATUS            status = STATUS_SUCCESS;
	PDEVICE_OBJECT      device_object;
	PDEVICE_EXTENSION   device_extension;
	HANDLE              thread_handle;
	UNICODE_STRING      sddl;

	ASSERT(DataOfMountManager.DriverObject != NULL);

	device_name.Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool, MAXIMUM_FILENAME_LENGTH * 2, DISK_TAG);

	if (device_name.Buffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	device_name.Length = 0;
	device_name.MaximumLength = MAXIMUM_FILENAME_LENGTH * 2;
	DataOfMountManager.gMountedDiskCount++;

	RtlUnicodeStringPrintf(&device_name, DIRECT_DISK_PREFIX L"%u", (DataOfMountManager.gMountedDiskCount - 1));
	RtlInitUnicodeString(&sddl, L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;BU)");

	status = IoCreateDeviceSecure(
		DataOfMountManager.DriverObject,
		sizeof(DEVICE_EXTENSION),
		&device_name,
		FILE_DEVICE_DISK,
		0,
		FALSE,
		&sddl,
		NULL,
		&device_object
	);

	if (!NT_SUCCESS(status))
	{
		ExFreePool(device_name.Buffer);
		return status;
	}

	device_object->Flags |= DO_DIRECT_IO;

	device_extension = (PDEVICE_EXTENSION)device_object->DeviceExtension;
	device_extension->device_ID = DataOfMountManager.gMountedDiskCount - 1;
	device_extension->device_name.Length = device_name.Length;
	device_extension->device_name.MaximumLength = device_name.MaximumLength;
	device_extension->device_name.Buffer = device_name.Buffer;
	device_extension->media_in_device = FALSE;
	InitializeListHead(&device_extension->list_head);

	KeInitializeSpinLock(&device_extension->list_lock);

	KeInitializeEvent(&device_extension->request_event, SynchronizationEvent, FALSE);

	device_extension->terminate_thread = FALSE;

	status = PsCreateSystemThread(
		&thread_handle,
		(ACCESS_MASK)0L,
		NULL,
		NULL,
		NULL,
		IOCTLHandle,
		device_object
	);

	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(device_object);
		ExFreePool(device_name.Buffer);
		return status;
	}

	status = ObReferenceObjectByHandle(
		thread_handle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&device_extension->thread_pointer,
		NULL
	);

	if (!NT_SUCCESS(status))
	{
		ZwClose(thread_handle);

		device_extension->terminate_thread = TRUE;

		KeSetEvent(&device_extension->request_event, (KPRIORITY)0, FALSE);

		IoDeleteDevice(device_object);

		ExFreePool(device_name.Buffer);

		return status;
	}

	ZwClose(thread_handle);
	return status;
}

NTSTATUS FileDiskOpenFile(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PDEVICE_EXTENSION               device_extension;
	PDISK_PARAMETERS                diskParam;
	NTSTATUS                        status;
	OBJECT_ATTRIBUTES               object_attributes;
	FILE_END_OF_FILE_INFORMATION    file_eof;
	FILE_BASIC_INFORMATION          file_basic;
	FILE_STANDARD_INFORMATION       file_standard;
	FILE_ALIGNMENT_INFORMATION      file_alignment;

	ASSERT(DeviceObject != NULL);
	ASSERT(Irp != NULL);

	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	diskParam = (PDISK_PARAMETERS)Irp->AssociatedIrp.SystemBuffer;



	device_extension->file_name.Length = diskParam->FileNameLength * sizeof(wchar_t);
	device_extension->file_name.MaximumLength = device_extension->file_name.Length + 1;
	device_extension->file_name.Buffer = ExAllocatePoolWithTag(NonPagedPool, device_extension->file_name.Length, DISK_TAG);

	if (device_extension->file_name.Buffer == NULL)
	{
		ExFreePool(device_extension->file_name.Buffer);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory(device_extension->file_name.Buffer,
		diskParam->FileName,
		device_extension->file_name.Length
	);

	device_extension->password.Length = diskParam->PasswordLength * sizeof(wchar_t);
	device_extension->password.MaximumLength = device_extension->password.Length + 1;
	device_extension->password.Buffer = ExAllocatePoolWithTag(NonPagedPool, device_extension->password.Length, DISK_TAG);

	if (device_extension->password.Buffer == NULL || device_extension->password.Buffer == NULL)
	{
		ExFreePool(device_extension->file_name.Buffer);
		ExFreePool(device_extension->password.Buffer);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory(
		device_extension->password.Buffer,
		diskParam->password,
		device_extension->password.Length
	);

	InitializeObjectAttributes(
		&object_attributes,
		&device_extension->file_name,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL
	);

	status = ZwCreateFile(
		&device_extension->file_handle,
		GENERIC_READ | GENERIC_WRITE,
		&object_attributes,
		&Irp->IoStatus,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE |
		FILE_RANDOM_ACCESS |
		FILE_NO_INTERMEDIATE_BUFFERING |
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0
	);

	if (NT_SUCCESS(status))
	{
		KdPrint(("FileDisk: File %.*S opened.\n", device_extension->file_name.Length / 2, device_extension->file_name.Buffer));
	}

	if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_NO_SUCH_FILE)
	{
		if (diskParam->Size.QuadPart == 0)
		{
			DbgPrint("FileDisk: File %.*S not found.\n", device_extension->file_name.Length / 2, device_extension->file_name.Buffer);
			ExFreePool(device_extension->file_name.Buffer);
			ExFreePool(device_extension->password.Buffer);

			Irp->IoStatus.Status = STATUS_NO_SUCH_FILE;
			Irp->IoStatus.Information = 0;

			return STATUS_NO_SUCH_FILE;
		}
		else
		{
			status = ZwCreateFile(
				&device_extension->file_handle,
				GENERIC_READ | GENERIC_WRITE,
				&object_attributes,
				&Irp->IoStatus,
				NULL,
				FILE_ATTRIBUTE_NORMAL,
				0,
				FILE_OPEN_IF,
				FILE_NON_DIRECTORY_FILE |
				FILE_RANDOM_ACCESS |
				FILE_NO_INTERMEDIATE_BUFFERING |
				FILE_SYNCHRONOUS_IO_NONALERT,
				NULL,
				0
			);

			if (!NT_SUCCESS(status))
			{
				DbgPrint("FileDisk: File %.*S could not be created.\n", device_extension->file_name.Length / 2, device_extension->file_name.Buffer);
				ExFreePool(device_extension->file_name.Buffer);
				ExFreePool(device_extension->password.Buffer);

				return status;
			}

			if (Irp->IoStatus.Information == FILE_CREATED)
			{
				KdPrint(("FileDisk: File %.*S created.\n", device_extension->file_name.Length / 2, device_extension->file_name.Buffer));
				status = ZwFsControlFile(
					device_extension->file_handle,
					NULL,
					NULL,
					NULL,
					&Irp->IoStatus,
					FSCTL_SET_SPARSE,
					NULL,
					0,
					NULL,
					0
				);

				if (NT_SUCCESS(status))
				{
					KdPrint(("FileDisk: File attributes set to sparse.\n"));
				}

				file_eof.EndOfFile.QuadPart = diskParam->Size.QuadPart;

				status = ZwSetInformationFile(
					device_extension->file_handle,
					&Irp->IoStatus,
					&file_eof,
					sizeof(FILE_END_OF_FILE_INFORMATION),
					FileEndOfFileInformation
				);

				if (!NT_SUCCESS(status))
				{
					DbgPrint("FileDisk: eof could not be set.\n");
					ExFreePool(device_extension->file_name.Buffer);
					ExFreePool(device_extension->password.Buffer);

					ZwClose(device_extension->file_handle);

					return status;
				}
				KdPrint(("FileDisk: eof set to %I64u.\n", file_eof.EndOfFile.QuadPart));
			}
		}
	}
	else if (!NT_SUCCESS(status))
	{
		DbgPrint("FileDisk: File %.*S could not be opened.\n", device_extension->file_name.Length / 2, device_extension->file_name.Buffer);
		ExFreePool(device_extension->file_name.Buffer);
		ExFreePool(device_extension->password.Buffer);

		return status;
	}

	status = ZwQueryInformationFile(
		device_extension->file_handle,
		&Irp->IoStatus,
		&file_basic,
		sizeof(FILE_BASIC_INFORMATION),
		FileBasicInformation
	);

	if (!NT_SUCCESS(status))
	{
		ExFreePool(device_extension->file_name.Buffer);
		ExFreePool(device_extension->password.Buffer);

		ZwClose(device_extension->file_handle);

		return status;
	}

	if (file_basic.FileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED))
	{
		DbgPrint("FileDisk: Warning: File is compressed or encrypted. File attributes: %#x.\n", file_basic.FileAttributes);
	}

	status = ZwQueryInformationFile(
		device_extension->file_handle,
		&Irp->IoStatus,
		&file_standard,
		sizeof(FILE_STANDARD_INFORMATION),
		FileStandardInformation
	);

	if (!NT_SUCCESS(status))
	{
		ExFreePool(device_extension->file_name.Buffer);
		ExFreePool(device_extension->password.Buffer);

		ZwClose(device_extension->file_handle);

		return status;
	}

	if (diskParam->Size.QuadPart != file_standard.EndOfFile.QuadPart)
	{
		status = STATUS_UNRECOGNIZED_VOLUME;
		ExFreePool(device_extension->file_name.Buffer);
		ExFreePool(device_extension->password.Buffer);

		ZwClose(device_extension->file_handle);

		return status;
	}
	device_extension->file_size.QuadPart = file_standard.EndOfFile.QuadPart;
	status = ZwQueryInformationFile(
		device_extension->file_handle,
		&Irp->IoStatus,
		&file_alignment,
		sizeof(FILE_ALIGNMENT_INFORMATION),
		FileAlignmentInformation
	);

	if (!NT_SUCCESS(status))
	{
		ExFreePool(device_extension->file_name.Buffer);
		ExFreePool(device_extension->password.Buffer);

		ZwClose(device_extension->file_handle);

		return status;
	}

	DeviceObject->AlignmentRequirement = file_alignment.AlignmentRequirement;

	DeviceObject->Characteristics &= ~FILE_READ_ONLY_DEVICE;

	device_extension->media_in_device = TRUE;

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	RtlCopyMemory(DataOfMountManager.listOfDisks[device_extension->device_ID].FileName,
		diskParam->FileName,
		(diskParam->FileNameLength * sizeof(wchar_t)
			));

	DataOfMountManager.listOfDisks[device_extension->device_ID].FileNameLength = diskParam->FileNameLength;
	DataOfMountManager.listOfDisks[device_extension->device_ID].Letter = diskParam->Letter;
	DataOfMountManager.listOfDisks[device_extension->device_ID].Size = diskParam->Size;
	DataOfMountManager.amountOfMountedDisk.amount++;
	return STATUS_SUCCESS;
}

NTSTATUS FileDiskCloseFile(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

{
	PDEVICE_EXTENSION device_extension;

	ASSERT(DeviceObject != NULL);
	ASSERT(Irp != NULL);

	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ExFreePool(device_extension->file_name.Buffer);
	ExFreePool(device_extension->password.Buffer);

	ZwClose(device_extension->file_handle);

	device_extension->media_in_device = FALSE;
	DataOfMountManager.listOfDisks[device_extension->device_ID].Letter = L'-';

	DataOfMountManager.amountOfMountedDisk.amount--;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	return STATUS_SUCCESS;
}