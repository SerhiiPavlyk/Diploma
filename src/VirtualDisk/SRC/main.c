#pragma warning (disable:28251)

#include "pch.h"
#include "MountManager.h"

#pragma comment(lib, "wdmsec.lib")

PDEVICE_OBJECT gDeviceObject = NULL;

UNICODE_STRING gDeviceName = RTL_CONSTANT_STRING(L"\\Device\\VirtualEncryptedDisk");

UNICODE_STRING gSymbolicLinkName = RTL_CONSTANT_STRING(L"\\Device\\VirtualEncryptedDiskSym");

VOID UnloadDriver(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	IoDeleteSymbolicLink(&gSymbolicLinkName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS FileDiskDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PDEVICE_EXTENSION   device_extension;
	PIO_STACK_LOCATION	ioStack;
	NTSTATUS            status;

	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ioStack = IoGetCurrentIrpStackLocation(Irp);

	if ((DeviceObject == gDeviceObject)
		&& (ioStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_FILE_DISK_GET_ALL_DISK)
		&& (ioStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_FILE_DISK_GET_AMOUNT_OF_MOUNTED_DISKS)
		&& (ioStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_FILE_DISK_GET_FREE_ID))
	{
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	else if (ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FILE_DISK_GET_AMOUNT_OF_MOUNTED_DISKS)
	{
		MountDisksAmount* response = (MountDisksAmount*)Irp->AssociatedIrp.SystemBuffer;
		response->amount = DataOfMountManager.amountOfMountedDisk.amount;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(MountDisksAmount);
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	else if (ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FILE_DISK_GET_ALL_DISK)
	{
		PIO_STACK_LOCATION irpStack;
		// Get the current I/O stack location
		irpStack = IoGetCurrentIrpStackLocation(Irp);
		USHORT totalLenOfNames = 0;
		PVOID buffer;
		for (size_t i = 0; i < DataOfMountManager.amountOfMountedDisk.amount; i++)
		{
			totalLenOfNames += DataOfMountManager.listOfDisks[i].FileNameLength;
		}
		ULONG outsize = DataOfMountManager.amountOfMountedDisk.amount * sizeof(Response) + totalLenOfNames * sizeof(wchar_t);
		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < outsize)
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;
			return status;
		}
		// Get the input/output buffers and their lengths
		buffer = Irp->AssociatedIrp.SystemBuffer;

		*(ULONG32*)buffer = DataOfMountManager.amountOfMountedDisk.amount;

		Response* response = (Response*)((char*)buffer + sizeof(ULONG32));

		// Copy the response data into the output buffer
		for (size_t i = 0, j = 0; i < DataOfMountManager.gMountedDiskCount; i++)
		{
			if (DataOfMountManager.listOfDisks[i].Letter != L'-')
			{
				RtlCopyMemory(response[j].FileName, DataOfMountManager.listOfDisks[i].FileName, DataOfMountManager.listOfDisks[i].FileNameLength * sizeof(wchar_t));
				response[j].FileNameLength = DataOfMountManager.listOfDisks[i].FileNameLength;
				response[j].Letter = DataOfMountManager.listOfDisks[i].Letter;
				response[j].Size = DataOfMountManager.listOfDisks[i].Size;
				j++;
			}
		}


		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = outsize;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	else if ((ioStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_FILE_DISK_GET_FREE_ID))
	{
		MountDisksAmount* response = (MountDisksAmount*)Irp->AssociatedIrp.SystemBuffer;
		UINT32 i = 0;
		for (; i < DataOfMountManager.gMountedDiskCount; i++)
		{
			if (DataOfMountManager.listOfDisks[i].Letter == L'-')
			{
				break;
			}
		}
		response->amount = i;
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(MountDisksAmount);

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}


	if (!device_extension->media_in_device &&
		ioStack->Parameters.DeviceIoControl.IoControlCode !=
		IOCTL_FILE_DISK_OPEN_FILE)
	{
		Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_NO_MEDIA_IN_DEVICE;
	}

	switch (ioStack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IOCTL_FILE_DISK_OPEN_FILE:
	{
		SECURITY_QUALITY_OF_SERVICE security_quality_of_service;

		if (device_extension->media_in_device)
		{
			KdPrint(("FileDisk: IOCTL_FILE_DISK_OPEN_FILE: Media already opened.\n"));

			status = STATUS_INVALID_DEVICE_REQUEST;
			Irp->IoStatus.Information = 0;

			break;
		}

		if (ioStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DISK_PARAMETERS))
		{
			status = STATUS_INVALID_PARAMETER;
			Irp->IoStatus.Information = 0;

			break;
		}

		if (ioStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(DISK_PARAMETERS) +
			((PDISK_PARAMETERS)Irp->AssociatedIrp.SystemBuffer)->FileNameLength - sizeof(UCHAR))
		{
			status = STATUS_INVALID_PARAMETER;
			Irp->IoStatus.Information = 0;

			break;
		}

		if (device_extension->security_client_context != NULL)
		{
			SeDeleteClientSecurity(device_extension->security_client_context);
		}
		else
		{
			device_extension->security_client_context = ExAllocatePoolWithTag(NonPagedPool, sizeof(SECURITY_CLIENT_CONTEXT), DISK_TAG);
		}

		RtlZeroMemory(&security_quality_of_service, sizeof(SECURITY_QUALITY_OF_SERVICE));

		security_quality_of_service.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
		security_quality_of_service.ImpersonationLevel = SecurityImpersonation;
		security_quality_of_service.ContextTrackingMode = SECURITY_STATIC_TRACKING;
		security_quality_of_service.EffectiveOnly = FALSE;

		SeCreateClientSecurity(
			PsGetCurrentThread(),
			&security_quality_of_service,
			FALSE,
			device_extension->security_client_context
		);

		IoMarkIrpPending(Irp);

		ExInterlockedInsertTailList(
			&device_extension->list_head,
			&Irp->Tail.Overlay.ListEntry,
			&device_extension->list_lock
		);

		KeSetEvent(&device_extension->request_event, (KPRIORITY)0, FALSE);



		status = STATUS_PENDING;

		break;
	}

	case IOCTL_FILE_DISK_CLOSE_FILE:
	{
		IoMarkIrpPending(Irp);

		ExInterlockedInsertTailList(
			&device_extension->list_head,
			&Irp->Tail.Overlay.ListEntry,
			&device_extension->list_lock
		);

		KeSetEvent(&device_extension->request_event, (KPRIORITY)0, FALSE);


		status = STATUS_PENDING;

		break;
	}

	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_CDROM_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY2:
	{
		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		break;
	}

	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
	case IOCTL_CDROM_GET_DRIVE_GEOMETRY:
	{
		PDISK_GEOMETRY  disk_geometry;
		ULONGLONG       length;
		ULONG           sector_size;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;

			break;
		}

		disk_geometry = (PDISK_GEOMETRY)Irp->AssociatedIrp.SystemBuffer;

		length = device_extension->file_size.QuadPart;

		sector_size = 512;

		disk_geometry->Cylinders.QuadPart = length / sector_size / 32 / 2;
		disk_geometry->MediaType = FixedMedia;
		disk_geometry->TracksPerCylinder = 2;
		disk_geometry->SectorsPerTrack = 32;
		disk_geometry->BytesPerSector = sector_size;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

		break;
	}

	case IOCTL_DISK_GET_LENGTH_INFO:
	{
		PGET_LENGTH_INFORMATION get_length_information;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;

			break;
		}

		get_length_information = (PGET_LENGTH_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

		get_length_information->Length.QuadPart = device_extension->file_size.QuadPart;

		status = STATUS_SUCCESS;

		Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

		break;
	}

	case IOCTL_DISK_GET_PARTITION_INFO:
	{
		PPARTITION_INFORMATION  partition_information;
		ULONGLONG               length;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;
			break;
		}

		partition_information = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

		length = device_extension->file_size.QuadPart;

		partition_information->StartingOffset.QuadPart = 0;
		partition_information->PartitionLength.QuadPart = length;
		partition_information->HiddenSectors = 1;
		partition_information->PartitionNumber = 0;
		partition_information->PartitionType = 0;
		partition_information->BootIndicator = FALSE;
		partition_information->RecognizedPartition = FALSE;
		partition_information->RewritePartition = FALSE;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);

		break;
	}

	case IOCTL_DISK_GET_PARTITION_INFO_EX:
	{
		PPARTITION_INFORMATION_EX   partition_information_ex;
		ULONGLONG                   length;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION_EX))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;
			break;
		}

		partition_information_ex = (PPARTITION_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;

		length = device_extension->file_size.QuadPart;

		partition_information_ex->PartitionStyle = PARTITION_STYLE_MBR;
		partition_information_ex->StartingOffset.QuadPart = 0;
		partition_information_ex->PartitionLength.QuadPart = length;
		partition_information_ex->PartitionNumber = 0;
		partition_information_ex->RewritePartition = FALSE;
		partition_information_ex->Mbr.PartitionType = 0;
		partition_information_ex->Mbr.BootIndicator = FALSE;
		partition_information_ex->Mbr.RecognizedPartition = FALSE;
		partition_information_ex->Mbr.HiddenSectors = 1;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);

		break;
	}

	case IOCTL_DISK_IS_WRITABLE:
	{
		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		break;
	}

	case IOCTL_DISK_MEDIA_REMOVAL:
	case IOCTL_STORAGE_MEDIA_REMOVAL:
	{
		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		break;
	}

	case IOCTL_DISK_SET_PARTITION_INFO:
	{
		if (ioStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SET_PARTITION_INFORMATION))
		{
			status = STATUS_INVALID_PARAMETER;
			Irp->IoStatus.Information = 0;

			break;
		}

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;

		break;
	}

	case IOCTL_DISK_VERIFY:
	{
		PVERIFY_INFORMATION verify_information;

		if (ioStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(VERIFY_INFORMATION))
		{
			status = STATUS_INVALID_PARAMETER;
			Irp->IoStatus.Information = 0;

			break;
		}

		verify_information = (PVERIFY_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = verify_information->Length;

		break;
	}

	case IOCTL_STORAGE_GET_DEVICE_NUMBER:
	{
		PSTORAGE_DEVICE_NUMBER number;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DEVICE_NUMBER))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;
			break;
		}

		number = (PSTORAGE_DEVICE_NUMBER)Irp->AssociatedIrp.SystemBuffer;

		number->DeviceType = FILE_DEVICE_DISK;
		number->DeviceNumber = device_extension->device_ID;
		number->PartitionNumber = (ULONG)-1;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(STORAGE_DEVICE_NUMBER);

		break;
	}

	case IOCTL_STORAGE_GET_HOTPLUG_INFO:
	{
		PSTORAGE_HOTPLUG_INFO info;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_HOTPLUG_INFO))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;

			break;
		}

		info = (PSTORAGE_HOTPLUG_INFO)Irp->AssociatedIrp.SystemBuffer;

		info->Size = sizeof(STORAGE_HOTPLUG_INFO);
		info->MediaRemovable = 0;
		info->MediaHotplug = 0;
		info->DeviceHotplug = 0;
		info->WriteCacheEnableOverride = 0;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);

		break;
	}

	case IOCTL_VOLUME_GET_GPT_ATTRIBUTES:
	{
		PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION attr;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;

			break;
		}

		attr = (PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

		attr->GptAttributes = 0;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);

		break;
	}

	case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
	{
		PVOLUME_DISK_EXTENTS ext;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_DISK_EXTENTS))
		{
			status = STATUS_INVALID_PARAMETER;
			Irp->IoStatus.Information = 0;

			break;
		}

		ext = (PVOLUME_DISK_EXTENTS)Irp->AssociatedIrp.SystemBuffer;

		ext->NumberOfDiskExtents = 1;
		ext->Extents[0].DiskNumber = device_extension->device_ID;
		ext->Extents[0].StartingOffset.QuadPart = 0;
		ext->Extents[0].ExtentLength.QuadPart = device_extension->file_size.QuadPart;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(VOLUME_DISK_EXTENTS);

		break;
	}

