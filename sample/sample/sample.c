#include <ntddk.h>

typedef struct
{
	KDPC Dpc;
	KTIMER Timer;

	KSPIN_LOCK ListSpinLock;
	LIST_ENTRY ListHead;
	PIRP pCurrentIrp;
}DEVICE_EXTENSION;

void SampleDriverUnload(PDRIVER_OBJECT pDrvObj)
{
	UNICODE_STRING SymbolicLinkName;

	pDrvObj = pDrvObj;

	// Remove symbolic link
	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	IoDeleteSymbolicLink(&SymbolicLinkName);
	
	// Remove device object
	IoDeleteDevice(pDrvObj->DeviceObject);
}

NTSTATUS MyCreateDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pDevObj = pDevObj;
	pIrp = pIrp;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, 0); // 0 -> IO_NO_INCREMENT
	return STATUS_SUCCESS;
}

NTSTATUS MyCloseDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	pDevObj = pDevObj;
	pIrp = pIrp;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, 0); // 0 -> IO_NO_INCREMENT
	return STATUS_SUCCESS;
}

void MyStartIo(DEVICE_EXTENSION* pDE)
{
	LARGE_INTEGER Result;
	Result.QuadPart = -1 * 5 * 10000000; // 5 sec
	KeSetTimer(&pDE->Timer, Result, &pDE->Dpc);
}

VOID MyTimerDpcRoutine(struct _KDPC* Dpc, PVOID  DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	DEVICE_EXTENSION* pDE = DeferredContext;
	PLIST_ENTRY pListEntry = NULL;
	Dpc = Dpc;
	SystemArgument1 = SystemArgument1;
	SystemArgument2 = SystemArgument2;

	if (pDE->pCurrentIrp)
	{
		IoCompleteRequest(pDE->pCurrentIrp, IO_NO_INCREMENT);
		pDE->pCurrentIrp = NULL;
	}

	if (!IsListEmpty(&pDE->ListHead))
	{
		pListEntry = ExInterlockedRemoveHeadList(&pDE->ListHead, &pDE->ListSpinLock);

		pDE->pCurrentIrp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.DeviceQueueEntry.DeviceListEntry);

		MyStartIo(pDE);
	}
}

NTSTATUS MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DEVICE_EXTENSION* pDE;
	KIRQL OldIrql;

	pDE = (DEVICE_EXTENSION*)pDevObj->DeviceExtension;

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;

	IoMarkIrpPending(pIrp);

	if (pDE->pCurrentIrp)
	{
		ExInterlockedInsertTailList(&pDE->ListHead, &pIrp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry, &pDE->ListSpinLock);
	}
	else
	{
		KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

		pDE->pCurrentIrp = pIrp;
		MyStartIo(pDE);

		KeLowerIrql(OldIrql);
	}

	return STATUS_PENDING;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS ntStatus;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SymbolicLinkName;
	DEVICE_EXTENSION* pDE;

	pRegPath = pRegPath;

	pDrvObj->MajorFunction[IRP_MJ_CREATE] = MyCreateDispatch;
	pDrvObj->MajorFunction[IRP_MJ_CLOSE] = MyCloseDispatch;
	pDrvObj->MajorFunction[IRP_MJ_READ] = MyReadDispatch;

	pDrvObj->DriverUnload = SampleDriverUnload;

	// Create DeviceObject
	RtlInitUnicodeString(&DeviceName, L"\\Device\\SAMPLE"); // Create symbolic name
	ntStatus = IoCreateDevice(pDrvObj, sizeof(DEVICE_EXTENSION), &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
	DeviceObject->Flags |= DO_BUFFERED_IO; // Use system buffer

	pDE = (DEVICE_EXTENSION*)DeviceObject->DeviceExtension;

	KeInitializeTimer(&pDE->Timer);
	KeInitializeDpc(&pDE->Dpc, MyTimerDpcRoutine, pDE);

	KeInitializeSpinLock(&pDE->ListSpinLock);

	InitializeListHead(&pDE->ListHead);

	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	return STATUS_SUCCESS;
}