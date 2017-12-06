#pragma once
#ifndef MAIN_H_
#define MAIN_H_
#include <Windows.h>

static const char SharedMemoryName[]="Global\\{645743562344534652}";

struct DriveInfo
{
	ULONG VolumeSerialNumber;
	ULONGLONG NewVolumeSize;
};

struct SharedMemoryData
{
	DWORD DrivesToModify;
	DriveInfo drive[40];

	DriveInfo* GetListEntry(ULONG Serial)
	{
		for (DWORD i = 0; i < this->DrivesToModify; i++)
		{
			if ( drive[i].VolumeSerialNumber == Serial )
				return &drive[i];
		}
		return NULL;
	}
};

static bool g_IsConfigFile = false;
#endif // MAIN_H_