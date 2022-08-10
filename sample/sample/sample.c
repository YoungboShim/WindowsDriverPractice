#include <ntddk.h>

typedef struct
{
	unsigned char Buffer[4];
	int DataSize;
} DEVICE_EXTENSION;

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

NTSTATUS MyCreateDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pDevObj = pDevObj;
	pIrp = pIrp;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS MyCloseDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pDevObj = pDevObj;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION pStack;
	DEVICE_EXTENSION* pDE;
	int Length;
	unsigned char* pSystemBuffer;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	pDE = pDevObj->DeviceExtension;
	Length = pStack->Parameters.Read.Length;
	if (Length > pDE->DataSize) Length = pDE->DataSize;
	pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;

	memcpy(pSystemBuffer, pDE->Buffer, Length);

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = Length;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS MyWriteDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	PIO_STACK_LOCATION pStack;
	DEVICE_EXTENSION* pDE;
	int Length;
	unsigned char* pSystemBuffer;

	pStack = IoGetCurrentIrpStackLocation(pIrp);
	pDE = pDevObj->DeviceExtension;
	Length = pStack->Parameters.Write.Length;
	if (Length > 4) Length = 4;
	pSystemBuffer = pIrp->AssociatedIrp.SystemBuffer;

	memcpy(pDE->Buffer, pSystemBuffer, Length);
	pDE->DataSize = Length;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = Length;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING RegPath)
{
	NTSTATUS ntStatus;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SymbolicLinkName;
	PDEVICE_OBJECT DeviceObject = NULL;
	DEVICE_EXTENSION* pDE;

	RegPath = RegPath;

	pDrvObj->DriverUnload = SampleDriverUnload;

	pDrvObj->MajorFunction[IRP_MJ_CREATE] = MyCreateDispatch;
	pDrvObj->MajorFunction[IRP_MJ_CLOSE] = MyCloseDispatch;
	pDrvObj->MajorFunction[IRP_MJ_READ] = MyReadDispatch;
	pDrvObj->MajorFunction[IRP_MJ_WRITE] = MyWriteDispatch;

	// Create DeviceObject
	RtlInitUnicodeString(&DeviceName, L"\\Device\\SAMPLE");
	ntStatus = IoCreateDevice(pDrvObj, sizeof(DEVICE_EXTENSION), &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	pDE = (DEVICE_EXTENSION * )DeviceObject->DeviceExtension;
	pDE->DataSize = 0; // init
	DeviceObject->Flags |= DO_BUFFERED_IO;

	// Create Symbolic name
	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	return STATUS_SUCCESS;
}