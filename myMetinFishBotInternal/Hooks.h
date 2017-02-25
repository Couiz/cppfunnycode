#pragma once

typedef void(__thiscall *AppendChatFn)(void*, int, const char *);

extern AppendChatFn orgAppendChat;
void __fastcall hkdAppendChat(void* pThis, int edx, int iType, const char * c_szChat);



typedef void(__thiscall *AppendWhisperFn)(void*, int, const char *);

extern AppendWhisperFn orgAppendWhisper;
void __fastcall hkdAppendWhisper(void* pThis, int edx, int iType, const char * c_szChat);
