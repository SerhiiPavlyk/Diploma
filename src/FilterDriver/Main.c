#include "FsFilter.h"
#include "Vector.h"
//////////////////////////////////////////////////////////////////////////
// Function prototypes

VOID FsFilterUnload(
    __in PDRIVER_OBJECT DriverObject
    );

UNICODE_STRING gDeviceName = RTL_CONSTANT_STRING(L"\\Device\\FSFilter");

UNICODE_STRING gSymbolicLinkName = RTL_CONSTANT_STRING(L"\\Device\\FSFilterSym");

PDEVICE_OBJECT gDeviceObject = NULL;
//////////////////////////////////////////////////////////////////////////
// Global data

PDRIVER_OBJECT   g_fsFilterDriverObject = NULL;

UNICODE_STRING* g_blocked_extensions[ARRAY_SIZE];
UNICODE_STRING* g_backup_extensions[ARRAY_SIZE];

DYNAMIC_UNICODE_STRING_ARRAY* g_blocked_files;
DYNAMIC_UNICODE_STRING_ARRAY* g_backup_files;

FAST_IO_DISPATCH g_fastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),
    FsFilterFastIoCheckIfPossible,
    FsFilterFastIoRead,
    FsFilterFastIoWrite,
    FsFilterFastIoQueryBasicInfo,
    FsFilterFastIoQueryStandardInfo,
    FsFilterFastIoLock,
    FsFilterFastIoUnlockSingle,
    FsFilterFastIoUnlockAll,
    FsFilterFastIoUnlockAllByKey,
    FsFilterFastIoDeviceControl,
    NULL,
    NULL,
    FsFilterFastIoDetachDevice,
    FsFilterFastIoQueryNetworkOpenInfo,
    NULL,
    FsFilterFastIoMdlRead,
    FsFilterFastIoMdlReadComplete,
    FsFilterFastIoPrepareMdlWrite,
    FsFilterFastIoMdlWriteComplete,
    FsFilterFastIoReadCompressed,
    FsFilterFastIoWriteCompressed,
    FsFilterFastIoMdlReadCompleteCompressed,
    FsFilterFastIoMdlWriteCompleteCompressed,
    FsFilterFastIoQueryOpen,
    NULL,
    NULL,
    NULL,
};

//////////////////////////////////////////////////////////////////////////
// DriverEntry - Entry point of the driver

NTSTATUS DriverEntry(
    __inout PDRIVER_OBJECT  DriverObject,
    __in    PUNICODE_STRING RegistryPath
    )
{    
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    i      = 0;

    //ASSERT(FALSE); // This will break to debugger

    //
    // Store our driver object.
    //

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

    g_fsFilterDriverObject = DriverObject;
    
    //
    //  Initialize the driver object dispatch table.
    //

    for (i = 1; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i) 
    {
        DriverObject->MajorFunction[i] = FsFilterDispatchPassThrough;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FsFilterDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FsFilterDispatchControl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FsFilterDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FsFilterDispatchClose;
    //DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FsFilterDispatchControl;

    //
    // Set fast-io dispatch table.
    //

    DriverObject->FastIoDispatch = &g_fastIoDispatch;

    g_blocked_files = ExAllocatePoolWithTag(NonPagedPool, sizeof(DYNAMIC_UNICODE_STRING_ARRAY), 'Tag');
    if (!g_blocked_files)
    {
        // Handle error
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = InitializeDynamicArray(g_blocked_files);
    if (!NT_SUCCESS(status))
    {
        // Handle error
        ExFreePool(g_blocked_files);
        return status;
    }

    g_backup_files = ExAllocatePoolWithTag(NonPagedPool, sizeof(DYNAMIC_UNICODE_STRING_ARRAY), 'Tag');
    if (!g_backup_files)
    {
        // Handle error
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = InitializeDynamicArray(g_backup_files);
    if (!NT_SUCCESS(status))
    {
        // Handle error
        ExFreePool(g_backup_files);
        return status;
    }

    for (int j = 0; j < ARRAY_SIZE; ++j)
    {
        g_blocked_extensions[j] = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), 'Tag');
        if (!g_blocked_extensions[j])
        {
            // Error handling
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlInitUnicodeString(g_blocked_extensions[j], L".default");

        g_backup_extensions[j] = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), 'Tag');
        if (!g_backup_extensions[j])
        {
            // Error handling
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlInitUnicodeString(g_backup_extensions[j], L".default");
    }

    //
    //  Registered callback routine for file system changes.
    //

    status = IoRegisterFsRegistrationChange(DriverObject, FsFilterNotificationCallback); 
    if (!NT_SUCCESS(status)) 
    {
        return status;
    }

    //
    // Set driver unload routine (debug purpose only).
    //

    DriverObject->DriverUnload = FsFilterUnload;

    return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
// Unload routine

VOID FsFilterUnload(
    __in PDRIVER_OBJECT DriverObject
    )
{
    ULONG           numDevices = 0;
    ULONG           i          = 0;    
    LARGE_INTEGER   interval;
    PDEVICE_OBJECT  devList[DEVOBJ_LIST_SIZE];

    interval.QuadPart = (5 * DELAY_ONE_SECOND); //delay 5 seconds

    for (int j = 0; j < ARRAY_SIZE; ++j)
    {
        if (g_blocked_extensions[j])
        {
            ExFreePool(g_blocked_extensions[j]);
            g_blocked_extensions[j] = NULL;
        }
        if (g_backup_extensions[j])
        {
            ExFreePool(g_backup_extensions[j]);
            g_backup_extensions[j] = NULL;
        }
    }
    CleanupDynamicArray(g_blocked_files);
    ExFreePool(g_blocked_files);
    CleanupDynamicArray(g_backup_files);
    ExFreePool(g_backup_files);
    //
    //  Unregistered callback routine for file system changes.
    //

    IoUnregisterFsRegistrationChange(DriverObject, FsFilterNotificationCallback);

    //
    //  This is the loop that will go through all of the devices we are attached
    //  to and detach from them.
    //

    for (;;)
    {
        IoEnumerateDeviceObjectList(
            DriverObject,
            devList,
            sizeof(devList),
            &numDevices);

        if (0 == numDevices)
        {
            break;
        }

        numDevices = min(numDevices, RTL_NUMBER_OF(devList));

        for (i = 0; i < numDevices; ++i) 
        {
            FsFilterDetachFromDevice(devList[i]);
            ObDereferenceObject(devList[i]);
        }
        
        KeDelayExecutionThread(KernelMode, FALSE, &interval);
    }
}

//////////////////////////////////////////////////////////////////////////
// Misc

BOOLEAN FsFilterIsMyDeviceObject(
    __in PDEVICE_OBJECT DeviceObject
    )
{
    return DeviceObject->DriverObject == g_fsFilterDriverObject;
}
