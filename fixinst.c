#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "launcher.h"

static LPWSTR get_executable_directory()
{
	HMODULE hThis = GetModuleHandleW(NULL);

	DWORD dwSize = MAX_PATH;
	LPWSTR p = malloc(dwSize * sizeof(WCHAR));
	if(!p)
		return NULL;

	while(GetModuleFileNameW(hThis, p, dwSize) == dwSize)
	{
		dwSize *= 2;
		LPWSTR np = realloc(p, dwSize * sizeof(WCHAR));
		if(!np)
		{
			free(p);
			return NULL;
		}
		p = np;
	}

	LPWSTR end = wcsrchr(p, '\\');
	if(end)
		*end = L'\0';

	return p;
}

static void usage(const char *argv0)
{
	printf("Usage: %s [lotrbfme|lotrbfme2|lotrbfme2ep1]\n", argv0);
}

enum kGame {
	GAME_NONE = 0,
	GAME_LOTRBFME,
	GAME_LOTRBFME2,
	GAME_LOTRBFME2EP1
};

typedef struct gameinfo
{
	enum kGame game;
	const char *sku;

	struct
	{
		wchar_t *game_dir;
		wchar_t *launcher_exe;
		wchar_t *userdata;
		wchar_t *options_ini;
	} paths;

	struct
	{
		wchar_t *app_path;
		wchar_t *game;
	} registry;
} gameinfo_t;

static wchar_t *joinpath(const wchar_t *p1, const wchar_t *p2, size_t *len)
{
	size_t _len = wcslen(p1) + 1 + wcslen(p2);
	wchar_t *s = malloc((_len + 1) * sizeof(wchar_t));
	if(!s)
		return NULL;

	swprintf(s, _len, L"%ls\\%ls", p1, p2);
	if(len)
		*len = _len;
	return s;
}

/*
 * Search for:
 * lotrbfme2ep1.exe - RotWK
 * lotrbfme2.exe    - BfME2
 * lotrbfme.exe     - BfME
 */
static const char *autodetect_game(const wchar_t *gamedir)
{
	wchar_t *launcher_path = joinpath(gamedir, L"lotrbfme2ep1.exe", NULL);
	if(!launcher_path)
		return NULL;

	size_t baselen = wcslen(gamedir) + 1; /* including '\\' */

	const char *sku = NULL;

	if(PathFileExistsW(launcher_path))
	{
		sku = "lotrbfme2ep1";
		goto done;
	}

	wcscpy(launcher_path + baselen, L"lotrbfme2.exe");
	if(PathFileExistsW(launcher_path))
	{
		sku = "lotrbfme2";
		goto done;
	}

	wcscpy(launcher_path + baselen, L"lotrbfme.exe");
	if(PathFileExistsW(launcher_path))
	{
		sku = "lotrbfme";
		goto done;
	}

done:
	free(launcher_path);
	return sku;
}

void gameinfo_release(gameinfo_t *gi)
{
	if(!gi)
		return;

	if(gi->paths.game_dir)
		free(gi->paths.game_dir);

	if(gi->paths.launcher_exe)
		free(gi->paths.launcher_exe);

	if(gi->paths.userdata)
		free(gi->paths.userdata);

	if(gi->paths.options_ini)
		free(gi->paths.options_ini);

	if(gi->registry.app_path)
		free(gi->registry.app_path);

	if(gi->registry.game)
		free(gi->registry.game);

}

