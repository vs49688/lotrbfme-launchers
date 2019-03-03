#ifndef _LAUNCHER_H
#define _LAUNCHER_H

#define LOTRBFME1_SKUNAME			L"The Battle for Middle-earth"
#define LOTRBFME1_G1				"46EB79F1-5924-4375-AE1E-1C3C36C7AC4D"
#define LOTRBFME1_G2				"4B4833CF-86AE-4bef-BB28-08170D3581DB"
#define LOTRBFME1_G3				"CA5F8EE2-0630-4ef3-BD73-D71F832CD25F"
#define LOTRBFME1_G4				"B966C0E5-16AC-4ebd-90AC-D7A8C8976040"
#define LOTRBFME1_GAMEREGPATH		L"SOFTWARE\\Electronic Arts\\EA Games\\The Battle for Middle-earth"
#define LOTRBFME1_USERDATALEAF		L"My Battle for Middle-earth Files"

#define LOTRBFME2_SKUNAME			L"The Battle for Middle-earth II"
#define LOTRBFME2_G1				"4CE5E3EE-B113-4417-B651-6575C092F128"
#define LOTRBFME2_G2				"37915039-6803-49e7-B69E-64FD313B7E8B"
#define LOTRBFME2_G3				"D0BE288D-395A-4a73-A50E-A796A9E1D804"
#define LOTRBFME2_G4				"D9151691-DF43-448c-87C2-742C1FC0FAEB"
#define LOTRBFME2_GAMEREGPATH		L"SOFTWARE\\Electronic Arts\\Electronic Arts\\The Battle for Middle-earth II"
#define LOTRBFME2_USERDATALEAF		L"My Battle for Middle-earth(tm) II Files"

#define LOTRBFME2EP1_SKUNAME		L"The Lord of the Rings, The Rise of the Witch-king"
#define LOTRBFME2EP1_G1				LOTRBFME2_G1
#define LOTRBFME2EP1_G2				LOTRBFME2_G2
#define LOTRBFME2EP1_G3				LOTRBFME2_G3
#define LOTRBFME2EP1_G4				LOTRBFME2_G4
#define LOTRBFME2EP1_GAMEREGPATH	L"SOFTWARE\\Electronic Arts\\Electronic Arts\\The Lord of the Rings, The Rise of the Witch-king"
#define LOTRBFME2EP1_USERDATALEAF	L"My The Lord of the Rings, The Rise of the Witch-king Files"

#if LOTRBFME_GAME == 1
#	define LOTRBFME_SKUNAME			LOTRBFME1_SKUNAME
#	define LOTRBFME_G1				LOTRBFME1_G1
#	define LOTRBFME_G2				LOTRBFME1_G2
#	define LOTRBFME_G3				LOTRBFME1_G3
#	define LOTRBFME_G4				LOTRBFME1_G4
#	define LOTRBFME_GAMEREGPATH		LOTRBFME1_GAMEREGPATH
#	define LOTRBFME_USERDATALEAF	LOTRBFME1_USERDATALEAF
#elif LOTRBFME_GAME == 2
#	define LOTRBFME_SKUNAME			LOTRBFME2_SKUNAME
#	define LOTRBFME_G1				LOTRBFME2_G1
#	define LOTRBFME_G2				LOTRBFME2_G2
#	define LOTRBFME_G3				LOTRBFME2_G3
#	define LOTRBFME_G4				LOTRBFME2_G4
#	define LOTRBFME_GAMEREGPATH		LOTRBFME2_GAMEREGPATH
#	define LOTRBFME_USERDATALEAF	LOTRBFME2_USERDATALEAF
#elif LOTRBFME_GAME == 3
#	define LOTRBFME_SKUNAME			LOTRBFME2EP1_SKUNAME
#	define LOTRBFME_G1				LOTRBFME2EP1_G1
#	define LOTRBFME_G2				LOTRBFME2EP1_G2
#	define LOTRBFME_G3				LOTRBFME2EP1_G3
#	define LOTRBFME_G4				LOTRBFME2EP1_G4
#	define LOTRBFME_GAMEREGPATH		LOTRBFME2EP1_GAMEREGPATH
#	define LOTRBFME_USERDATALEAF	LOTRBFME2EP1_USERDATALEAF
#endif



#endif /* _LAUNCHER_H */