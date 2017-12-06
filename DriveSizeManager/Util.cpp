#include "Util.h"
#include <stdio.h>

/// <summary>pauses the execution and waits for user input to continue</summary>
void pause()
{
	printf("Press any key to continue . . .");

	HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
	if ( hStdIn == NULL || hStdIn == INVALID_HANDLE_VALUE )
		return;
	HWND hwndConsoleWindow = GetConsoleWindow();

	bool bIsKeyPressed = false;

	while (bIsKeyPressed == false)
	{
		DWORD dwEventCount = NULL;
		if ( GetNumberOfConsoleInputEvents(hStdIn, &dwEventCount) != TRUE ) break;
		if ( dwEventCount == NULL )
		{
			Sleep( 420 );
			continue;
		}
		INPUT_RECORD *NewEvents = new INPUT_RECORD[dwEventCount];
		if ( NewEvents == NULL ) break;

		if ( ReadConsoleInputA(hStdIn, NewEvents, dwEventCount, &dwEventCount) != TRUE )
		{
			delete [] NewEvents;
			continue;
		}
		
		for (DWORD i = 0; i < dwEventCount; i++)
		{
			if (NewEvents[i].EventType == KEY_EVENT)
			{
				if (hwndConsoleWindow != NULL)
				{
					if ( GetForegroundWindow() != hwndConsoleWindow )
						break;
				}
				bIsKeyPressed = true;
				break;
			}
		}
		delete [] NewEvents;

		if ( bIsKeyPressed == true )
			break;
	}
	printf("\n");

	return;
}

#include <ntstatus.h>
#include "..\VolumeInformationHook\WindowsNtStructs.h"

HMODULE g_hNtDll = NULL;
RtlDosPathNameToNtPathName_U_t RtlDosPathNameToNtPathName_U = NULL;
t_NtQueryVolumeInformationFile NtQueryVolumeInformationFile = NULL;

/// <summary>Gets the drive's serial number from its path</summary>
/// <param name="pVolumePath">drive path</param>
/// <returns>drives serial number on success otherwise it returns zero</returns>
ULONG GetVolueSerial(wchar_t* pVolumePath)
{
	if ( g_hNtDll == NULL )
		 g_hNtDll = GetModuleHandleA("ntdll");

	if ( RtlDosPathNameToNtPathName_U == NULL )
		 RtlDosPathNameToNtPathName_U = (RtlDosPathNameToNtPathName_U_t)GetProcAddress(g_hNtDll, "RtlDosPathNameToNtPathName_U");

	if ( NtQueryVolumeInformationFile == NULL )
		 NtQueryVolumeInformationFile = (t_NtQueryVolumeInformationFile)GetProcAddress(g_hNtDll, "NtQueryVolumeInformationFile");

	FILE_FS_FULL_SIZE_INFORMATION FileFsFullSizeInfo;
	ZeroMemory(&FileFsFullSizeInfo, sizeof(FILE_FS_FULL_SIZE_INFORMATION) );

	UNICODE_STRING unicodeVolumePath;
	ZeroMemory( &unicodeVolumePath, sizeof( UNICODE_STRING ) );

	RtlDosPathNameToNtPathName_U(pVolumePath, &unicodeVolumePath, NULL, NULL);

	OBJECT_ATTRIBUTES ObjectAttributes;
	ZeroMemory( &ObjectAttributes, sizeof( OBJECT_ATTRIBUTES ) );

	InitializeObjectAttributes(&ObjectAttributes, &unicodeVolumePath, OBJ_CASE_INSENSITIVE, NULL, NULL);

	HANDLE hHandle = NULL;
	IO_STATUS_BLOCK IoStatusBlock;
	ZeroMemory( &IoStatusBlock, sizeof( IO_STATUS_BLOCK ) );

	NTSTATUS NtOpenFileStatus = NtOpenFile(&hHandle, SYNCHRONIZE|FILE_LIST_DIRECTORY, &ObjectAttributes, &IoStatusBlock, FILE_SHARE_READ|FILE_SHARE_WRITE, FILE_DIRECTORY_FILE|FILE_SYNCHRONOUS_IO_NONALERT);
	RtlFreeUnicodeString(&unicodeVolumePath);

	if ( NtOpenFileStatus != STATUS_SUCCESS )
		return NULL;

	FILE_FS_VOLUME_INFORMATION FsVolumeInformation;
	ZeroMemory(&FsVolumeInformation, sizeof(FILE_FS_VOLUME_INFORMATION) );

	NTSTATUS QueryVolumeInformationFileStatus = NtQueryVolumeInformationFile(hHandle, &IoStatusBlock, &FsVolumeInformation, sizeof(FILE_FS_VOLUME_INFORMATION), FileFsVolumeInformation);
	if ( QueryVolumeInformationFileStatus != STATUS_SUCCESS )
		return NULL;

	NtClose(hHandle);

	return FsVolumeInformation.VolumeSerialNumber;
}

