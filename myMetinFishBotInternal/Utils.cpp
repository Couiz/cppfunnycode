#include "Utils.h"

ULONG Utils::PatternScan(std::string sModuleName, PBYTE pbPattern, std::string sMask,
	ULONG uCodeBase, ULONG uSizeOfCode)
{
	BOOL bPatternDidMatch = FALSE;
	HMODULE hModule = GetModuleHandleA(sModuleName.c_str());

	if (!hModule)
		return 0x0;

	PIMAGE_DOS_HEADER pDsHeader = PIMAGE_DOS_HEADER(hModule);
	PIMAGE_NT_HEADERS pPeHeader = PIMAGE_NT_HEADERS(LONG(hModule) + pDsHeader->e_lfanew);
	PIMAGE_OPTIONAL_HEADER pOptionalHeader = &pPeHeader->OptionalHeader;

	if (uCodeBase == 0x0)
		uCodeBase = (ULONG)hModule + pOptionalHeader->BaseOfCode;

	if (uSizeOfCode == 0x0)
		uSizeOfCode = pOptionalHeader->SizeOfCode;

	ULONG uArraySize = sMask.length();

	if (!uCodeBase || !uSizeOfCode || !uArraySize)
		return 0x0;

	for (size_t i = uCodeBase; i <= uCodeBase + uSizeOfCode; i++)
	{
		for (size_t t = 0; t < uArraySize; t++)
		{
			if (*((PBYTE)i + t) == pbPattern[t] || sMask.c_str()[t] == '?')
				bPatternDidMatch = TRUE;

			else
			{
				bPatternDidMatch = FALSE;
				break;
			}
		}

		if (bPatternDidMatch)
			return i;
	}

	return 0x0;
} // credits to Interwebz staff

bool Utils::LoadPyScript(const char* str)
{
	typedef int(*PyRun_SimpleStringFn)(const char *);

	HMODULE hPython = GetModuleHandle(L"python27.dll");
	PyRun_SimpleStringFn PyRun_SimpleString = (PyRun_SimpleStringFn)GetProcAddress(hPython, (LPCSTR)"PyRun_SimpleString");

	int iRet = PyRun_SimpleString(str);
	if (iRet == -1)	// < 0 ?!?!??
	{
		return false;
	}
	return true;
}

std::wstring Utils::StringToWString(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}