#include "CFishBot.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include "CPythonPlayer.h"
#include <time.h> 
#include <thread>
#include <chrono>

CFishBot g_FishBot;
bool CFishBot::m_isFishing;

CFishBot::CFishBot()
{
	srand(time(NULL));

	m_isFishing = false;
}

CFishBot::~CFishBot()
{
}

void CFishBot::MainBot(int iClicks)
{
	if (!m_isFishing)
	{
		std::thread tSpaceClick(SpaceClick, iClicks); // should use CreateThread... safer
		tSpaceClick.detach();
	}
}

void CFishBot::SpaceClick(int iClicks)
{
	m_isFishing = true; // xd

	std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 420 + 320));

	for (size_t i = 0; i < iClicks; i++)
	{
		g_pPythonPlayer->SetAttackKeyState(TRUE);
		std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 60 + 30));

		g_pPythonPlayer->SetAttackKeyState(FALSE);
		std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 170 + 130));

	}

	std::this_thread::sleep_for(std::chrono::milliseconds(5000)); //autistic workaround

	m_isFishing = false; // xd
}