/// <summary>compares two ASCII strings without case sensitivity.</summary>
/// <param name="str1">first string</param>
/// <param name="str2">second string</param>
/// <returns>true on success</returns>
bool StrCmpToLower(char* str1, char* str2 )
{
	DWORD i = 0;
	while (true)
	{
		if ( tolower(str1[i]) != tolower(str2[i]) )	
			return false;
		if (str1[i] == NULL ) //both should be the same so checking one is ok
			return true;
		i++;
	}
	return false;
}

/// <summary>gets the dir in which the current executable is placed in</summary>
/// <param name="OutBuffer">buffer to write the result into</param>
/// <param name="MaxBufferLen">maximal length of the output buffer</param>
/// <returns>returns the directory text length</returns>
DWORD GetCurrentExecutableDirectory( char* OutBuffer, DWORD MaxBufferLen )
{
	DWORD dwStrLen = 
	GetModuleFileNameA( NULL, OutBuffer, MaxBufferLen );

	if ( dwStrLen < 3 )
		return NULL;

	for (DWORD i = dwStrLen; i > 0 ; i--)
	{
		if ( OutBuffer[i] == '\\' )
		{
			OutBuffer[i+1] = NULL;
			return i+1;
		}
	}
	return NULL;
}

//https://stackoverflow.com/questions/22047673/transfering-data-through-a-memory-mapped-file-using-win32-winapi
/// <summary>opens / creates a mapped file</summary>
/// <param name="SharedMemoryName">virtual file name</param>
/// <param name="dwSharedMemorySize">virtual file size</param>
/// <returns>returns a handle to the mapped file</returns>
HANDLE SetUpFileMapping(char* SharedMemoryName, DWORD dwSharedMemorySize)
{
	HANDLE hMapFile = NULL;

	hMapFile = OpenFileMappingA(
								FILE_MAP_ALL_ACCESS,
								FALSE,
								SharedMemoryName);
	if ( hMapFile != NULL && hMapFile != INVALID_HANDLE_VALUE )
		return hMapFile;

	//its very important to setup the Descriptor, 
	//without it only processes run as the same user and privilege can access the shared data
	SECURITY_DESCRIPTOR SecurityDescriptor;
	ZeroMemory( &SecurityDescriptor, NULL );
	if(InitializeSecurityDescriptor(&SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION) != TRUE)
		return hMapFile;

	if(SetSecurityDescriptorDacl(&SecurityDescriptor, TRUE, 0, FALSE) != TRUE)
		return hMapFile;

	SECURITY_ATTRIBUTES SecurityAttributes;
	SecurityAttributes.bInheritHandle = FALSE;
	SecurityAttributes.lpSecurityDescriptor = &SecurityDescriptor;
	SecurityAttributes.nLength=sizeof(SECURITY_DESCRIPTOR);

	hMapFile = CreateFileMappingA(
                 INVALID_HANDLE_VALUE,		// use paging file
                 &SecurityAttributes,
                 PAGE_READWRITE,			// read/write access
                 0,							// maximum object size (high-order DWORD)
                 dwSharedMemorySize,		// maximum object size (low-order DWORD)
                 SharedMemoryName);			// name of mapping object
	if ( hMapFile == NULL || hMapFile == INVALID_HANDLE_VALUE )
		return INVALID_HANDLE_VALUE;

	return hMapFile;
}

