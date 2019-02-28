# Battle for Middle Earth Launcher Notes

## The Battle for Middle-earth
When the launcher starts, it creates three objects:
- `46EB79F1-5924-4375-AE1E-1C3C36C7AC4D`, using `CreateMutexA()`.
  - This is required to exist by the game executable. Without it, it exits immediately.
- `4B4833CF-86AE-4bef-BB28-08170D3581DB`, using `CreateMutexW()`.
  - This appears to not be used. It is omitted in this launcher.
- `CA5F8EE2-0630-4ef3-BD73-D71F832CD25F`, using `CreateEventA()`.
  - Used to signal the game executable that the payload is ready.

Next, the game executable is launched, which waits for the event above to be triggered.

The registry is checked for the game's "electronic registration code" (ergc), located in:
- `HKLM\SOFTWARE\Electronic Arts\EA Games\The Battle for Middle-earth\ergc`, or
- `HKCU\SOFTWARE\Electronic Arts\EA Games\The Battle for Middle-earth\ergc`
If one doesn't exist, it checks the other. If none exists, it uses an empty string.

The registry is checked for the game's install directory, located in:
- `HKLM\SOFTWARE\Electronic Arts\EA Games\The Battle for Middle-earth`, `InstallPath`, or
- `HKCU\SOFTWARE\Electronic Arts\EA Games\The Battle for Middle-earth`, `InstallPath`
This is used to obtain the volume serial number of the drive it's installed on.

The registry is checked for the system's product id, located in:
- `HKLM\Software\Microsoft\Windows\CurrentVersion`, `ProductID`
This key doesn't appear to exist on later versons of windows, so it just uses an empty string.

These three properties are combined into what I call an "infostring", which is of the format:
`%lx-%s-%s`, `serial_number`, `ergc`, `product_id`.

Using the infostring, it makes a copy of the game keys and does some processing on them.

~~See the **FOR REFERENCE** section in `main.cpp`, and `payload.cpp` for more detail.~~

**EDIT:** I'm not game to give them out here publicly.

These modified keys are then used to decrypt the contents of `LOTRBFMe.dat`, a 36-byte file in
the game directory that is created by the installer. If decryption is successful, the result should
be `B966C0E5-16AC-4ebd-90AC-D7A8C8976040`. The launcher doesn't check if this is correct, which is
the main part of the "copy-protection".

The launcher then creates a 36-byte memory-mapped file and writes the decrypted payload into it.
A message is posted to the game executable's main thread, with the message id as `0xBEEF`, wParam as 0,
and lParam the handle to this memory-mapped file.
The event is then signalled, and the game starts up.

The launcher waits for the game to exit.

If the payload is incorrect, any match will automatically end after three minutes as punishment
for not buying the game.

## The Battle for Middle-earth II
The second game uses an identical method, except it loads the mutex names, event names, and
payloads from a file called `gi.dat` in the game directory.

The equivalent of `LOTRBFMe.dat` is `game2.dat`.

`gi.dat` is a blob of NULL-terminated strings with a header. Each pair of strings is a key/value pair.
```c
struct gi_dat
{
    uint8 magic[4] = {0x47, 0x49, 0x20, 0x20} // "GI  "
    uint32_le count;	// The number of key-value pairs in the file.
    struct
    {
        asciiz key;
        asciiz value;
    } content[count];
};
```

Known keys are:

| Key | Value | Description |
| --- | ----- | ---- |
| `SkuName` | `lotrbfme2` |
| `GameName` | `The Battle for Middle-earth II` |
| `GameRegPath` | `SOFTWARE\Electronic Arts\Electronic Arts\The Battle for Middle-earth II` |
| `InstallerRegPath` | `SOFTWARE\Electronic Arts\The Battle for Middle-earth II` |
| `OnlineServer` | `http://servserv.generals.ea.com/servserv/lotrbfme2` |
| `UserDataLeafName` | `My Battle for Middle-earth(tm) II Files` |
| `G1` | `4CE5E3EE-B113-4417-B651-6575C092F128` | `CreateMutexA()` mutex name. |
| `G2` | `37915039-6803-49e7-B69E-64FD313B7E8B` | `CreateMutexW()` mutex name. |
| `G3` | `D0BE288D-395A-4a73-A50E-A796A9E1D804` | The event name. |
| `G4` | `D9151691-DF43-448c-87C2-742C1FC0FAEB` | The expected payload.


## The Battle for Middle-earth II: Rise of the Witch King

Exactly the same as the base game except with slightly different `gi.dat` values.
The `G1-G4` values are the same.

| Key | Value | Description |
| --- | ----- | ---- |
| `SkuName` | `lotrbfme2ep1` |
| `GameName` | `The Lord of the Rings, The Rise of the Witch-king` |
| `GameRegPath` | `SOFTWARE\Electronic Arts\Electronic Arts\The Lord of the Rings, The Rise of the Witch-king` |
| `InstallerRegPath` | `SOFTWARE\Electronic Arts\The Lord of the Rings, The Rise of the Witch-king` |
| `OnlineServer` | `http://servserv.generals.ea.com/servserv/lotrbfme2ep1` |
| `UserDataLeafName` | `My The Lord of the Rings, The Rise of the Witch-king Files` |
| `G1` | `4CE5E3EE-B113-4417-B651-6575C092F128` | `CreateMutexA()` mutex name. |
| `G2` | `37915039-6803-49e7-B69E-64FD313B7E8B` | `CreateMutexW()` mutex name. |
| `G3` | `D0BE288D-395A-4a73-A50E-A796A9E1D804` | The event name. |
| `G4` | `D9151691-DF43-448c-87C2-742C1FC0FAEB` | The expected payload.


## Other notes

### [RotWK] Manual Installation
Just extract `0compressed.zip` and `0en-uk_compressed.zip` to the same folder.
The file names have embedded '\\' in them, but the standard `unzip` on Linux utility seems to handle this fine.

Also copy `gi.dat` to the same folder.

### [RotWK] Setup
RotWK needs BFME2 installed to run. It uses `HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths\lotrbfme2.exe`
to determine the game path and looks for the following files:
* eauninstall.exe
* game.dat
* lotrbfme2.exe
* lotrbfme2.lcf

If any don't exist, it refuses to launch. Their contents isn't checked, only that they exist.
If all files are present, the game loads uses BFME2's `filelist.txt` to load shared assets
before starting up.

```
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\App Paths\lotrbfme2.exe]
@="Z:\\Games\\The Battle for Middle-earth (tm) II\\lotrbfme2.exe"
"Path"="Z:\\Games\\The Battle for Middle-earth (tm) II"
```

### [BFME2] [RotWK] Launcher hangs and crashes

This isn't actually the launcher, it's the game.
Create and place the following in the following:
* `%APPDATA%\My Battle for Middle-earth(tm) II Files\Options.ini`
* `%APPDATA%\My The Lord of the Rings, The Rise of the Witch-king Files\Options.ini`


```ini
AllHealthBars = yes
MusicVolume = 60.000000
AmbientVolume = 70.000000
SFXVolume = 100.000000
VoiceVolume = 80.000000
MovieVolume = 80.000000
AudioLOD = High
UseEAX3 = no
Brightness = 50
HasGotOnline = yes
IdealStaticGameLOD = UltraHigh
StaticGameLOD = UltraHigh
Resolution = 1920 1080
ScrollFactor = 50
FlashTutorial = 0
HasSeenLogoMovies = yes
TimesInGame = 1
```
