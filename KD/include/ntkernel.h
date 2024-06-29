#pragma once
#include <ntifs.h>
#include <stdarg.h>

inline void DebugPrint(const char* format, ...)
{
#ifndef DEBUG
	UNREFERENCED_PARAMETER(format);
#endif

#ifdef DEBUG
	va_list list;
	va_start(list, format);
	
	vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, format, list);
	
	va_end(list);
#endif
}

extern "C" {
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializiationFunction);
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress,
		PEPROCESS TargetProcess, PVOID TargetAddress,
		SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
}