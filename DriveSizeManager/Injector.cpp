#include "Injector.h"
#include "Util.h"

//look for more information about DLL injection at Wikipedia
//https://en.wikipedia.org/wiki/DLL_injection

/// <summary>Load a DLL into a remote process</summary>
/// <param name="dwTargetPID">ProcessId of the target process</param>
/// <param name="DllPath">The path to the DLL to inject</param>
/// <returns>true on success</returns>
bool InjectDll( DWORD dwTargetPID, char* DllPath )
{
	//Note that I did these comments some time ago to show a friend how dll-injection works
	size_t DllPathLen = strlen(DllPath);
	if ( DllPathLen == NULL )
		return false;

	// The System Dlls get mapped to the same address in every process :D
	static HMODULE hKernel32 = NULL;
	if ( hKernel32 == NULL )
		 hKernel32 = GetModuleHandleA("kernel32.dll");
	static void* FncLoadLibraryAddr = NULL;
	if ( FncLoadLibraryAddr == NULL )
		 FncLoadLibraryAddr = (PVOID)GetProcAddress(hKernel32, "LoadLibraryA");

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwTargetPID);
	if ( hProcess == INVALID_HANDLE_VALUE || hProcess == NULL )
		return false;

	//Allocate space in the targets process for our string
	char* pDllPath = (char*)VirtualAllocEx(hProcess, 0, DllPathLen+5, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if ( pDllPath == NULL ) //check if remote allocation failed
		{ CloseHandle( hProcess ); return false;}

	//write the string into the fresh allocated space:
	BOOL bWriteProcMemSuccess =
	WriteProcessMemory(hProcess, pDllPath, (LPVOID)DllPath, DllPathLen, NULL);
	if ( bWriteProcMemSuccess != TRUE )
		{ CloseHandle( hProcess ); return false;}

	//Remote starting LoadLibrary:
	HANDLE hThread = CreateRemoteThread(	hProcess, 
											NULL, 
											NULL, 
											(LPTHREAD_START_ROUTINE)FncLoadLibraryAddr,
											pDllPath, //<= The parameter
											NULL, 
											NULL
										);
	if ( hThread == NULL || hThread == INVALID_HANDLE_VALUE )
		{  CloseHandle( hProcess ); return false;}

	//Lets wait till LoadLibrary is finished:
	WaitForSingleObject( hThread, INFINITE );

	Sleep( 100 );

	//the dll path is not longer needed lets free that:
	VirtualFreeEx( hProcess, pDllPath, NULL, MEM_RELEASE );

	CloseHandle( hProcess );

	return true;
}

#include <TlHelp32.h>
DWORD_PTR GetRemoteModuleAddress(DWORD dwTargetPID, char *szModulePath )
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwTargetPID);
	if (hSnap == INVALID_HANDLE_VALUE || hSnap == NULL )
		return NULL;

	MODULEENTRY32 ModuleEntry32;
	ZeroMemory( &ModuleEntry32, sizeof( MODULEENTRY32 ) );
	ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
	
	if (Module32First(hSnap, &ModuleEntry32))
	{
		while (true)
		{
			//printf("me.szExePath = \"%s\"\n",me.szExePath);
			if (StrCmpToLower(ModuleEntry32.szExePath, szModulePath) == true)
			{
				
				CloseHandle(hSnap);
				return (DWORD_PTR)ModuleEntry32.modBaseAddr;
			}
			if ( Module32Next(hSnap, &ModuleEntry32) != TRUE ) 
				break;
		}
	}
	CloseHandle(hSnap);
	return NULL;
}

bool UnloadDllEx( HANDLE hProcess, DWORD_PTR dwModuleAddressToUnload )
{
	static HMODULE hNtDll = NULL;
	if ( hNtDll == NULL )
		 hNtDll = GetModuleHandleA("ntdll.dll");

	static DWORD64 LdrUnloadDll = NULL;
	if ( LdrUnloadDll == NULL )
		 LdrUnloadDll = (DWORD64)GetProcAddress(hNtDll, "LdrUnloadDll");

	HANDLE hThread = CreateRemoteThread( hProcess, 
										 NULL, 
										 NULL, 
										 (LPTHREAD_START_ROUTINE)LdrUnloadDll, 
										 (LPVOID)dwModuleAddressToUnload, 
										 NULL, 
										 NULL 
									   );

	if ( hThread == INVALID_HANDLE_VALUE || hThread == NULL )
	{
		CloseHandle( hProcess );
		return false;
	}
	CloseHandle( hThread );
	return true;
}

/// <summary>Unload a DLL from a remote process</summary>
/// <param name="dwTargetPID">ProcessId of the target process.</param>
/// <param name="DllPath">Path to the DLL to unload</param>
/// <returns>true on success</returns>
bool UnloadDll( DWORD dwTargetPID, char* DllPath )
{
	DWORD_PTR dwAddress = GetRemoteModuleAddress( dwTargetPID, DllPath );
	if ( dwAddress == NULL )
		return false; //dll seems to be not loaded

	HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, dwTargetPID );
	if ( hProcess == NULL || hProcess == INVALID_HANDLE_VALUE )
		return false;

	if ( UnloadDllEx( hProcess, dwAddress ) != true )
	{
		CloseHandle( hProcess );
		return false;
	}

	CloseHandle( hProcess );
	return true;
}

/// <summary>Gets all ProcessIds from processes with the specified name</summary>
/// <param name="ProcName">process name to look for</param>
/// <returns>std::vector of ProcessIds</returns>
std::vector<DWORD> GetProcsId(char* ProcName)
{
	std::vector<DWORD> result; result.clear();

	PROCESSENTRY32 ProcEntry32;
	ZeroMemory( &ProcEntry32, sizeof(PROCESSENTRY32) );
	ProcEntry32.dwSize = sizeof( PROCESSENTRY32 );

	HANDLE hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( hSnapshot == NULL || hSnapshot == INVALID_HANDLE_VALUE )
		return result;

	if ( Process32First( hSnapshot, &ProcEntry32 ) != TRUE )
		return result;

	while (true)
	{
		if (StrCmpToLower(ProcEntry32.szExeFile, ProcName) == true)
		{
			DWORD dwProcessID = ProcEntry32.th32ProcessID;
			result.push_back( dwProcessID );
		}
		if ( Process32Next( hSnapshot, &ProcEntry32 ) != TRUE ) break;
	}
	CloseHandle( hSnapshot );

	return result;
}