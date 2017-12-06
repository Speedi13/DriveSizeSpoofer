#pragma once
#ifndef UTIL_H_
#define UTIL_H_
#include <Windows.h>

/// <summary>pauses the execution and waits for user input to continue</summary>
void pause(void);

/// <summary>Gets the drive's serial number from its path</summary>
/// <param name="pVolumePath">drive path</param>
/// <returns>drives serial number on success otherwise it returns zero</returns>
ULONG GetVolueSerial(wchar_t* pVolumePath);

/// <summary>compares two ASCII strings without case sensitivity.</summary>
/// <param name="str1">first string</param>
/// <param name="str2">second string</param>
/// <returns>true on success</returns>
bool StrCmpToLower(char* str1, char* str2 );

/// <summary>gets the dir in which the current executable is placed in</summary>
/// <param name="OutBuffer">buffer to write the result into</param>
/// <param name="MaxBufferLen">maximal length of the output buffer</param>
/// <returns>returns the directory text length</returns>
DWORD GetCurrentExecutableDirectory( char* OutBuffer, DWORD MaxBufferLen );

//https://stackoverflow.com/questions/22047673/transfering-data-through-a-memory-mapped-file-using-win32-winapi
/// <summary>opens / creates a mapped file</summary>
/// <param name="SharedMemoryName">virtual file name</param>
/// <param name="dwSharedMemorySize">virtual file size</param>
/// <returns>returns a handle to the mapped file</returns>
HANDLE SetUpFileMapping(char* SharedMemoryName, DWORD dwSharedMemorySize);

/// <summary>writes an unsigned long long to an ini-file</summary>
/// <param name="szFilePath">the path to the ini file</param>
/// <param name="szCategory">the category from the ini-file</param>
/// <param name="szSettingName">the setting to save</param>
/// <param name="Value">the new value for the setting</param>
void IniSaveSetting_Integer( wchar_t* szFilePath ,char* szCategory, char* szSettingName, ULONGLONG Value );

typedef void t_ConsoleResetCallback(void);
/// <summary>clears the console output</summary>
/// <param name="Callback">the function that will be called after the output is cleared</param>
void ResetConsole( t_ConsoleResetCallback* Callback = NULL );

/// <summary>checks if the file exists</summary>
/// <param name="pFilePath">the path to the file</param>
/// <returns>returns true if the file exists</returns>
bool DoesFileExistA( char* pFilePath );

/// <summary>checks if the current process is elevated</summary>
/// <returns>returns TRUE if the current process is elevated</returns>
BOOL IsElevated(void);

/// <summary>trys to restart the current process as elevated</summary>
/// <returns>returns true if it succeeded</returns>
bool TryToRestartElevated(void);

#endif // UTIL_H_