#if (NTDDI_VERSION < NTDDI_VISTA)
#define IOCTL_DISK_IS_CLUSTERED CTL_CODE(IOCTL_DISK_BASE, 0x003e, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif  // NTDDI_VERSION < NTDDI_VISTA

	case IOCTL_DISK_IS_CLUSTERED:
	{
		PBOOLEAN clus;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN))
		{
			status = STATUS_BUFFER_TOO_SMALL;
			Irp->IoStatus.Information = 0;

			break;
		}

		clus = (PBOOLEAN)Irp->AssociatedIrp.SystemBuffer;

		*clus = FALSE;

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(BOOLEAN);

		break;
	}

	case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
	{
		PMOUNTDEV_NAME name;

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTDEV_NAME))
		{
			status = STATUS_INVALID_PARAMETER;
			Irp->IoStatus.Information = 0;

			break;
		}

		name = (PMOUNTDEV_NAME)Irp->AssociatedIrp.SystemBuffer;
		name->NameLength = device_extension->device_name.Length * sizeof(WCHAR);

		if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < name->NameLength + sizeof(USHORT))
		{
			status = STATUS_BUFFER_OVERFLOW;
			Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);

			break;
		}

		RtlCopyMemory(name->Name, device_extension->device_name.Buffer, name->NameLength);

		status = STATUS_SUCCESS;
		Irp->IoStatus.Information = name->NameLength + sizeof(USHORT);

		break;
	}

	case IOCTL_DISK_GET_MEDIA_TYPES:
	{
		KdPrint(("FileDisk: Unhandled ioctl IOCTL_DISK_GET_MEDIA_TYPES.\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;

		break;
	}
	case 0x66001b:
	{
		KdPrint(("FileDisk: Unhandled ioctl FT_BALANCED_READ_MODE.\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;

		break;
	}
	case IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES:
	{
		KdPrint(("FileDisk: Unhandled ioctl IOCTL_STORAGE_MANAGE_DATA_SET_ATTRIBUTES.\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;

		break;
	}
	case IOCTL_STORAGE_QUERY_PROPERTY:
	{
		KdPrint(("FileDisk: Unhandled ioctl IOCTL_STORAGE_QUERY_PROPERTY.\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;

		break;
	}

#if (NTDDI_VERSION < NTDDI_VISTA)
#define IOCTL_VOLUME_QUERY_ALLOCATION_HINT CTL_CODE(IOCTL_VOLUME_BASE, 20, METHOD_OUT_DIRECT, FILE_READ_ACCESS)
#endif  // NTDDI_VERSION < NTDDI_VISTA

	case IOCTL_VOLUME_QUERY_ALLOCATION_HINT:
	{
		KdPrint(("FileDisk: Unhandled ioctl IOCTL_VOLUME_QUERY_ALLOCATION_HINT.\n"));
		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;

		break;
	}
	default:
	{
		KdPrint(("FileDisk: Unknown IoControlCode %#x\n", ioStack->Parameters.DeviceIoControl.IoControlCode));

		status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Information = 0;
	}
	}

	if (status != STATUS_PENDING)
	{
		Irp->IoStatus.Status = status;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return status;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = UnloadDriver;

	NTSTATUS status = STATUS_SUCCESS;

	status = IoCreateDevice(DriverObject,
		0,
		&gDeviceName,
		FILE_DEVICE_NULL,
		0,
		FALSE,
		&gDeviceObject);

	if (status != STATUS_SUCCESS)
	{
		DbgPrintEx(0, 0, "IoCreateDevice fail!\n");
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	status = IoCreateSymbolicLink(&gSymbolicLinkName, &gDeviceName);

	if (status != STATUS_SUCCESS)
	{
		DbgPrintEx(0, 0, "IoCreateSymbolicLink fail!\n");
		return STATUS_FAILED_DRIVER_ENTRY;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = FileDiskCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = FileDiskCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = FileDiskReadWrite;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = FileDiskReadWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FileDiskDeviceControl;

	MountManagerInit(DriverObject);

	for (int i = 0; i < MAX_DISK; ++i)
	{
		MountManagerCreateDevice();
	}



	return status;
}
