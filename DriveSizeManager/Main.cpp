#include <Windows.h>
#include <stdio.h>
#include "Util.h"
#include "Injector.h"
#include "Main.h"

void OnConsoleReset()
{
	printf("***********************************************************\n");
	printf("*                    Drive Space Unlocker                 *\n");
	printf("*                      from Speedi13                      *\n");
	printf("*                                                         *\n");
	printf("*               https://github.com/Speedi13               *\n");
	printf("***********************************************************\n\n");
}

int main( int argc, char* argv[] )
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	//lets try to get administrator privileges
	if ( IsElevated() != TRUE )
	{	
		if ( TryToRestartElevated() == true )
			return 0;
	}
	
	SharedMemoryData* pSharedMemoryData = NULL;

	//This will fail when we are not running with administrator privileges
	HANDLE hMapFile = SetUpFileMapping( const_cast<char*>(SharedMemoryName), sizeof(SharedMemoryData) );
	if ( hMapFile == INVALID_HANDLE_VALUE || hMapFile == NULL )
	{
		printf("[!] Error while setting up shared memory [0x%X]!\n",GetLastError());
		Sleep( 1000 );
		
		//now lets try using a config file:
		g_IsConfigFile = true;
		pSharedMemoryData = (SharedMemoryData*)malloc( sizeof(SharedMemoryData ) );
	}
	else
	{
		pSharedMemoryData = (SharedMemoryData*)
			MapViewOfFile(	hMapFile,					// handle to map object
							FILE_MAP_ALL_ACCESS,		// read/write permission
							NULL,
							NULL,
							sizeof(SharedMemoryData)
						);
		if ( pSharedMemoryData == NULL )
		{
			printf("[!] Error while MapViewOfFile [0x%X] [0x%p]!\n",GetLastError(),pSharedMemoryData);
			pause();
			return 1;
		}
	}
	bool bUnloadDll = false;
	while (true)
	{
		ResetConsole(OnConsoleReset);
		printf("	1) Change Volume Size (unlock the drives)\n");
		printf("	2) Undo changes\n");
		printf("	3) Exit\n");
		printf("> ");
		DWORD dwSelectedOption = NULL;
		scanf_s( "%u", &dwSelectedOption );
		if ( dwSelectedOption < 1 || 3 < dwSelectedOption )
		{
			printf("[!] Error number entered is not an available option!\n");
			pause();
			continue;
		}
		if ( dwSelectedOption == 1 )
			break;
		else
		if( dwSelectedOption == 3 )
		{
			if ( g_IsConfigFile != true )
			{
				UnmapViewOfFile(pSharedMemoryData);
				CloseHandle(hMapFile);
			}
			return ERROR_SUCCESS;
		}
		else
		if ( dwSelectedOption == 2 )
		{
			bUnloadDll = true;
			break;
		}
	}
	DWORD dwMaxPathLen = 1024;
	char* DllPath = (char*)malloc( dwMaxPathLen );
	DWORD dwExePathLen = 
	GetCurrentExecutableDirectory( DllPath, dwMaxPathLen );
	if ( dwExePathLen == NULL )
	{
		printf("[!] Error while getting current executable directory!\n");
		pause();
		return 1;
	}

	char* ConfigPath = NULL;
	wchar_t* wConfigPath = NULL;
	if ( g_IsConfigFile == true )
	{
		ConfigPath = (char*)malloc( dwMaxPathLen );
		memcpy( ConfigPath, DllPath, dwExePathLen+1 );
		strcpy_s( &ConfigPath[dwExePathLen-1], dwMaxPathLen-dwExePathLen , "\\VolumeInformationHook.ini" );

		size_t dwConfigPathLen = strlen( ConfigPath );

		wConfigPath = (wchar_t*)malloc( dwConfigPathLen*2+2 );
		for (DWORD l = 0; l < dwConfigPathLen+1; l++)
			wConfigPath[l] = ConfigPath[l];
		
		if ( bUnloadDll == true ) //Delete it now
		{
			DeleteFileA( ConfigPath );
			DeleteFileW( wConfigPath );
		}

		free( ConfigPath );
		ConfigPath = NULL;
	}

	strcpy_s( &DllPath[dwExePathLen-1], dwMaxPathLen-dwExePathLen , "\\VolumeInformationHook.dll" );

	if ( DoesFileExistA( DllPath ) != true )
	{
		printf("[!] Error \"VolumeInformationHook.dll\" not found!\n");
		MessageBoxA( NULL, 
					 "ERROR: \"VolumeInformationHook.dll\" not found!", 
					 NULL /*setting this to null will result in the title being error*/, 
					 MB_ICONERROR
				  );
		pause();
		return 1;
	}

	std::vector<DWORD> explorerPids = GetProcsId("explorer.exe");
	if ( explorerPids.size() == NULL )
	{
		printf("[!] Error no explorer process found!\n");
		pause();
		return 1;
	}

	for (DWORD k = 0; k < explorerPids.size(); k++)
	{
		if ( bUnloadDll == true )
			UnloadDll( explorerPids.at(k), DllPath );
		else
			InjectDll( explorerPids.at(k), DllPath );
	}
	
	free( DllPath );

	char VolumeName[10];
	while (true)
	{
		if ( bUnloadDll == true )
			break;
		ResetConsole(OnConsoleReset);
		printf("Enter Drive leter:\n");
		scanf_s("%9s",VolumeName);
		if ( StrCmpToLower(VolumeName,"exit") )
			break;

		WCHAR wVolumePath[8];
		wVolumePath[0] = VolumeName[0];
		wVolumePath[1] = ':';
		wVolumePath[2] = '\\';
		wVolumePath[3] = NULL;

		ULONG dwVolumeSerial = GetVolueSerial( wVolumePath );
		if( dwVolumeSerial == NULL )
		{
			printf("[!] Error while accessing drive information {\"%s\"}[%u]!\n",VolumeName,dwVolumeSerial);
			pause();
			continue;
		}
		printf("Enter New Volume Size (in GB):\n");
		ULONGLONG NewVolumeSize = NULL;
		scanf_s("%llu",&NewVolumeSize);
		if ( (__int64)(NewVolumeSize) < 1 )
		{
			printf("[!] Error invalid volume size [%llu]!\n",NewVolumeSize);
			pause();
			continue;
		}
		//turn GB into byte
		NewVolumeSize *= 1024;
		NewVolumeSize *= 1024;
		NewVolumeSize *= 1024;
		if ( g_IsConfigFile != true )
		{
			DriveInfo* pDriveInfo = NULL;
			pDriveInfo = pSharedMemoryData->GetListEntry( dwVolumeSerial );
			if ( pDriveInfo == NULL )
			{
				if ( pSharedMemoryData->DrivesToModify >= 40 )
				{
					printf("[!] Error max number of drives reached [%u]!\n",pSharedMemoryData->DrivesToModify);
					pause();
					continue;
				}
				pDriveInfo = &pSharedMemoryData->drive[pSharedMemoryData->DrivesToModify];
				pSharedMemoryData->DrivesToModify++;
			}

			pDriveInfo->NewVolumeSize = NewVolumeSize;
			pDriveInfo->VolumeSerialNumber = dwVolumeSerial;
		}
		else //if ( g_IsConfigFile == true )
		{
			static char* szSerialNumber = NULL;
			if ( szSerialNumber == NULL )
				 szSerialNumber = (char*)malloc( 16 );
			sprintf_s(szSerialNumber,15,"0x%X",dwVolumeSerial);
			IniSaveSetting_Integer( wConfigPath, szSerialNumber, "NewVolumeSize", NewVolumeSize );
		}

		pause();
	}

	UnmapViewOfFile(pSharedMemoryData);
	CloseHandle(hMapFile);
	return ERROR_SUCCESS;
}