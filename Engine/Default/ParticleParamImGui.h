// ParticleParamImGui.h
#pragma once
#include "ParticleParams.h"
NS_BEGIN(Engine)

// ---- 타입별 그리기 함수 (전 패턴이 재사용) ----
inline void DrawField(const char* name, _float3& v) { ImGui::DragFloat3(name, &v.x, 0.05f); }
inline void DrawField(const char* name, _float& v) { ImGui::DragFloat(name, &v, 0.05f); }
inline void DrawField(const char* name, uint32_t& v) { int tmp = (int)v; if (ImGui::DragInt(name, &tmp, 1, 0, 9999)) v = (uint32_t)std::max(0, tmp); }
inline void DrawField(const char* name, _float4& v) { ImGui::ColorEdit4(name, &v.x); }
inline void DrawField(const char* name, _bool& v){bool tmp = (bool)v;if (ImGui::Checkbox(name, &tmp))v = tmp;}

#define DRAW_PARAM_FIELD(type, name, defaultVal) DrawField(#name, p.name);
inline void DrawImGui(SStairsParam& p) { STAIRS_FIELDS(DRAW_PARAM_FIELD) }
inline void DrawImGui(SCircleParam& p) { CIRCLE_FIELDS(DRAW_PARAM_FIELD) }
inline void DrawImGui(SSpiralParam& p) { SPIRAL_FIELDS(DRAW_PARAM_FIELD) }
inline void DrawImGui(SStraightGroundParam& p) { STRAIGHT_GROUND_FIELDS(DRAW_PARAM_FIELD) }
#undef DRAW_PARAM_FIELD

// ---- json 저장/로드 (타입별 헬퍼) ----
inline void SaveField(nlohmann::json& out, const char* name, const _float3& v) { out[name] = { v.x, v.y, v.z }; }
inline void SaveField(nlohmann::json& out, const char* name, const _float4& v) { out[name] = { v.x, v.y, v.z, v.w }; }
inline void SaveField(nlohmann::json& out, const char* name, const _float& v) { out[name] = v; }
inline void SaveField(nlohmann::json& out, const char* name, const uint32_t& v) { out[name] = v; }
inline void SaveField(nlohmann::json& out, const char* name, const _bool& v){out[name] = (bool)v;}

inline void LoadField(const nlohmann::json& in, const char* name, _float3& v)
{
	auto a = in.value(name, std::vector<float>{v.x, v.y, v.z});
	v = { a[0], a[1], a[2] };
}
inline void LoadField(const nlohmann::json& in, const char* name, _float4& v)
{
	auto a = in.value(name, std::vector<float>{v.x, v.y, v.z, v.w});
	v = { a[0], a[1], a[2], a[3] };
}
inline void LoadField(const nlohmann::json& in, const char* name, _float& v) { v = in.value(name, v); }
inline void LoadField(const nlohmann::json& in, const char* name, uint32_t& v) { v = in.value(name, v); }
inline void LoadField(const nlohmann::json& in, const char* name, _bool& v)
{
	if (in.contains(name))
		v = in[name].get<bool>();
}

#define SAVE_PARAM_FIELD(type, name, defaultVal) SaveField(out, #name, p.name);
#define LOAD_PARAM_FIELD(type, name, defaultVal) LoadField(in, #name, p.name);

inline void SaveParam(const SStairsParam& p, nlohmann::json& out) { STAIRS_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SCircleParam& p, nlohmann::json& out) { CIRCLE_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SSpiralParam& p, nlohmann::json& out) { SPIRAL_FIELDS(SAVE_PARAM_FIELD) }
inline void SaveParam(const SStraightGroundParam& p, nlohmann::json& out) { STRAIGHT_GROUND_FIELDS(SAVE_PARAM_FIELD) }

inline void LoadParam(SStairsParam& p, const nlohmann::json& in) { STAIRS_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SCircleParam& p, const nlohmann::json& in) { CIRCLE_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SSpiralParam& p, const nlohmann::json& in) { SPIRAL_FIELDS(LOAD_PARAM_FIELD) }
inline void LoadParam(SStraightGroundParam& p, const nlohmann::json& in) { STRAIGHT_GROUND_FIELDS(LOAD_PARAM_FIELD) }

#undef SAVE_PARAM_FIELD
#undef LOAD_PARAM_FIELD

// ---- variant 통합 버전 (커맨드 큐/프리셋에서 사용) ----
inline void DrawImGui(PatternParamVariant& v)
{
	std::visit([](auto& param) { DrawImGui(param); }, v);
}
inline void SaveParam(const PatternParamVariant& v, nlohmann::json& out)
{
	std::visit([&](const auto& param) { SaveParam(param, out); }, v);
}
// variant 로드는 kind 인덱스를 먼저 알아야 하므로 개별 LoadParam(struct&, json)을 직접 호출

NS_END
