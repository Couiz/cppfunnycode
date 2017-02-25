#pragma once


class CPythonChat
{
public:
	virtual void __vfunc0() = 0;
	virtual void AppendChat(int iType, const char * c_szChat) = 0;
};
extern CPythonChat* g_pPythonChat;