#pragma once
//enum State
//{
//	PULL,
//	WAIT,
//	FISHING,
//};

class CFishBot
{
public:
	CFishBot();
	~CFishBot();

	void MainBot(int iClicks);

	static void SpaceClick(int iClicks);

private:
	static bool m_isFishing;
};
extern CFishBot g_FishBot;