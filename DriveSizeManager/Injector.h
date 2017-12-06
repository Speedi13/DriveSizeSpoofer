#pragma once
#ifndef INJECTOR_H_
#define INJECTOR_H_
#include <Windows.h>

//look for more information about DLL injection at Wikipedia
//https://en.wikipedia.org/wiki/DLL_injection

/// <summary>Load a DLL into a remote process</summary>
/// <param name="dwTargetPID">ProcessId of the target process</param>
/// <param name="DllPath">The path to the DLL to inject</param>
/// <returns>true on success</returns>
bool InjectDll( DWORD dwTargetPID, char* DllPath );

/// <summary>Unload a DLL from a remote process</summary>
/// <param name="dwTargetPID">ProcessId of the target process.</param>
/// <param name="DllPath">Path to the DLL to unload</param>
/// <returns>true on success</returns>
bool UnloadDll( DWORD dwTargetPID, char* DllPath );

//Ejection utilities:
DWORD_PTR GetRemoteModuleAddress(DWORD dwTargetPID, char *szModulePath );
bool UnloadDllEx( HANDLE hProcess, DWORD_PTR dwModuleAddressToUnload );

#include <vector>
/// <summary>Gets all ProcessIds from processes with the specified name</summary>
/// <param name="ProcName">process name to look for</param>
/// <returns>std::vector of ProcessIds</returns>
std::vector<DWORD> GetProcsId(char* ProcName);

#endif // INJECTOR_H_