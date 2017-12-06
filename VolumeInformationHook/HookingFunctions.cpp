#include <Windows.h>
#include"HookingFunctions.h"

//look for more information about hooking there:
//https://en.wikipedia.org/wiki/Hooking
//https://www.unknowncheats.me/forum/programming-beginners/120047-make-your-own-dll-hack-2-winapi-hooking.html

/// <summary>places a hook at the address of the syscall specified address</summary>
/// <param name="pSyscallAddress">address of the syscall to hook</param>
/// <param name="HookFnc">address of the function to jump to</param>
/// <returns>returns the address of the copy of the original syscall</returns>
void* UnHookSyscall( void* SyscallAddress, void* SyscallCopyAddress )
{
	DWORD dwSyscallSize = NULL;
	for ( dwSyscallSize = 0;  ; dwSyscallSize++)
	{
		if ( dwSyscallSize > 35 ){ dwSyscallSize = NULL; break; }
		if (  *(BYTE*)( (DWORD_PTR)SyscallCopyAddress + dwSyscallSize + 0) == 0xCC &&
			  *(BYTE*)( (DWORD_PTR)SyscallCopyAddress + dwSyscallSize + 1) == 0xCC )
		{
			break;
		}
	}
	if ( dwSyscallSize == NULL )
		return NULL;

	DWORD dwOldProtection = NULL;
	VirtualProtect( SyscallAddress, dwSyscallSize, PAGE_EXECUTE_READWRITE, &dwOldProtection );

	for (DWORD j = 0; j < dwSyscallSize; j++)
		*(BYTE*)( (DWORD_PTR)SyscallAddress + j ) = *(BYTE*)( (DWORD_PTR)SyscallCopyAddress + j );
	
	VirtualProtect( SyscallAddress, dwSyscallSize, dwOldProtection, NULL );

	VirtualFree( SyscallCopyAddress, NULL, MEM_RELEASE );

	return SyscallAddress;
}

/// <summary>restores the syscall to the original bytes</summary>
/// <param name="pSyscallAddress">address of the syscall to restore</param>
/// <param name="SyscallCopyAddress">address of the copy from the original syscall</param>
/// <returns>returns the address of the original syscall</returns>
void* HookSysCall( void* pSyscallAddress, void* HookFnc, void* pOriCallAddress  /*if set it will copy the syscall bytes there*/ )
{
	DWORD dwSyscallSize = NULL;
	for ( dwSyscallSize = 0;  ; dwSyscallSize++)
	{
		if ( dwSyscallSize > 30 ){ dwSyscallSize = NULL; break; }

		if (
#ifdef _AMD64_ //for x64 Architecture
		     *(BYTE*)( (DWORD64)pSyscallAddress + dwSyscallSize + 0) == 0x05 && 
			 *(BYTE*)( (DWORD64)pSyscallAddress + dwSyscallSize + 1) == 0xC3
#else 
			 *(BYTE*)( (DWORD_PTR)pSyscallAddress + dwSyscallSize + 0) == 0xC4 && 
			 *(BYTE*)( (DWORD_PTR)pSyscallAddress + dwSyscallSize + 2) == 0xC2
#endif
		)
		{
#ifdef _AMD64_ //for x64 Architecture
			dwSyscallSize += 2;
#else //for x86 Architecture
			dwSyscallSize += 5;
#endif
			break;
		}
	}
	if ( dwSyscallSize == NULL ) return NULL;

	if ( pOriCallAddress == NULL )
	{
		pOriCallAddress = (void*)VirtualAlloc( NULL, dwSyscallSize+2, MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);
		for (DWORD k = 0; k < dwSyscallSize; k++)
			*(BYTE*)( (DWORD_PTR)pOriCallAddress + k ) = *(BYTE*)( (DWORD_PTR)pSyscallAddress + k );

		*(BYTE*)( (DWORD_PTR)pOriCallAddress + dwSyscallSize + 0 ) = 0xCC;
		*(BYTE*)( (DWORD_PTR)pOriCallAddress + dwSyscallSize + 1 ) = 0xCC;
		
	}
	else
	{
		for (DWORD k = 0; k < dwSyscallSize; k++)
			*(BYTE*)( (DWORD_PTR)pOriCallAddress + k ) = *(BYTE*)( (DWORD_PTR)pSyscallAddress + k );

		*(BYTE*)( (DWORD_PTR)pOriCallAddress + dwSyscallSize + 0 ) = 0xCC;
		*(BYTE*)( (DWORD_PTR)pOriCallAddress + dwSyscallSize + 1 ) = 0xCC;
	}
	//printf("pOriCallAddress = %#p\n",pOriCallAddress);

	DWORD dwOldProtection = NULL;
	VirtualProtect( pSyscallAddress, 15, PAGE_EXECUTE_READWRITE, &dwOldProtection );

	DWORD64 dwDist = NULL;
	if ( (DWORD64)HookFnc > (DWORD64)pSyscallAddress )
		dwDist = (DWORD64)HookFnc-(DWORD64)pSyscallAddress;
	if ( (DWORD64)HookFnc < (DWORD64)pSyscallAddress )
		dwDist = (DWORD64)pSyscallAddress-(DWORD64)HookFnc;

	if ( (DWORD64)dwDist < (DWORD64)0x00000000FFFFFFFF )
	{
		//x86 jump
		*(BYTE*)( (DWORD64)pSyscallAddress + 0x00 ) = 0xE9;
		*(__int32*)( (DWORD64)pSyscallAddress + 0x01 ) = CalculateRelativePtr( (void*)( (DWORD64)pSyscallAddress+1 ), HookFnc );
	}
	else
	{
		//x64 jump
		BYTE push_ret[] = {
			0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*mov rax,0000000000000000*/ 
			0x50														/*push rax*/, 
			0xC3														/*ret*/
		};

		*(DWORD64*)((DWORD64)push_ret + 0x2 ) = (DWORD64)( HookFnc );
		*(DWORD64*)( (DWORD64)pSyscallAddress + 0x00 ) = *(DWORD64*)( (DWORD64)&push_ret[0] + 0x00 );
		*(DWORD64*)( (DWORD64)pSyscallAddress + 0x08 ) = *(DWORD64*)( (DWORD64)&push_ret[0] + 0x08 );

	}
	VirtualProtect( pSyscallAddress, 15, dwOldProtection, NULL );

	return pOriCallAddress;
}

/// <summary>calculate the relative offset between the two specific addresses</summary>
/// <param name="pCodeAddress">from address</param>
/// <param name="pToAddress">to address</param>
/// <returns>returns the relative offset</returns>
__int32 CalculateRelativePtr( void* pCodeAddress, void* pToAddress )
{
	DWORD_PTR diff = (DWORD_PTR)pToAddress - (DWORD_PTR)pCodeAddress;
	__int32 Offset = (__int32)diff - sizeof(__int32);
	return Offset ;
}