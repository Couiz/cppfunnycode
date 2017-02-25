#pragma once


class CPythonPlayer
{
public:
	void SetAttackKeyState(bool isPress);
	const char* GetName();
};
extern CPythonPlayer* g_pPythonPlayer;