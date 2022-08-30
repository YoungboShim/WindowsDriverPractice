#include <ntddk.h>

#define TARGET_PHYSICALADDRESS	(ULONGLONG)(0x00000000000F0000)
#define MAPPING_SIZE			(0x10000) // 64KB

typedef struct
{
	unsigned char* pKernelLevelMappedVirtualAddress;
	unsigned char* pMappedUserLevelMappedVirtualAddress;
	PMDL pMdl;
}DEVICE_EXTENSION;

void SampleDriverUnload(PDRIVER_OBJECT pDrvObj)
{
	UNICODE_STRING SymbolicLinkName;
	DEVICE_EXTENSION* pDE = NULL;

	pDE = (DEVICE_EXTENSION*)pDrvObj->DeviceObject->DeviceExtension;

	MmUnmapIoSpace(pDE->pKernelLevelMappedVirtualAddress, MAPPING_SIZE);

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
	DEVICE_EXTENSION* pDE = NULL;

	pDE = (DEVICE_EXTENSION*)pDevObj->DeviceExtension;

	if (pDE->pMappedUserLevelMappedVirtualAddress)
	{
		MmUnmapLockedPages(pDE->pMappedUserLevelMappedVirtualAddress, pDE->pMdl);
		IoFreeMdl(pDE->pMdl);
		pDE->pMappedUserLevelMappedVirtualAddress = NULL;
		pDE->pMdl = NULL;
	}

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, 0); // 0 -> IO_NO_INCREMENT
	return STATUS_SUCCESS;
}

NTSTATUS MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DEVICE_EXTENSION* pDE = NULL;
	unsigned char** ppReturnBuffer = NULL;

	pDE = (DEVICE_EXTENSION*)pDevObj->DeviceExtension;
	ppReturnBuffer = (unsigned char**)pIrp->AssociatedIrp.SystemBuffer;

	if (pDE->pMappedUserLevelMappedVirtualAddress)
	{
		MmUnmapLockedPages(pDE->pMappedUserLevelMappedVirtualAddress, pDE->pMdl);
		IoFreeMdl(pDE->pMdl);
		pDE->pMappedUserLevelMappedVirtualAddress = NULL;
		pDE->pMdl = NULL;
	}

	pDE->pMdl = IoAllocateMdl(pDE->pKernelLevelMappedVirtualAddress, MAPPING_SIZE, FALSE, FALSE, NULL);
	if (pDE->pMdl)
	{
		MmBuildMdlForNonPagedPool(pDE->pMdl);
		*ppReturnBuffer = MmMapLockedPagesSpecifyCache(pDE->pMdl, UserMode, MmNonCached, NULL, FALSE, HighPagePriority);
		pDE->pMappedUserLevelMappedVirtualAddress = *ppReturnBuffer;
	}

	pIrp->IoStatus.Information = sizeof(unsigned char*);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, 0);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDrvObj, PUNICODE_STRING pRegPath)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	NTSTATUS ntStatus;
	UNICODE_STRING DeviceName;
	UNICODE_STRING SymbolicLinkName;
	DEVICE_EXTENSION* pDE;
	PHYSICAL_ADDRESS PhysicalAddress;	// PHYSICAL_ADDRESS -> typedef of LARGE_INTEGER -> 64 bit integer

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

	RtlInitUnicodeString(&SymbolicLinkName, L"\\DosDevices\\MYSAMPLE");
	ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);

	PhysicalAddress.QuadPart = TARGET_PHYSICALADDRESS;
	pDE->pKernelLevelMappedVirtualAddress = (unsigned char*) MmMapIoSpace(PhysicalAddress, MAPPING_SIZE, MmNonCached);
	pDE->pMappedUserLevelMappedVirtualAddress = NULL;
	pDE->pMdl = NULL;

	return STATUS_SUCCESS;
}