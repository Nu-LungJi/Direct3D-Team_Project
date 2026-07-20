#pragma once

#include "Handle.h"
namespace Engine
{
	class CGameObject;

	enum class PX_ACTOR_TYPE : uint8_t
	{
		RIGID_BODY,
		CHARACTER_CONTROLLER,
		RAGDOLL_BONE,
		ARTICULATION_LINK
	};

	enum class PX_SHAPE_TYPE : uint8_t
	{
		BOX,
		SPHERE,
		CAPSULE,
		TRIANGLE_MESH,
		RAGDOLL
	};

	struct PX_ACTOR_USER_DATA
	{
		CHandle hGameObject{};
		PX_ACTOR_TYPE eType{ PX_ACTOR_TYPE::RIGID_BODY };
		uint32_t iSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	struct PX_SHAPE_USER_DATA
	{
		CHandle hGameObject{};
		PX_SHAPE_TYPE eType{ PX_SHAPE_TYPE::BOX };
		uint32_t iSubIndex{ std::numeric_limits<uint32_t>::max() };
	};

	inline constexpr uint32_t PX_DEFAULT_LAYER = 1u;
	inline constexpr uint32_t PX_ALL_LAYERS = std::numeric_limits<uint32_t>::max();

	struct PX_FILTER_DESC
	{
		uint32_t iLayer{ PX_DEFAULT_LAYER };
		uint32_t iSimulationMask{ PX_ALL_LAYERS };
		uint32_t iQueryMask{ PX_ALL_LAYERS };
	};

	struct PX_SYNC_DATA
	{
		_float3 vPos{};
		_float4 vQuat{};
	};

	struct PX_ON_COLLISION_DATA
	{
		bool hello;
	};

	struct PX_ON_TRIGGER_DATA
	{
		bool hello;
	};

	struct PX_RAYCAST_RESULT
	{
		_bool bHit{ false };
		_float3 vHitpos{}; // 충돌 지점
		_float3 vHitNormal{}; // 충돌 표면의 법선 벡터
		_float fDistance{}; // 시작점으로부터의 거리
		CGameObject* pGameObject{};
	};

}
