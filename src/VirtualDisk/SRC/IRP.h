#pragma once

#include "pch.h"

VOID IOCTLHandle(IN PVOID Context);

NTSTATUS FileDiskAdjustPrivilege(IN ULONG Privilege, IN BOOLEAN  Enable);

NTSTATUS FileDiskReadWrite(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS FileDiskOpenFile(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS FileDiskCloseFile(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);