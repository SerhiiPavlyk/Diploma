#pragma once
#include "IRP.h"

typedef struct MManager
{
	UINT32 gMountedDiskCount;
	PDRIVER_OBJECT DriverObject;
	DISK_PARAMETERS listOfDisks[MAX_DISK];
	MountDisksAmount amountOfMountedDisk;
} MountManager, * PMountManager;


MountManager DataOfMountManager;

VOID MountManagerInit(PDRIVER_OBJECT DriverObject);

NTSTATUS MountManagerCreateDevice();

NTSTATUS FileDiskCreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS FileDiskOpenFile(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS FileDiskCloseFile(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
