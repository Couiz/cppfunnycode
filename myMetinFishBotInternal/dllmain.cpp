#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "CVMTHookManager.h"
#include "CPythonChat.h"
#include "CPythonPlayer.h"
#include "wintoastlib.h"
#include "Hooks.h"
#include "CPatternScan.h"
#include "Utils.h"

CPythonChat* g_pPythonChat;
CPythonPlayer* g_pPythonPlayer;

CVMTHookManager pythonChatVMT;

using namespace WinToastLib;

BOOL WINAPI OnAttach()
{
	DWORD hGame = (DWORD)GetModuleHandle(L"game.bin");
	if (!hGame)
		return false;


	//"game.bin"+00308944
	g_pPythonChat = (CPythonChat*)*(DWORD*)(hGame + 0x308944);
	if (g_pPythonChat == nullptr)
		return false;


	//"game.bin"+00306418
	g_pPythonPlayer = (CPythonPlayer*)*(DWORD*)(hGame + 0x306418);
	if (g_pPythonPlayer == nullptr)
		return false;


	//AppendChat hook
	pythonChatVMT.bInitialize((PDWORD*)g_pPythonChat);
	orgAppendChat = (AppendChatFn)pythonChatVMT.dwHookMethod((DWORD)hkdAppendChat, 1);

	//AppendWhisper hook

	//	game.bin+FA312 - 8B 55 08              - mov edx,[ebp+08]
	//	game.bin+FA315 - 52                    - push edx
	//	game.bin+FA316 - 8B 4D D0              - mov ecx,[ebp-30]
	//	game.bin+FA319 - E8 62E8FFFF           - call game.bin+F8B80       <---------- we are here after E8
	//	game.bin+FA31E - 8B 4D F4              - mov ecx,[ebp-0C]
	//	game.bin+FA321 - 64 89 0D 00000000     - mov fs:[00000000],ecx { 0 }
	//	game.bin+FA328 - 59                    - pop ecx

	DWORD* callAppendWhisper = (DWORD*)(hGame + 0xFA31A);
	orgAppendWhisper = (AppendWhisperFn)((DWORD)callAppendWhisper + (DWORD)*callAppendWhisper + 4); 
	// + 4 because of 4bytes address we need to skip it... relative address
	*callAppendWhisper = (DWORD)hkdAppendWhisper - (DWORD)callAppendWhisper - 4;

	
	//other shit initialization
	WinToast::instance()->setAppName(L"ClassicMT2 Notification");
	WinToast::instance()->setAppUserModelId(WinToast::configureAUMI(L"COUIZDEV", L"ClassicMT2", L"ClassicMT2", L"00001"));
	if (!WinToast::instance()->initialize())
	{
		MessageBox(NULL, L"Error, your system in not compatible!", L"Error", 0);
		return false;
	}

	if (!Utils::LoadPyScript("import uniqueee"))	// import file with fishbot i mean menu etc.
	{
		MessageBox(NULL, L"Failed injecting python script / uniqueee.py not found.", L"Error", 0);
		return false;
	}

	MessageBox(NULL, L"Have fun :D", L"Have fun :D ~Couiz", 0);

	return true;
	//fcking crap init end;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		HANDLE hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)OnAttach, NULL, NULL, NULL);
		CloseHandle(hThread);
		return true;
	}	
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return false;
}