gameinfo_t *gameinfo_build(enum kGame game, gameinfo_t *gi, const wchar_t *game_dir)
{
	if(game == GAME_NONE)
		return NULL;


	const char *skuname = NULL;
	const wchar_t *launcher_exe = NULL;
	const wchar_t *userdataleaf = NULL;
	const wchar_t *gameregpath = NULL;

	if(game == GAME_LOTRBFME)
	{
		skuname = "lotrbfme";
		launcher_exe = L"lotrbfme.exe";
		userdataleaf = LOTRBFME1_USERDATALEAF;
		gameregpath = LOTRBFME1_GAMEREGPATH;
	}
	else if(game == GAME_LOTRBFME2)
	{
		skuname = "lotrbfme2";
		launcher_exe = L"lotrbfme2.exe";
		userdataleaf = LOTRBFME2_USERDATALEAF;
		gameregpath = LOTRBFME2_GAMEREGPATH;
	}
	else if(game == GAME_LOTRBFME2EP1)
	{
		skuname = "lotrbfme2eep1";
		launcher_exe = L"lotrbfme2ep1.exe";
		userdataleaf = LOTRBFME2EP1_USERDATALEAF;
		gameregpath = LOTRBFME2EP1_GAMEREGPATH;
	}
	else
	{
		return NULL;
	}

	memset(gi, 0, sizeof(gameinfo_t));

	gi->game = game;
	gi->sku = skuname;

	if(!(gi->paths.game_dir = wcsdup(game_dir)))
		goto fail;

	if(!(gi->paths.launcher_exe = joinpath(gi->paths.game_dir, launcher_exe, NULL)))
		goto fail;

	wchar_t buf[MAX_PATH];
	if(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf) != S_OK)
		goto fail;

	if(!(gi->paths.userdata = joinpath(buf, userdataleaf, NULL)))
		goto fail;

	if(!(gi->paths.options_ini = joinpath(gi->paths.userdata, L"Options.ini", NULL)))
		goto fail;

	if(!(gi->registry.app_path = joinpath(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths", launcher_exe, NULL)))
		goto fail;

	if(!(gi->registry.game = wcsdup(gameregpath)))
		goto fail;
	return gi;

fail:
	gameinfo_release(gi);
	return NULL;
}



/* Attempt to fix the game's registry keys. */
static BOOL fix_registry(gameinfo_t *gi)
{
	HKEY hKey = NULL;
	BOOL r = TRUE;

	/* Create the InstallPath key so patchers and installers can find it. */
	if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, gi->registry.game, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return FALSE;

	if(RegSetValueExW(hKey, L"InstallPath", 0, REG_SZ, (const BYTE*)gi->paths.game_dir, wcslen(gi->paths.game_dir) * sizeof(WCHAR)) != ERROR_SUCCESS)
	{
		r = FALSE;
		goto fail;
	}

	RegCloseKey(hKey);

	if(RegCreateKeyExW(HKEY_LOCAL_MACHINE, gi->registry.app_path, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
		return FALSE;

	if(RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)gi->paths.launcher_exe, wcslen(gi->paths.launcher_exe) * sizeof(WCHAR)) != ERROR_SUCCESS)
	{
		r = FALSE;
		goto fail;
	}

	if(RegSetValueExW(hKey, L"Path", 0, REG_SZ, (const BYTE*)gi->paths.game_dir, wcslen(gi->paths.game_dir) * sizeof(WCHAR)) != ERROR_SUCCESS)
	{
		r = FALSE;
		goto fail;
	}

fail:
	RegCloseKey(hKey);
	return r;
}

static const char options_ini_content[] =
	"AllHealthBars = yes\n"
	"MusicVolume = 60.000000\n"
	"AmbientVolume = 70.000000\n"
	"SFXVolume = 100.000000\n"
	"VoiceVolume = 80.000000\n"
	"MovieVolume = 80.000000\n"
	"AudioLOD = High\n"
	"UseEAX3 = no\n"
	"Brightness = 50\n"
	"HasGotOnline = yes\n"
	"IdealStaticGameLOD = Low\n"
	"StaticGameLOD = Low\n"
	"Resolution = 800 600\n"
	"ScrollFactor = 50\n"
	"FlashTutorial = 0\n"
	"HasSeenLogoMovies = yes\n"
	"TimesInGame = 1";

static BOOL fix_config(gameinfo_t *gi)
{
	/* Create the directory, ignoring if it already exists. */
	if(!CreateDirectoryW(gi->paths.userdata, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
		return FALSE;

	HANDLE hFile = NULL;

	/* Create Options.ini if it doesn't exist. */
	if(!(hFile = CreateFileW(gi->paths.options_ini, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)))
		return FALSE;

	BOOL r = FALSE;

	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		r = TRUE;
		goto fail;
	}

	if(SetFilePointer(hFile, 0, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		goto fail;

	if(!SetEndOfFile(hFile))
		goto fail;

	DWORD dwNum = 0;
	if(!WriteFile(hFile, options_ini_content, sizeof(options_ini_content), &dwNum, NULL))
		goto fail;

	r = TRUE;
fail:
	CloseHandle(hFile);
	return r;
}

int main(int argc, char **argv)
{
	if(argc > 2)
	{
		usage(argv[0]);
		return 2;
	}

	LPWSTR progDir = get_executable_directory();
	if(!progDir)
	{
		fprintf(stderr, "Error getting executable directory.\n");
		return 1;
	}

	const char *ssku = NULL;
	if(argc == 2)
		ssku = argv[1];

	if(!ssku)
		ssku = autodetect_game(progDir);

	if(!ssku)
	{
		fprintf(stderr, "Unable to determine SKU, exiting...\n");
		free(progDir);
		return 1;
	}

	enum kGame game = GAME_NONE;
	if(!strcmp("lotrbfme", ssku))
		game = GAME_LOTRBFME;
	else if(!strcmp("lotrbfme2", ssku))
		game = GAME_LOTRBFME2;
	else if(!strcmp("lotrbfme2ep1", ssku))
		game = GAME_LOTRBFME2EP1;

	if(game == GAME_NONE)
	{
		fprintf(stderr, "Invalid SKU, exiting...\n");
		free(progDir);
		return 1;
	}

	printf("Using SKU %s...\n", ssku);

	gameinfo_t gi;
	if(gameinfo_build(game, &gi, progDir) != &gi)
	{
		fprintf(stderr, "Error building game strings, exiting...\n");
		free(progDir);
		return 1;
	}
	free(progDir);

	printf("  paths.game_dir     = %ls\n", gi.paths.game_dir);
	printf("  paths.launcher_exe = %ls\n", gi.paths.launcher_exe);
	printf("  paths.userdata     = %ls\n", gi.paths.userdata);
	printf("  paths.options_ini  = %ls\n", gi.paths.options_ini);
	printf("  registry.app_path  = %ls\n", gi.registry.app_path);
	printf("  registry.game      = %ls\n", gi.registry.game);

	if(!fix_registry(&gi))
		fprintf(stderr, "Error fixing registry...\n");

	if(!fix_config(&gi))
		fprintf(stderr, "Error fixing directories...\n");

	gameinfo_release(&gi);
	return 0;
}