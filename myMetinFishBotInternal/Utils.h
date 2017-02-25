#pragma once
#include <Windows.h>
#include <string>

namespace Utils
{
	extern ULONG PatternScan(
		std::string ModuleName, 
		PBYTE Pattern, 
		std::string Mask, 
		ULONG uCodeBase, 
		ULONG uSizeOfCode
	);

	extern bool LoadPyScript(
		const char* str
	);

	extern std::wstring StringToWString(
		const std::string& str
	);
}
