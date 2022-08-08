#include <ntddk.h>

void SampleDriverUnload(PDRIVER_OBJECT pDrvObj)
{
	UNICODE_STRING SymbolicLinkName;

	pDrvObj = pDrvObj;

	// Delete symoblic name
	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	IoDeleteSymbolicLink(&SymbolicLinkName);

	// Delete DeviceObject
	IoDeleteDevice(pDrvObj->DeviceObject);

	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING RegPath)
{
	NTSTATUS ntStatus;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SymbolicLinkName;
	PDEVICE_OBJECT DeviceObject = NULL;

	RegPath = RegPath;

	pDrvObj->DriverUnload = SampleDriverUnload;

	// Create DeviceObject
	RtlInitUnicodeString(&DeviceName, L"\\Device\\SAMPLE");
	ntStatus = IoCreateDevice(pDrvObj, 0, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

	// Create Symbolic name
	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	return STATUS_SUCCESS;
}