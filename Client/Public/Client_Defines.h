#pragma once

#include <cstdint>

namespace Client
{
	static const unsigned int	g_iWinSizeX{ 1280 };
	static const unsigned int	g_iWinSizeY{ 720 };

	enum class COLLISION_LAYER : uint32_t
	{
		DEFAULT = 1u << 0,
		PLAYER_BODY = 1u << 1,
		ENEMY_BODY = 1u << 2,
		WORLD = 1u << 3,
		PLAYER_ATTACK = 1u << 4,
		ENEMY_ATTACK = 1u << 5,
		INTERACTION = 1u << 6,
	};

}

extern HWND g_hWnd;
extern HINSTANCE g_hInstance;
