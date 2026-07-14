#pragma once
#include "Engine_Defines.h"
#include "Engine_Struct.h"
NS_BEGIN(Engine)


// 
// ============================================================
// X-매크로: 필드 목록을 한 곳에서만 정의
// X(타입, 이름, 기본값)
// ============================================================



#define STAIRS_FIELDS(X) \
    X(_float3, vStartPos, _float3(0,0,0)) \
    X(uint32_t, iStepCount, 5) \
    X(_float, fStepWidth, 1.f) \
    X(_float, fStepHeight, 1.f) \
    X(_float, fStepDepth, 1.f) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0))

#define CIRCLE_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(_float, fRadius, 3.f) \
    X(uint32_t, iCount, 12) \
    X(_float, fSize, 1.f) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0)) \
    X(_float, fYOffset, 0.f)

#define SPIRAL_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(_float, fRadius, 3.f) \
    X(uint32_t, iCount, 20) \
    X(_float, fHeightPerStep, 0.2f) \
    X(_float, fAngleStepDeg, 15.f) \
    X(_float, fSize, 1.f) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0))

#define STRAIGHT_GROUND_FIELDS(X) \
    X(_float3, vStartPos, _float3(0,0,0)) \
    X(uint32_t, iRow, 3) \
    X(uint32_t, iCol, 3) \
    X(_float, fOffsetX, 1.f) \
    X(_float, fOffsetZ, 1.f) \
    X(_float, fSpawnDelay, 0.1f) \
    X(_float, fSize, 1.f) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0))

// ============================================================
// struct 자동 생성 매크로
// ============================================================
#define DECLARE_PARAM_STRUCT(StructName, FIELD_LIST) \
struct StructName \
{ \
    FIELD_LIST(DECLARE_PARAM_FIELD) \
}; \

#define DECLARE_PARAM_FIELD(type, name, defaultVal) type name = defaultVal;

// 실제 struct 선언 (매크로 한 줄씩)
struct SStairsParam { STAIRS_FIELDS(DECLARE_PARAM_FIELD) };
struct SCircleParam { CIRCLE_FIELDS(DECLARE_PARAM_FIELD) };
struct SSpiralParam { SPIRAL_FIELDS(DECLARE_PARAM_FIELD) };
struct SStraightGroundParam { STRAIGHT_GROUND_FIELDS(DECLARE_PARAM_FIELD) };

#undef DECLARE_PARAM_FIELD



using PatternParamVariant = std::variant<SStairsParam, SCircleParam, SSpiralParam, SStraightGroundParam>;

// 콤보박스 등에서 쓸 이름 목록 (variant 인덱스와 순서 반드시 일치)
inline constexpr const char* PATTERN_KIND_NAMES[] =
{
	"Stairs", "Circle",  "Spiral", "StraightGround"
};

// 인덱스로 기본값 variant 생성 (콤보박스에서 종류 바꿀 때 사용)
inline PatternParamVariant MakeDefaultPatternParam(int index)
{
	switch (index)
	{
	case 0: return SStairsParam{};
	case 1: return SCircleParam{};
	case 2: return SSpiralParam{};
	case 3: return SStraightGroundParam{};
	default: return SStairsParam{};
	}
}


NS_END
