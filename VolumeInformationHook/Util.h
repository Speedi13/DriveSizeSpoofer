#pragma once
#ifndef UTIL_H_
#define UTIL_H_

#include <Windows.h>

/// <summary>gets a handle to a named virtual memory</summary>
/// <param name="SharedMemoryName">name</param>
/// <returns>returns a handle</returns>
HANDLE OpenSharedMapping( char* SharedMemoryName );

/// <summary>maps the virtual file into the local address space and returns the memory address of it</summary>
/// <param name="hHandle">handle to the virtual memory</param>
/// <param name="Size">size of the virtual memory</param>
/// <returns>returns the memory address</returns>
void* GetSharedMemory( HANDLE hHandle, SIZE_T Size );

/// <summary>reads an unsigned long long from an ini-file</summary>
/// <param name="szFilePath">the path to the ini file</param>
/// <param name="szCategory">the category from the ini-file</param>
/// <param name="szSettingName">the setting to read</param>
/// <param name="Value">a pointer to the output value</param>
ULONGLONG IniLoadSetting_Integer( wchar_t* szFilePath ,char* szCategory, char* szSettingName, ULONGLONG* Value );

#endif // UTIL_H_