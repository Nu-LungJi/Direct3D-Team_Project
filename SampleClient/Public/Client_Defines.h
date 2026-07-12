#pragma once

namespace Client
{
	static const unsigned int	g_iWinSizeX{ 1280 };
	static const unsigned int	g_iWinSizeY{ 720 };


	enum class TURN { LEFT_45, LEFT_90, LEFT_135, LEFT_180, RIGHT_45, RIGHT_90, RIGHT_135, RIGHT_180, END };
	enum class PARTES {WEAPON,END};
}

extern HWND g_hWnd;
extern HINSTANCE g_hInstance;
