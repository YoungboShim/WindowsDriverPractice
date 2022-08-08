#include <ntddk.h>

WCHAR g_TempString[512] = { 0, };
void NotifyRoutine(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	Process = Process; ProcessId = ProcessId;
	if (CreateInfo == NULL)
		goto exit;

	memset(g_TempString, 0, 512 * sizeof(WCHAR));
	memcpy(g_TempString, CreateInfo->ImageFileName->Buffer, CreateInfo->ImageFileName->Length);
	_wcsupr(g_TempString);
	if (wcswcs(g_TempString, L"NOTEPAD.EXE"))
	{
		CreateInfo->CreationStatus = STATUS_UNSUCCESSFUL;
	}

exit:
	return;
}

void SampleDriverUnload(PDRIVER_OBJECT pDrvObj)
{
	pDrvObj = pDrvObj;
	PsSetCreateProcessNotifyRoutineEx(NotifyRoutine, TRUE);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegistryPath)
{
	pRegistryPath = pRegistryPath;

	pDrvObj->DriverUnload = SampleDriverUnload;

	PsSetCreateProcessNotifyRoutineEx(NotifyRoutine, FALSE);

	return STATUS_SUCCESS;
}