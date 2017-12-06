#include <Windows.h>
#include <ntstatus.h>
#include "HookingFunctions.h"
#include "WindowsNtStructs.h"
#include "Util.h"
#include "..\DriveSizeManager\Main.h"

BYTE* pNtQueryVolumeInformationFile = NULL;
HANDLE g_hSharedMemoryHandle = NULL;

SharedMemoryData* g_pSharedMemoryData = NULL;

t_NtQueryVolumeInformationFile NtQueryVolumeInformationFile = NULL;

bool g_bUseConfigFile = false;
wchar_t* g_wConfigPath = NULL;

NTSTATUS NTAPI hkNtQueryVolumeInformationFile(HANDLE FileHandle,
	PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FsInformation,
    ULONG Length,
    FS_INFORMATION_CLASS FsInformationClass)
{
	NTSTATUS status = NtQueryVolumeInformationFile( FileHandle, IoStatusBlock, FsInformation, Length, FsInformationClass );
	if ( FsInformationClass != FileFsFullSizeInformation )
		return status;

	if ( g_bUseConfigFile == false && 
		(g_pSharedMemoryData == NULL || g_hSharedMemoryHandle == NULL || g_hSharedMemoryHandle == INVALID_HANDLE_VALUE ) )
		return status;

	_FILE_FS_FULL_SIZE_INFORMATION* pFileFsSizeInfo = (_FILE_FS_FULL_SIZE_INFORMATION*)FsInformation;

	FILE_FS_VOLUME_INFORMATION FsVolumeInformation;
	ZeroMemory(&FsVolumeInformation, sizeof(FILE_FS_VOLUME_INFORMATION) );

	NTSTATUS  FsVolumeInformationStatus = NtQueryVolumeInformationFile(FileHandle, IoStatusBlock, &FsVolumeInformation, sizeof(FILE_FS_VOLUME_INFORMATION), FileFsVolumeInformation);
	if ( FsVolumeInformationStatus != STATUS_SUCCESS )
		return status;

	DriveInfo* pDriveInfo = NULL;

	if ( g_bUseConfigFile == true )
	{
		static DriveInfo drive;
		drive.VolumeSerialNumber = FsVolumeInformation.VolumeSerialNumber;
		char szSerialNumber[20];
		wsprintfA( szSerialNumber, "0x%X", FsVolumeInformation.VolumeSerialNumber );
		drive.NewVolumeSize = IniLoadSetting_Integer( g_wConfigPath, szSerialNumber, "NewVolumeSize", &drive.NewVolumeSize );
		if (drive.NewVolumeSize != NULL)
			pDriveInfo = &drive;
	}
	if ( g_pSharedMemoryData != NULL )
	{
		for (DWORD i = 0; i < g_pSharedMemoryData->DrivesToModify; i++)
		{
			if ( g_pSharedMemoryData->drive[i].VolumeSerialNumber == FsVolumeInformation.VolumeSerialNumber )
			{
				pDriveInfo = &g_pSharedMemoryData->drive[i];
				break;
			}
		}
	}
	if ( pDriveInfo != NULL )
	{
		//convert AllocationUnits count into bytes
			ULONGLONG FreeSpace = pFileFsSizeInfo->ActualAvailableAllocationUnits.QuadPart * 
								  pFileFsSizeInfo->SectorsPerAllocationUnit *
								  pFileFsSizeInfo->BytesPerSector;

			ULONGLONG MaxSpace =  pFileFsSizeInfo->TotalAllocationUnits.QuadPart * 
								  pFileFsSizeInfo->SectorsPerAllocationUnit *
								  pFileFsSizeInfo->BytesPerSector;

			ULONGLONG NewVolumeSize = pDriveInfo->NewVolumeSize;
			ULONGLONG UsedSpace = MaxSpace - FreeSpace;			
			ULONGLONG NewFreeSpace = NewVolumeSize - UsedSpace;

			//convert bytes back into bytes AllocationUnits
			ULONGLONG NewTotalAllocationUnits =		(NewVolumeSize / pFileFsSizeInfo->BytesPerSector) / pFileFsSizeInfo->SectorsPerAllocationUnit;
			ULONGLONG NewAvailableAllocationUnits = (NewFreeSpace  / pFileFsSizeInfo->BytesPerSector) / pFileFsSizeInfo->SectorsPerAllocationUnit;

			pFileFsSizeInfo->TotalAllocationUnits.QuadPart = NewTotalAllocationUnits;
			pFileFsSizeInfo->ActualAvailableAllocationUnits.QuadPart = NewAvailableAllocationUnits;
			pFileFsSizeInfo->CallerAvailableAllocationUnits.QuadPart = NewAvailableAllocationUnits;
			
	}

	return status;
}

