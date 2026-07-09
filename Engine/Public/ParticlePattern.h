#pragma once
#include "Engine_Defines.h"
#include "Engine_Struct.h"

NS_BEGIN(Engine)

// ============================================================
// ParticlePattern
// ------------------------------------------------------------
// "파티클을 어떤 모양으로 배치할까"에 대한 순수 계산만 담당합니다.
// CParticleManager나 렌더링, 스폰 실행과는 완전히 무관합니다.
// 여기 있는 함수들은 전부 std::vector<PARTICLE_SPAWN_DATA>를
// "계산해서 반환"만 하고, 실제로 스폰(Spawn 호출)하는 책임은
// 호출부(보스 패턴 코드, 맵 초기화 코드 등)에 있습니다.
//
// 새로운 배치 모양이 필요하면 이 파일에 함수를 추가하세요.
// CParticleManager.cpp는 건드릴 필요가 없습니다.
// ============================================================
namespace ParticlePattern
{
	// 계단 모양으로 일렬 배치 (한 칸씩 위로 + 앞으로)
	std::vector<PARTICLE_SPAWN_DATA> MakeStairs(
		const _float3& vStartPos,
		uint32_t iStepCount,
		_float fStepWidth,
		_float fStepHeight,
		_float fStepDepth,
		_float fLife = 1.f,
		const _float4& color = _float4(1.f, 1.f, 1.f, 1.f),
		const _float4& emissive = _float4(0.f, 0.f, 0.f, 0.f)
		);

	// 원형(링) 배치
	std::vector<PARTICLE_SPAWN_DATA> MakeCircle(
		const _float3& vCenter,
		_float fRadius,
		uint32_t iCount,
		_float fSize,
		_float fLife = 1.f,
		const _float4& color = _float4(1.f, 1.f, 1.f, 1.f),
		const _float4& emissive = _float4(0.f, 0.f, 0.f, 0.f),
		_float fYOffset = 0.f);

	// 격자(그리드) 배치
	std::vector<PARTICLE_SPAWN_DATA> MakeGrid(
		const _float3& vOrigin,
		uint32_t iRows,
		uint32_t iCols,
		_float fSpacing,
		_float fSize,
		_float fLife = 1.f,
		const _float4& color = _float4(1.f, 1.f, 1.f, 1.f),
		const _float4& emissive = _float4(0.f, 0.f, 0.f, 0.f)
		);

	// 나선형 배치 (예: 보스 주변 상승 나선 패턴)
	std::vector<PARTICLE_SPAWN_DATA> MakeSpiral(
		const _float3& vCenter,
		_float fRadius,
		uint32_t iCount,
		_float fHeightPerStep,
		_float fAngleStepDeg,
		_float fSize,
		_float fLife = 1.f,
		const _float4& color = _float4(1.f, 1.f, 1.f, 1.f),
		const _float4& emissive = _float4(0.f, 0.f, 0.f, 0.f)
		);
}

NS_END
