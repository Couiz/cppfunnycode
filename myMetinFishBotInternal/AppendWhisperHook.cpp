#include <string>

#include "Hooks.h"
#include "wintoastlib.h"
#include "CPythonPlayer.h"
#include "CPythonChat.h"

using namespace WinToastLib;

std::wstring StringToWString(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}


AppendWhisperFn orgAppendWhisper;
void __fastcall hkdAppendWhisper(void* pThis, int edx, int iType, const char * c_szChat)
{
	orgAppendWhisper(pThis, iType, c_szChat);

	//g_pPythonChat->AppendChat(0, c_szChat); //test purpose :p

	if (strstr(c_szChat, g_pPythonPlayer->GetName()))
	{
		return;
	}

	std::wstring wText = StringToWString(c_szChat);

	WinToastTemplate templ = WinToastTemplate(WinToastTemplate::TextTwoLines);
	templ.setTextField(L"Szept", WinToastTemplate::TextField::FirstLine);
	templ.setTextField(wText, WinToastTemplate::TextField::SecondLine);

	if (!WinToast::instance()->showToast(templ, new WinToastHandler)) {
		g_pPythonChat->AppendChat(0, "Error: Could not launch your toast notification!");
	}
}