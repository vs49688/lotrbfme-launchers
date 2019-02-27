#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "launcher.h"

/* Make them symbols as we need the size. */
constexpr const wchar_t GameName[] = LOTRBFME_GAME_NAME;
constexpr const char G1[] = LOTRBFME_G1;
constexpr const char G2[] = LOTRBFME_G2;
constexpr const char G3[] = LOTRBFME_G3;
constexpr const char G4[] = LOTRBFME_G4;

static_assert(sizeof(G1) == 36 + 1, "invalid G1 size");
static_assert(sizeof(G2) == 36 + 1, "invalid G2 size");
static_assert(sizeof(G3) == 36 + 1, "invalid G3 size");
static_assert(sizeof(G4) == 36 + 1, "invalid G4 size");

struct handle_deleter
{
	void operator()(HANDLE h) { CloseHandle(h); }
};
using handle_ptr = std::unique_ptr<std::remove_pointer<HANDLE>::type, handle_deleter>;

static handle_ptr setup_file_payload()
{
	handle_ptr hFile;
	{
		SECURITY_ATTRIBUTES attributes;
		ZeroMemory(&attributes, sizeof(attributes));
		attributes.nLength = 12;
		attributes.lpSecurityDescriptor = 0;
		attributes.bInheritHandle = 1;

		hFile.reset(CreateFileMappingA(INVALID_HANDLE_VALUE, &attributes, PAGE_READWRITE, 0, sizeof(G4) - 1, nullptr));
	}

	if(!hFile)
		return nullptr;

	void *ptr = MapViewOfFileEx(hFile.get(), 0xF001F, 0, 0, 0, 0);
	if(!ptr)
		return nullptr;

	memcpy(ptr, G4, sizeof(G4) - 1);

	UnmapViewOfFile(ptr);
	return hFile;
}

void show_error_box(LPCWSTR msg)
{
	MessageBoxW(nullptr, msg, GameName, MB_OK | MB_ICONERROR);
}

static LPWSTR get_executable_directory()
{
	HMODULE hThis = GetModuleHandleW(nullptr);
	HANDLE hHeap = GetProcessHeap();

	DWORD dwSize = MAX_PATH;
	LPWSTR p = reinterpret_cast<LPWSTR>(HeapAlloc(hHeap, 0, dwSize * sizeof(WCHAR)));
	if(!p)
		return nullptr;

	DWORD dwRet = 0;
	while((dwRet = GetModuleFileNameW(hThis, p, dwSize)) == dwSize)
	{
		dwSize *= 2;
		if(!(p = reinterpret_cast<LPWSTR>(HeapReAlloc(hHeap, 0, p, dwSize * sizeof(WCHAR)))))
			return nullptr;
	}

	LPWSTR end = wcsrchr(p, '\\');
	if(end)
		*end = L'\0';

	return p;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPWSTR progDir = get_executable_directory();
	if(progDir == nullptr)
	{
		show_error_box(L"Error getting executable directory.");
		return -1;
	}

	if(!SetCurrentDirectoryW(progDir))
	{
		HeapFree(GetProcessHeap(), 0, progDir);
		show_error_box(L"Error setting current directory.");
		return -1;
	}
	HeapFree(GetProcessHeap(), 0, progDir);

	handle_ptr hFile = setup_file_payload();
	if(hFile == nullptr)
	{
		show_error_box(L"Error mapping file.");
		return -1;
	}

	handle_ptr hMutex(CreateMutexA(nullptr, FALSE, G1));
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
	handle_ptr hEvent(CreateEventA(0, 0, 0, G3));
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

	if(!CreateProcessW(L"game.dat", GetCommandLineW(), nullptr, nullptr, TRUE, 0 /*CREATE_SUSPENDED*/, nullptr, nullptr, &si, &pi))
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
