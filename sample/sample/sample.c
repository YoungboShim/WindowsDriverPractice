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

VOID MyCancelRoutine(PDEVICE_OBJECT DeviceObj, PIRP pIrp);

void MyStartIo(DEVICE_EXTENSION* pDE, PIRP pIrp)
{
	LARGE_INTEGER Result;
	KIRQL OldIrql;

	IoAcquireCancelSpinLock(&OldIrql);

	if (pDE->pCurrentIrp != pIrp)
	{
		IoReleaseCancelSpinLock(OldIrql);
		goto exit;
	}

	IoSetCancelRoutine(pDE->pCurrentIrp, NULL);
	IoReleaseCancelSpinLock(OldIrql);

	Result.QuadPart = -1 * 5 * 10000000; // 5 sec
	KeSetTimer(&pDE->Timer, Result, &pDE->Dpc);

	IoSetCancelRoutine(pDE->pCurrentIrp, MyCancelRoutine);

exit:
	return;
}

void MyStartPacket(DEVICE_EXTENSION* pDE, PIRP pIrp)
{
	KIRQL OldIrql;

	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(pIrp, MyCancelRoutine);

	if (pDE->pCurrentIrp)
	{
		ExInterlockedInsertTailList(&pDE->ListHead, &pIrp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry, &pDE->ListSpinLock);
		IoReleaseCancelSpinLock(OldIrql);
	}
	else
	{
		pDE->pCurrentIrp = pIrp;
		IoReleaseCancelSpinLock(OldIrql);
		MyStartIo(pDE, pIrp);
	}
}

void MyStartNextPacket(DEVICE_EXTENSION* pDE)
{
	LIST_ENTRY* pListEntry;
	PIRP pIrp;
	KIRQL OldIrql;

	IoAcquireCancelSpinLock(&OldIrql);
	pDE->pCurrentIrp = NULL;
	pListEntry = ExInterlockedRemoveHeadList(&pDE->ListHead, &pDE->ListSpinLock);
	if (pListEntry)
	{
		pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.DeviceQueueEntry.DeviceListEntry);
		pDE->pCurrentIrp = pIrp;
		IoReleaseCancelSpinLock(OldIrql);
		MyStartIo(pDE, pIrp);
	}
	else
	{
		IoReleaseCancelSpinLock(OldIrql);
	}
}

VOID MyTimerDpcRoutine(struct _KDPC* Dpc, PVOID  DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
	DEVICE_EXTENSION* pDE = DeferredContext;
	PIRP pIrp;

	Dpc = Dpc;
	SystemArgument1 = SystemArgument1;
	SystemArgument2 = SystemArgument2;

	pIrp = pDE->pCurrentIrp;
	if (pIrp)
	{
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		pIrp->IoStatus.Information = 0;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}

	MyStartNextPacket(pDE);
	return;
}

VOID MyCancelRoutine(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DEVICE_EXTENSION* pDE;
	pDE = (DEVICE_EXTENSION*)pDevObj->DeviceExtension;

	if (pDE->pCurrentIrp == pIrp)
	{
		KeCancelTimer(&pDE->Timer);
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
		MyStartNextPacket(pDE);
	}
	else
	{
		RemoveEntryList(&pIrp->Tail.Overlay.DeviceQueueEntry.DeviceListEntry);
		IoReleaseCancelSpinLock(pIrp->CancelIrql);
	}

	pIrp->IoStatus.Status = STATUS_CANCELLED;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, 0); // 0 -> IO_NO_INCREMENT
}

NTSTATUS MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DEVICE_EXTENSION* pDE;
	pDE = (DEVICE_EXTENSION*)pDevObj->DeviceExtension;
	IoMarkIrpPending(pIrp);
	MyStartPacket(pDE, pIrp);

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