#pragma once
#include <ntifs.h>

namespace driver::ioctl
{
	namespace codes
	{
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x6900, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		constexpr ULONG read_cb =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x6901, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		constexpr ULONG write_cb =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x6902, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	struct Request
	{
		HANDLE process_id;

		PVOID callback_in;
		PVOID* callback_out;
	};

	NTSTATUS create(PDEVICE_OBJECT device, PIRP pirp);
	NTSTATUS close(PDEVICE_OBJECT device, PIRP pirp);
	NTSTATUS device_control(PDEVICE_OBJECT device, PIRP pirp);
}