#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct handle_deleter
{
	void operator()(HANDLE h) { CloseHandle(h); }
};
using handle_ptr = std::unique_ptr<std::remove_pointer<HANDLE>::type, handle_deleter>;

static std::wstring get_executable_directory(void)
{
	wchar_t *pgmPtr;
	_get_wpgmptr(&pgmPtr);

	std::wstring w(pgmPtr);
	return w.substr(0, w.find_last_of(L"/\\"));
}

static handle_ptr setup_file_payload(void)
{
	constexpr const char payload[] = "D9151691-DF43-448c-87C2-742C1FC0FAEB";
	static_assert(sizeof(payload) == 36 + 1, "sizeof(payload) != 36 + 1");

	handle_ptr hFile;
	{
		SECURITY_ATTRIBUTES attributes;
		ZeroMemory(&attributes, sizeof(attributes));
		attributes.nLength = 12;
		attributes.lpSecurityDescriptor = 0;
		attributes.bInheritHandle = 1;

		hFile.reset(CreateFileMappingA(INVALID_HANDLE_VALUE, &attributes, PAGE_READWRITE, 0, sizeof(payload) - 1, nullptr));
	}

	if(!hFile)
		return nullptr;

	void *ptr = MapViewOfFileEx(hFile.get(), 0xF001F, 0, 0, 0, 0);
	if(!ptr)
		return nullptr;

	memcpy(ptr, payload, sizeof(payload) - 1);

	UnmapViewOfFile(ptr);
	return hFile;
}

void show_error_box(LPCWSTR msg)
{
	static const wchar_t s_message_box_title[] = L"The Battle for Middle-earth II";
	MessageBoxW(nullptr, msg, s_message_box_title, MB_OK | MB_ICONERROR);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	std::wstring progDir = get_executable_directory();

	if(!SetCurrentDirectoryW(progDir.c_str()))
	{
		show_error_box(L"Error setting current directory.");
		return -1;
	}

	handle_ptr hFile = setup_file_payload();
	if(hFile == nullptr)
	{
		show_error_box(L"Error mapping file.");
		return -1;
	}

	handle_ptr hMutex(CreateMutexA(nullptr, FALSE, "4CE5E3EE-B113-4417-B651-6575C092F128"));
	if(hMutex == nullptr)
	{
		DWORD dwErr = GetLastError();
		if(dwErr == ERROR_ALREADY_EXISTS)
			show_error_box(L"Game already running.");
		else
			show_error_box(L"Error creating mutex.");
		return -1;
	}

	/* Create the event to trigger game.dat */
	handle_ptr hEvent(CreateEventA(0, 0, 0, "D0BE288D-395A-4a73-A50E-A796A9E1D804"));
	if(hEvent == nullptr)
	{
		show_error_box(L"Error creating event.");
		return -1;
	}


	STARTUPINFOW si;
	{
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.dwFlags |= STARTF_USESTDHANDLES;
	}

	PROCESS_INFORMATION pi;
	{
		ZeroMemory(&pi, sizeof(pi));
	}

	if(!CreateProcessW(L"game.dat", lpCmdLine, nullptr, nullptr, TRUE, 0 /*CREATE_SUSPENDED*/, nullptr, nullptr, &si, &pi))
	{
		show_error_box(L"Error launching game executable.");
		return -1;
	}

	HANDLE Handles[2] = {hEvent.get(), pi.hProcess};

	/* Wait for the event or process termination. */
	DWORD dwResult = WaitForMultipleObjects(2, Handles, 0, INFINITE);
	if(dwResult == WAIT_OBJECT_0)
	{
		/* We got the event, send the payload and wait for termination. */
		PostThreadMessageA(pi.dwThreadId, 0xBEEF, 0, reinterpret_cast<LPARAM>(hFile.get()));
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

	DWORD exitCode;
	BOOL result = GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	return result ? exitCode : -1;
}

