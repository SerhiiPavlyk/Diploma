#include "pch.h"

#include "ConfigUpdater.h"
#include "Logger.h"

SERVICE_STATUS        g_ServiceStatus = { 0 };
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD);
DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);

#define SERVICE_NAME  _T("SHSService")

static std::unique_ptr<ConfigUpdater> g_updater = nullptr;
static std::unique_ptr <Logger> g_logger = nullptr;
VOID WINAPI ServiceCtrlHandler(DWORD CtrlCode)
{


    switch (CtrlCode)
    {
    case SERVICE_CONTROL_STOP:

        g_logger->Debug("Service Stop");

        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING)
            break;

        g_updater->UnmockAllBackupDisks();

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_ServiceStatus.dwWin32ExitCode = 0;
        g_ServiceStatus.dwCheckPoint = 4;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: SetServiceStatus returned error"));
        }

        // This will signal the worker thread to start shutting down
        SetEvent(g_ServiceStopEvent);

        break;

    default:
        break;
    }

    OutputDebugString(_T("My Sample Service: ServiceCtrlHandler: Exit"));
}

DWORD WINAPI ServiceUpdateThread(LPVOID lpParam)
{
    //int backup_time = 900000;
    int backup_time = 5 * 60000; //60000 = 1 min
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {
        g_updater->BackupAllFiles();
        g_updater->BlockAllFiles();
        Sleep(backup_time);// Wait 
    }
    return ERROR_SUCCESS;
}


DWORD WINAPI ServiceWorkerThread(LPVOID lpParam)
{
    OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Entry"));
#ifdef DEBUG
    while (!IsDebuggerPresent())
    {
        Sleep(1000);
    }
#endif

    g_updater->CreateAllBackupDisks();
    g_updater->SendExtensionsToDriver();
    HANDLE hThread;
    // Start the thread that will perform the main task of the service
    hThread = CreateThread(NULL, 0, ServiceUpdateThread, NULL, 0, NULL);

    //  Periodically check if the service has been requested to stop
    while (WaitForSingleObject(g_ServiceStopEvent, 0) != WAIT_OBJECT_0)
    {


         //  Simulate some work by sleeping
        Sleep(3000);
    }

    OutputDebugString(_T("My Sample Service: ServiceWorkerThread: Exit"));

    return ERROR_SUCCESS;
}

VOID WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    DWORD Status = E_FAIL;

    OutputDebugString(_T("My Sample Service: ServiceMain: Entry"));

    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    HANDLE hThread;
    if (g_StatusHandle == NULL)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: RegisterServiceCtrlHandler returned error"));
        goto EXIT;
    }

    // Tell the service controller we are starting
    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    /*
     * Perform tasks neccesary to start the service here
     */
    OutputDebugString(_T("My Sample Service: ServiceMain: Performing Service Start Operations"));

    // Create stop event to wait on later.
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: CreateEvent(g_ServiceStopEvent) returned error"));

        g_ServiceStatus.dwControlsAccepted = 0;
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        g_ServiceStatus.dwWin32ExitCode = GetLastError();
        g_ServiceStatus.dwCheckPoint = 1;

        if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
        {
            OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
        }
        goto EXIT;
    }

    // Tell the service controller we are started
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 0;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

    // Start the thread that will perform the main task of the service
    hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);

    OutputDebugString(_T("My Sample Service: ServiceMain: Waiting for Worker Thread to complete"));

    // Wait until our worker thread exits effectively signaling that the service needs to stop
    WaitForSingleObject(hThread, INFINITE);

    OutputDebugString(_T("My Sample Service: ServiceMain: Worker Thread Stop Event signaled"));


    /*
     * Perform any cleanup tasks
     */
    OutputDebugString(_T("My Sample Service: ServiceMain: Performing Cleanup Operations"));

    CloseHandle(g_ServiceStopEvent);

    g_ServiceStatus.dwControlsAccepted = 0;
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    g_ServiceStatus.dwWin32ExitCode = 0;
    g_ServiceStatus.dwCheckPoint = 3;

    if (SetServiceStatus(g_StatusHandle, &g_ServiceStatus) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: ServiceMain: SetServiceStatus returned error"));
    }

EXIT:
    OutputDebugString(_T("My Sample Service: ServiceMain: Exit"));

    return;
}



int _tmain(int argc, TCHAR* argv[])
{

    OutputDebugString(_T("My Sample Service: Main: Entry"));

    std::ofstream file("C:\\SeekAndHideSecureService.log", std::ofstream::app);
    
    g_logger.reset(new Logger(file));
    g_updater.reset(new ConfigUpdater(g_logger.get()));
    wchar_t service_name_buffer[] = SERVICE_NAME;
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {service_name_buffer, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
        {NULL, NULL}
    };

    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        OutputDebugString(_T("My Sample Service: Main: StartServiceCtrlDispatcher returned error"));
        return GetLastError();
    }

    OutputDebugString(_T("My Sample Service: Main: Exit"));
    return 0;
}