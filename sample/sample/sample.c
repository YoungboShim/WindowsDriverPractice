#include <ntddk.h>
#include <math.h>

#define PENDING_SUPPORTED

typedef struct
{
	unsigned char Buffer[4];
	int DataSize;
#ifdef PENDING_SUPPORTED
	KTIMER Timer;
	KDPC Dpc;
	PIRP pPendingIrp;
#endif
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

#ifdef PENDING_SUPPORTED
void MyTimerDpcRoutine(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArg1, PVOID SystemArg2)
{
	DEVICE_EXTENSION* pDE = (DEVICE_EXTENSION*)DeferredContext;
	Dpc = Dpc;
	SystemArg1 = SystemArg1;
	SystemArg2 = SystemArg2;

	IoCompleteRequest(pDE->pPendingIrp, IO_NO_INCREMENT);
}
#endif

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

#ifdef PENDING_SUPPORTED
	pDE->pPendingIrp = pIrp;
	IoMarkIrpPending(pIrp);
	{
		LARGE_INTEGER pendingTime;
		pendingTime.QuadPart = -1 * 5 * 10000000;	// -1: relative time, 5: 5sec, 10^7: base time is 10^-7sec
		KeSetTimer(&pDE->Timer, pendingTime, &pDE->Dpc);
	}
	return STATUS_PENDING;
#else
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
#endif // PENDING_SUPPORTED
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
	NTSTATUS ntStatus;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SymbolicLinkName;
	PDEVICE_OBJECT DeviceObject = NULL;
	DEVICE_EXTENSION* pDE;

	pRegPath = pRegPath;

	pDrvObj->DriverUnload = SampleDriverUnload;

	pDrvObj->MajorFunction[IRP_MJ_CREATE] = MyCreateDispatch;
	pDrvObj->MajorFunction[IRP_MJ_CLOSE] = MyCloseDispatch;
	pDrvObj->MajorFunction[IRP_MJ_READ] = MyReadDispatch;

	// Create DeviceObject
	RtlInitUnicodeString(&DeviceName, L"\\Device\\SAMPLE");
	ntStatus = IoCreateDevice(pDrvObj, sizeof(DEVICE_EXTENSION), &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	DeviceObject->Flags |= DO_BUFFERED_IO;	// Use System Buffer 

	pDE = (DEVICE_EXTENSION * )DeviceObject->DeviceExtension;

#ifdef PENDING_SUPPORTED
	KeInitializeTimer(&pDE->Timer);
	KeInitializeDpc(&pDE->Dpc, MyTimerDpcRoutine, pDE);
#endif

	memcpy(pDE->Buffer, "HELLO", 5);
	pDE->DataSize = 5;

	// Create Symbolic name
	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	return STATUS_SUCCESS;
}