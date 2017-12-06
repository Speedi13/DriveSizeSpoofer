#pragma once
#pragma comment(lib, "ntdll.lib")
#ifndef WINDOWS_NT_STRUCTS_H_
#define WINDOWS_NT_STRUCTS_H_

#include <Windows.h>
#include <Winternl.h>

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsVolumeFlagsInformation,// 10
    FileFsSectorSizeInformation, // 11
    FileFsDataCopyInformation,   // 12
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef NTSTATUS (NTAPI *t_NtQueryVolumeInformationFile)( _In_ HANDLE FileHandle,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_writes_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FS_INFORMATION_CLASS FsInformationClass);
//t_NtQueryVolumeInformationFile NtQueryVolumeInformationFile = NULL;
//extern t_NtQueryVolumeInformationFile NtQueryVolumeInformationFile;

typedef struct _FILE_FS_VOLUME_INFORMATION {
  LARGE_INTEGER VolumeCreationTime;
  ULONG         VolumeSerialNumber;
  ULONG         VolumeLabelLength;
  BOOLEAN       SupportsObjects;
  WCHAR         VolumeLabel[1];
  char buffer[0x500];
} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
  LARGE_INTEGER TotalAllocationUnits;
  LARGE_INTEGER CallerAvailableAllocationUnits;
  LARGE_INTEGER ActualAvailableAllocationUnits;
  ULONG         SectorsPerAllocationUnit;
  ULONG         BytesPerSector;
} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;


typedef struct _RTLP_CURDIR_REF
{
    LONG RefCount;
    HANDLE Handle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

typedef struct RTL_RELATIVE_NAME_U {
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

typedef BOOLEAN (__stdcall *RtlDosPathNameToNtPathName_U_t)(PCWSTR,UNICODE_STRING*,PCWSTR*,PRTL_RELATIVE_NAME_U);
//RtlDosPathNameToNtPathName_U_t RtlDosPathNameToNtPathName_U = NULL;
//extern RtlDosPathNameToNtPathName_U_t RtlDosPathNameToNtPathName_U;

#endif // WINDOWS_NT_STRUCTS_H_