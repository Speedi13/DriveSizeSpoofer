#pragma once
#ifndef HOOKINGFUNCTIONS_H_
#define HOOKINGFUNCTIONS_H_

//look for more information about hooking there:
//https://en.wikipedia.org/wiki/Hooking
//https://www.unknowncheats.me/forum/programming-beginners/120047-make-your-own-dll-hack-2-winapi-hooking.html


/// <summary>places a hook at the address of the syscall specified address</summary>
/// <param name="pSyscallAddress">address of the syscall to hook</param>
/// <param name="HookFnc">address of the function to jump to</param>
/// <returns>returns the address of the copy of the original syscall</returns>
void* HookSysCall( void* pSyscallAddress, void* HookFnc, void* pOriCallAddress = NULL );

/// <summary>restores the syscall to the original bytes</summary>
/// <param name="pSyscallAddress">address of the syscall to restore</param>
/// <param name="SyscallCopyAddress">address of the copy from the original syscall</param>
/// <returns>returns the address of the original syscall</returns>
void* UnHookSyscall( void* SyscallAddress, void* SyscallCopyAddress );

/// <summary>calculate the relative offset between the two specific addresses</summary>
/// <param name="pCodeAddress">from address</param>
/// <param name="pToAddress">to address</param>
/// <returns>returns the relative offset</returns>
__int32 CalculateRelativePtr( void* pCodeAddress, void* pToAddress );

#endif // HOOKINGFUNCTIONS_H_