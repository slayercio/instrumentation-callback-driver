#include <ntifs.h>
#include "ntkernel.h"
#include "ctl.h"


NTSTATUS RealEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	UNREFERENCED_PARAMETER(registry);

	UNICODE_STRING device_name = RTL_CONSTANT_STRING(L"\\Device\\Instrumentation");

	PDEVICE_OBJECT device = nullptr;
	NTSTATUS status = IoCreateDevice(driver, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device);

	if (status != STATUS_SUCCESS)
	{
		DebugPrint("[-] Failed to create device object!\n");

		return status;
	}

	DebugPrint("[+] Created device object!\n");

	UNICODE_STRING symbolic_name = RTL_CONSTANT_STRING(L"\\DosDevices\\Instrumentation");

	status = IoCreateSymbolicLink(&symbolic_name, &device_name);
	if (status != STATUS_SUCCESS)
	{
		DebugPrint("[-] Failed to create symbolic link!\n");
		
		return status;
	}

	DebugPrint("[+] Created symbolic link!\n");

	SetFlag(device->Flags, DO_BUFFERED_IO);

	driver->MajorFunction[IRP_MJ_CREATE] = driver::ioctl::create;
	driver->MajorFunction[IRP_MJ_CLOSE] = driver::ioctl::close;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::ioctl::device_control;

	ClearFlag(device->Flags, DO_DEVICE_INITIALIZING);

	DebugPrint("[+] Device initialized!\n");

	return status;
}

NTSTATUS DriverEntry()
{
	UNICODE_STRING driver_name = RTL_CONSTANT_STRING(L"\\Driver\\Instrumentation");

	return IoCreateDriver(&driver_name, RealEntry);
}