#include <ntddk.h>

#define TARGET_PHYSICALADDRESS	(ULONGLONG)(0x0000000000000000)
#define MAPPING_SIZE			(0x1000) // 4KB

typedef struct
{
	unsigned char* pKernelLevelMappedVirtualAddress;
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
	pDevObj = pDevObj;
	pIrp = pIrp;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, 0); // 0 -> IO_NO_INCREMENT
	return STATUS_SUCCESS;
}

NTSTATUS MyReadDispatch(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
	DEVICE_EXTENSION* pDE = NULL;
	unsigned char** ppSystemBuffer = NULL;

	pDE = (DEVICE_EXTENSION*)pDevObj->DeviceExtension;

	ppSystemBuffer = (unsigned char**)pIrp->AssociatedIrp.SystemBuffer;
	*ppSystemBuffer = pDE->pKernelLevelMappedVirtualAddress;
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

	return STATUS_SUCCESS;
}