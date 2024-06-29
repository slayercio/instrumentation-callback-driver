#include "ctl.h"
#include "ntkernel.h"

constexpr auto INSTRUMENTATION_CALLBACK_OFFSET = 0x03D8;

NTSTATUS driver::ioctl::create(PDEVICE_OBJECT device, PIRP pirp)
{
	UNREFERENCED_PARAMETER(device);

	IoCompleteRequest(pirp, IO_NO_INCREMENT);

	return pirp->IoStatus.Status;
}

NTSTATUS driver::ioctl::close(PDEVICE_OBJECT device, PIRP pirp)
{
	UNREFERENCED_PARAMETER(device);

	IoCompleteRequest(pirp, IO_NO_INCREMENT);

	return pirp->IoStatus.Status;
}

NTSTATUS driver::ioctl::device_control(PDEVICE_OBJECT device, PIRP pirp)
{
	UNREFERENCED_PARAMETER(device);

	DebugPrint("[+] Device control called!\n");

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(pirp);

	if (stack_irp == nullptr)
	{
		DebugPrint("[-] StackIrp == nullptr!\n");
		
		IoCompleteRequest(pirp, IO_NO_INCREMENT);
		return status;
	}

	auto request = reinterpret_cast<driver::ioctl::Request*>(pirp->AssociatedIrp.SystemBuffer);

	if (request == nullptr)
	{
		DebugPrint("[-] Request == nullptr!\n");
		
		IoCompleteRequest(pirp, IO_NO_INCREMENT);
		return status;
	}

	static PEPROCESS target_process = nullptr;
	const ULONG code = stack_irp->Parameters.DeviceIoControl.IoControlCode;

	DebugPrint("[/] Code: %X\n", code);

	switch (code)
	{
	case codes::attach:
	{
		status = PsLookupProcessByProcessId(request->process_id, &target_process);
		break;
	}

	case codes::read_cb:
	{
		if (target_process == nullptr)
		{
			DebugPrint("[-] Attach to a process first!\n");
			break;
		}

		PVOID* callback_address = reinterpret_cast<PVOID*>(reinterpret_cast<char*>(target_process) + INSTRUMENTATION_CALLBACK_OFFSET);
		PVOID callback = *callback_address;

		if (request->callback_out == nullptr)
		{
			DebugPrint("[-] callback_out == nullptr!\n");

			break;
		}

		*request->callback_out = callback;
		status = STATUS_SUCCESS;
		break;
	}
	case codes::write_cb:
	{
		if (target_process == nullptr)
		{
			DebugPrint("[-] Attach to a process first!\n");
			break;
		}

		DebugPrint("[/] Process: %p\n", (void*) target_process);

		PVOID* callback_address = reinterpret_cast<PVOID*>(reinterpret_cast<char*>(target_process) + INSTRUMENTATION_CALLBACK_OFFSET);
		DebugPrint("[/] Original CallbackAddress: %p\n", *callback_address);

		*callback_address = request->callback_in;
		DebugPrint("[+] New CallbackAddress: %p\n", *callback_address);

		status = STATUS_SUCCESS;

		DebugPrint("[+] Instrumentation callback set!\n");
		break;
	}

	default:
		DebugPrint("[-] Unknown io code!\n");

		break;
	}

	IoCompleteRequest(pirp, IO_NO_INCREMENT);

	pirp->IoStatus.Status = status;
	pirp->IoStatus.Information = sizeof(Request);

	return status;
}
