#pragma once
#include "Engine_Defines.h"
struct UIViewport
{
	float scale;
	float offsetX;
	float offsetY;

	static UIViewport FromSize(float width, float height) {
		const float scale =std::min(width / 1280.f, height / 720.f);

		return { scale, (width - 1280.f * scale) * 0.5f, (height - 720.f * scale) * 0.5f };
	}

	// UI 설계 좌표 → 렌더 화면 좌표
	_float2 ToScreen(_float2 position) const {
		return { offsetX + position.x * scale, offsetY + position.y * scale };
	}

	// 렌더 화면 좌표 → UI 설계 좌표
	_float2 ToUI(_float2 position) const {
		return { (position.x - offsetX) / scale, (position.y - offsetY) / scale };
	}
};
