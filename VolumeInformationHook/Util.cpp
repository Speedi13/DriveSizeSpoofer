#include <Windows.h>

//https://stackoverflow.com/questions/22047673/transfering-data-through-a-memory-mapped-file-using-win32-winapi
/// <summary>gets a handle to a named virtual memory</summary>
/// <param name="SharedMemoryName">name</param>
/// <returns>returns a handle</returns>
HANDLE OpenSharedMapping( char* SharedMemoryName )
{
	HANDLE hMapFile = NULL;

	hMapFile = OpenFileMappingA(
								FILE_MAP_ALL_ACCESS,
								FALSE,
								SharedMemoryName);
	if ( hMapFile != NULL && hMapFile != INVALID_HANDLE_VALUE )
		return hMapFile;

	return NULL;
}

/// <summary>maps the virtual file into the local address space and returns the memory address of it</summary>
/// <param name="hHandle">handle to the virtual memory</param>
/// <param name="Size">size of the virtual memory</param>
/// <returns>returns the memory address</returns>
void* GetSharedMemory( HANDLE hHandle, SIZE_T Size )
{
	void* pAddress = 
	MapViewOfFile(	hHandle, // handle to map object
					FILE_MAP_READ,  // read only permission
					NULL,
					NULL,
					Size);

	return pAddress;
}

/// <summary>converts string with a number to a number</summary>
/// <param name="s">number string, no heximal number!</param>
/// <returns>returns an unsigned 64Bit number</returns>
DWORD64 _atoi(wchar_t s[])
{
    DWORD64 i, n;
    n = 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9'; ++i)
        n = 10 * n + (s[i] - '0');
    return n;
}

/// <summary>reads an unsigned long long from an ini-file</summary>
/// <param name="szFilePath">the path to the ini file</param>
/// <param name="szCategory">the category from the ini-file</param>
/// <param name="szSettingName">the setting to read</param>
/// <param name="Value">a pointer to the output value</param>
ULONGLONG IniLoadSetting_Integer( wchar_t* szFilePath ,char* szCategory, char* szSettingName, ULONGLONG* Value )
{
	int szCategoryLen = (int)strlen( szCategory );
	int szSettingNameLen = (int)strlen( szSettingName );

	wchar_t* wCategory = (wchar_t*)malloc(szCategoryLen*2+2);
	wchar_t* wSettingName = (wchar_t*)malloc(szSettingNameLen*2+2);

	MultiByteToWideChar( CP_ACP, NULL, szCategory, szCategoryLen+1, wCategory, szCategoryLen+1 );
	MultiByteToWideChar( CP_ACP, NULL, szSettingName, szSettingNameLen+1, wSettingName, szSettingNameLen+1 );

	//*(T*)Value = (T)GetPrivateProfileIntW( wCategory, wSettingName, (INT)*Value, szFilePath );

	wchar_t buffer[30];
	buffer[0]='0';
	buffer[1]=0;
	GetPrivateProfileStringW( wCategory, wSettingName, L"0", buffer, 30, szFilePath );

	*(ULONGLONG*)Value = _atoi( buffer );

	free( wCategory );
	free( wSettingName );

	return *(LONGLONG*)Value;
}


/* //can be used for debugging purposes
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
void CreateConsole( )
{
	if ( !AllocConsole( ) )
	{
		return;
	}

	auto lStdHandle = GetStdHandle( STD_OUTPUT_HANDLE );
	auto hConHandle = _open_osfhandle( PtrToUlong( lStdHandle ), _O_TEXT );
	auto fp = _fdopen( hConHandle, "w" );

	freopen_s( &fp, "CONOUT$", "w", stdout );

	*stdout = *fp;
	setvbuf( stdout, NULL, _IONBF, 0 );
}
*/