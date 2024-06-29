#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>

std::string GetLastErrorAsString()
{
	//Get the error message ID, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0) {
		return std::string(); //No error message has been recorded
	}

	LPSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	//Copy the error message into a std::string.
	std::string message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}


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

	static bool attach_to_process(HANDLE driver, const DWORD pid)
	{
		Request r;
		r.process_id = reinterpret_cast<HANDLE>(pid);

		return DeviceIoControl(
			driver, codes::attach, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr
		);
	}

	static void* read_callback(HANDLE driver)
	{
		Request r;
		PVOID cb = nullptr;

		r.callback_out = &cb;

		if (!DeviceIoControl(driver, codes::read_cb, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr))
		{
			return nullptr;
		}

		return cb;
	}

	static bool write_callback(HANDLE driver, PVOID callback)
	{
		Request r;
		
		r.callback_in = callback;

		return DeviceIoControl(
			driver, codes::write_cb, &r, sizeof(r), &r, sizeof(r), nullptr, nullptr
		);
	}
}

static DWORD getProcId(const wchar_t* process_name)
{
	DWORD procId = 0;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hSnap == INVALID_HANDLE_VALUE)
		return procId;

	PROCESSENTRY32W entry = {};
	entry.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnap, &entry))
	{
		if (_wcsicmp(process_name, entry.szExeFile) == 0)
		{
			procId = entry.th32ProcessID;
		}
		else
		{
			while (Process32NextW(hSnap, &entry))
			{
				if (_wcsicmp(process_name, entry.szExeFile) == 0)
				{
					procId = entry.th32ProcessID;
					
					break;
				}
			}
		}
	}

	CloseHandle(hSnap);
	return procId;
}

int main()
{
	const DWORD pid = getProcId(L"instrumentation_cb.exe");

	if (!pid)
	{
		std::cerr << "Failed to get pid of: instrumentation_cb.exe" << std::endl;

		return 1;
	}

	std::cout << "pid: " << pid << std::endl;

	const HANDLE driver = CreateFileA("\\\\.\\Instrumentation", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (driver == INVALID_HANDLE_VALUE)
	{
		std::cerr << "Failed to get driver handle!" << std::endl;
		std::cin.get();

		return 1;
	}

	if (!driver::ioctl::attach_to_process(driver, pid))
	{
		std::cerr << "Failed to attach to process!" << std::endl;
		std::cerr << GetLastErrorAsString() << std::endl;
	
		CloseHandle(driver);
		return 1;
	}

	void* callback = driver::ioctl::read_callback(driver);

	std::cout << "read callback: " << callback << std::endl;

	void* new_callback = nullptr;
	std::cin >> new_callback;

	if (!driver::ioctl::write_callback(driver, new_callback))
	{
		std::cerr << "Failed to write new callback!" << std::endl;
		std::cerr << GetLastErrorAsString() << std::endl;
	
		CloseHandle(driver);
		return 1;
	}

	std::cout << "Success!" << std::endl;
}