/// <summary>writes an unsigned long long to an ini-file</summary>
/// <param name="szFilePath">the path to the ini file</param>
/// <param name="szCategory">the category from the ini-file</param>
/// <param name="szSettingName">the setting to save</param>
/// <param name="Value">the new value for the setting</param>
void IniSaveSetting_Integer( wchar_t* szFilePath ,char* szCategory, char* szSettingName, ULONGLONG Value )
{
	int szCategoryLen = (int)strlen( szCategory );
	int szSettingNameLen = (int)strlen( szSettingName );

	wchar_t* wCategory = (wchar_t*)malloc(szCategoryLen*2+2);
	wchar_t* wSettingName = (wchar_t*)malloc(szSettingNameLen*2+2);

	MultiByteToWideChar( CP_ACP, NULL, szCategory, szCategoryLen+1, wCategory, szCategoryLen+1 );
	MultiByteToWideChar( CP_ACP, NULL, szSettingName, szSettingNameLen+1, wSettingName, szSettingNameLen+1 );

	char szbuffer[30];
	sprintf_s(szbuffer, "%llu", Value );

	wchar_t wbuffer[30];
	for (DWORD j = 0; j < 30; j++)
	{
		wbuffer[j] = szbuffer[j];
		if ( wbuffer[j] == NULL )
			break;
	}
	WritePrivateProfileStringW( wCategory, wSettingName, wbuffer, szFilePath );

	free( wCategory );
	free( wSettingName );
}

/// <summary>clears the console output</summary>
/// <param name="Callback">the function that will be called after the output is cleared</param>
void ResetConsole( t_ConsoleResetCallback* Callback )
{
	COORD topLeft = { 0, 0 };
	topLeft.X = topLeft.Y = NULL;

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
	ZeroMemory( &screen, sizeof(CONSOLE_SCREEN_BUFFER_INFO) );
    DWORD written = NULL;

    GetConsoleScreenBufferInfo(hStdOut, &screen);
    FillConsoleOutputCharacterA(
        hStdOut, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );

    FillConsoleOutputAttribute(
        hStdOut, screen.wAttributes,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );

    SetConsoleCursorPosition(hStdOut, topLeft);

	if ( Callback != NULL )
		 Callback();
}

/// <summary>checks if the file exists</summary>
/// <param name="pFilePath">the path to the file</param>
/// <returns>returns true if the file exists</returns>
bool DoesFileExistA( char* pFilePath )
{
	DWORD dwResult = GetFileAttributesA( pFilePath );
	if ( dwResult != INVALID_FILE_ATTRIBUTES )
		return true;
	return false;
}

/// <summary>checks if the current process is elevated</summary>
/// <returns>returns TRUE if the current process is elevated</returns>
BOOL IsElevated(void)
{
	BOOL bReturn = FALSE;

	TOKEN_ELEVATION tokenInformation;
	ZeroMemory( &tokenInformation, sizeof(TOKEN_ELEVATION) );

	HANDLE hToken = INVALID_HANDLE_VALUE;
	if(OpenProcessToken(INVALID_HANDLE_VALUE/*current process*/, TOKEN_QUERY, &hToken) != TRUE)
		return FALSE;

	DWORD dwReturnSize = 0;
	if(GetTokenInformation(hToken, TokenElevation, &tokenInformation, sizeof(TOKEN_ELEVATION), &dwReturnSize) == TRUE)
		bReturn = (BOOL)tokenInformation.TokenIsElevated;

	CloseHandle(hToken);
	return bReturn;
}

/// <summary>trys to restart the current process as elevated</summary>
/// <returns>returns true if it succeeded</returns>
bool TryToRestartElevated(void)
{
	wchar_t* pPath = (wchar_t*)malloc( 1024*2 );
	GetModuleFileNameW( NULL, pPath, 1024 );

	SHELLEXECUTEINFOW ShellExecuteInfo;
	ZeroMemory( &ShellExecuteInfo, sizeof( SHELLEXECUTEINFOW ) );

	ShellExecuteInfo.cbSize = sizeof( SHELLEXECUTEINFOW );
    ShellExecuteInfo.lpVerb = L"runas";
    ShellExecuteInfo.lpFile = pPath;
    ShellExecuteInfo.hwnd = NULL;
    ShellExecuteInfo.nShow = SW_NORMAL;

	bool bResult = false;

    if (ShellExecuteExW(&ShellExecuteInfo) == TRUE)
		bResult = true;

	free( pPath );

	return bResult;
}