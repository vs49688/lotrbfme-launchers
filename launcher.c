#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "launcher.h"

/* Make this a symbol as we need its size.. */
static const char G4[] = LOTRBFME_G4;
//static_assert(sizeof(G4) == 36 + 1, "invalid G4 size");

static HANDLE setup_file_payload(void)
{
	SECURITY_ATTRIBUTES attributes;
	ZeroMemory(&attributes, sizeof(attributes));
	attributes.nLength = 12;
	attributes.lpSecurityDescriptor = 0;
	attributes.bInheritHandle = 1;

	HANDLE hFile = CreateFileMappingA(INVALID_HANDLE_VALUE, &attributes, PAGE_READWRITE, 0, sizeof(G4) - 1, NULL);
	if(!hFile)
		return NULL;

	void *ptr = MapViewOfFileEx(hFile, 0xF001F, 0, 0, 0, 0);
	if(!ptr)
	{
		CloseHandle(hFile);
		return NULL;
	}

	memcpy(ptr, G4, sizeof(G4) - 1);

	UnmapViewOfFile(ptr);
	return hFile;
}

static void show_error_box(LPCWSTR msg)
{
	MessageBoxW(NULL, msg, LOTRBFME_GAME_NAME, MB_OK | MB_ICONERROR);
}

static LPWSTR get_executable_directory()
{
	HMODULE hThis = GetModuleHandleW(NULL);
	HANDLE hHeap = GetProcessHeap();

	DWORD dwSize = MAX_PATH;
	LPWSTR p = HeapAlloc(hHeap, 0, dwSize * sizeof(WCHAR));
	if(!p)
		return NULL;

	while(GetModuleFileNameW(hThis, p, dwSize) == dwSize)
	{
		dwSize *= 2;
		LPWSTR np = HeapReAlloc(hHeap, 0, p, dwSize * sizeof(WCHAR));
		if(!np)
		{
			HeapFree(hHeap, 0, p);
			return NULL;
		}
		p = np;
	}

	LPWSTR end = wcsrchr(p, '\\');
	if(end)
		*end = L'\0';

	return p;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LPWSTR progDir = get_executable_directory();
	if(!progDir)
	{
		show_error_box(L"Error getting executable directory.");
		return 1;
	}

	if(!SetCurrentDirectoryW(progDir))
	{
		HeapFree(GetProcessHeap(), 0, progDir);
		show_error_box(L"Error setting current directory.");
		return 1;
	}
	HeapFree(GetProcessHeap(), 0, progDir);

	HANDLE hFile = setup_file_payload();
	if(!hFile)
	{
		show_error_box(L"Error mapping file.");
		return -1;
	}

	BOOL result = FALSE;
	DWORD exitCode = 0;
	HANDLE hMutex = NULL;
	HANDLE hEvent = NULL;

	if(!(hMutex = CreateMutexA(NULL, FALSE, LOTRBFME_G1)))
	{
		DWORD dwErr = GetLastError();
		if(dwErr == ERROR_ALREADY_EXISTS)
			show_error_box(L"Game already running.");
		else
			show_error_box(L"Error creating mutex.");

		goto create_mutex_fail;
	}

	/* Create the event to trigger game.dat */
	if(!(hEvent = CreateEventA(0, 0, 0, LOTRBFME_G3)))
	{
		show_error_box(L"Error creating event.");
		goto create_event_fail;
	}


	STARTUPINFOW si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	if(!CreateProcessW(L"game.dat", GetCommandLineW(), NULL, NULL, TRUE, 0 /*CREATE_SUSPENDED*/, NULL, NULL, &si, &pi))
	{
		show_error_box(L"Error launching game executable.");
		goto create_process_fail;
	}

	HANDLE Handles[2] = {hEvent, pi.hProcess};

	/* Wait for the event or process termination. */
	DWORD dwResult = WaitForMultipleObjects(2, Handles, 0, INFINITE);
	if(dwResult == WAIT_OBJECT_0)
	{
		/* We got the event, send the payload and wait for termination. */
		PostThreadMessageA(pi.dwThreadId, 0xBEEF, 0, (LPARAM)hFile);
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

	result = GetExitCodeProcess(pi.hProcess, &exitCode);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

create_process_fail:
	CloseHandle(hEvent);
create_event_fail:
	CloseHandle(hMutex);
create_mutex_fail:
	CloseHandle(hFile);
	return result ? (int)exitCode : -1;
}