DWORD __stdcall SetupHook( HMODULE hOwnModule )
{
	HMODULE hNtDll = GetModuleHandleA( "ntdll" );
	if ( hNtDll == NULL ) return 1;

	pNtQueryVolumeInformationFile = (BYTE*)GetProcAddress( hNtDll, "NtQueryVolumeInformationFile" );
	if ( pNtQueryVolumeInformationFile == NULL ) return 1;	
	
	g_hSharedMemoryHandle = NULL;
	g_pSharedMemoryData = NULL;

	NtQueryVolumeInformationFile = (t_NtQueryVolumeInformationFile)HookSysCall( pNtQueryVolumeInformationFile, hkNtQueryVolumeInformationFile );

	DWORD dwTrys = NULL;

	while (true)
	{
		g_hSharedMemoryHandle = OpenSharedMapping( const_cast<char*>( SharedMemoryName ) );
		if ( g_hSharedMemoryHandle != NULL && g_hSharedMemoryHandle != INVALID_HANDLE_VALUE )
		{
			g_pSharedMemoryData = (SharedMemoryData*)GetSharedMemory( g_hSharedMemoryHandle, sizeof(SharedMemoryData) );
			if ( g_pSharedMemoryData != NULL )
				break;
			else
				CloseHandle( g_hSharedMemoryHandle );
		}
		Sleep( 1000 );
		dwTrys++;
		if ( dwTrys > 4 )
		{
			if ( g_bUseConfigFile != true )
			{
				g_bUseConfigFile = true;
				DWORD dwMaxPathLen = 1024;
				g_wConfigPath = (wchar_t*)malloc( dwMaxPathLen*2 );

				//get the path to the current module
				DWORD dwConfigPathLen = GetModuleFileNameW( hOwnModule, g_wConfigPath, dwMaxPathLen );
				if ( dwConfigPathLen < 3 ) //path is invalid
					g_bUseConfigFile = false;
				else
				{
					//replace the file ending with .ini
					g_wConfigPath[ dwConfigPathLen - 3 ] = 'i';
					g_wConfigPath[ dwConfigPathLen - 2 ] = 'n';
					g_wConfigPath[ dwConfigPathLen - 1 ] = 'i';
				}
			}
		}
	}
	g_bUseConfigFile = false;
	OutputDebugStringA( "VolumeInformationHook loaded!\n");

	return ERROR_SUCCESS;
}

void UnHook( )
{
	if ( pNtQueryVolumeInformationFile == NULL )
		return;
	UnHookSyscall( pNtQueryVolumeInformationFile, NtQueryVolumeInformationFile );

	UnmapViewOfFile(g_pSharedMemoryData);
	CloseHandle(g_hSharedMemoryHandle);

	OutputDebugStringA( "VolumeInformationHook unloaded!\n");
}

HANDLE hMainThread = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved //can be used for parameters
					 )
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		
		hMainThread = CreateThread( NULL, NULL, (LPTHREAD_START_ROUTINE)SetupHook, hModule, NULL, NULL );
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		TerminateThread( hMainThread, ERROR_SUCCESS );
		Sleep( 100 );
		UnHook();
		break;
	}
	return TRUE;
}
