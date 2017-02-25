#include <string>

#include "Hooks.h"
#include "CFishBot.h"

AppendChatFn orgAppendChat;
void __fastcall hkdAppendChat(void* pThis, int edx, int iType, const char * c_szChat)
{
	orgAppendChat(pThis, iType, c_szChat);

	if (strstr(c_szChat, "\x4E\x61\x63\x69\x9C\x6E\x69\x6A\x20\x53\x70\x61\x63\x6A\xEA\x3A")) // "Naciśnij spację..."
	{
		int iCount = atoi(&c_szChat[17]);

		if (iCount > 0 && iCount <= 5)
		{
			g_FishBot.MainBot(iCount); // i dont care ;3333 x razy, bad codezzzzzz
		}
	}
}