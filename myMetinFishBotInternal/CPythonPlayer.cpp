#include "CPythonPlayer.h"
#include "CPatternScan.h"
#include "Utils.h"

#define NICKNAME_OFFSET 0x14

void CPythonPlayer::SetAttackKeyState(bool isPress)
{
	typedef void(__thiscall *OriginalFn)(void*, bool);
	static OriginalFn fnSetAttackKeyState = (OriginalFn)Utils::PatternScan("game.bin", (PBYTE)"\x55\x8B\xEC\x83\xEC\x08\x89\x4D\xFC\x0F\xB6\x45\x08", "xxxxxxxxxxxxx", 0, 0);

	if (!fnSetAttackKeyState)
	{
		return;
	}

	fnSetAttackKeyState(this, isPress);
}

const char * CPythonPlayer::GetName()
{
	return (const char*)(this + NICKNAME_OFFSET);
}
