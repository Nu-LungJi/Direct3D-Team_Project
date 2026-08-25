#pragma once
#include "Engine_Base.h"
#include <nlohmann/json.hpp>
#include <shared_mutex>

NS_BEGIN(Engine)

// 모델 태그별 머티리얼 설정을 스레드 안전하게 보관하고
// Material.json 파일로 저장하거나 불러온다.
class ENGINE_DLL CMapMaterialRepository final
{
public:
	using MATERIAL_MAP = std::unordered_map<std::string, MATERIAL_DESC>;

public:
	HRESULT SaveFile(
		const std::filesystem::path& filePath,
		const MATERIAL_MAP& materials) const;
	HRESULT LoadFile(const std::filesystem::path& filePath);

	// 모델 로딩 스레드에서도 안전하게 조회할 수 있다.
	MATERIAL_DESC Find(const std::string& modelName) const;
	// 로드된 값을 런타임 모델에 적용할 때 사용할 복사본을 반환한다.
	MATERIAL_MAP GetSnapshot() const;
	void Clear();

private:
	nlohmann::ordered_json WriteMaterial(const MATERIAL_DESC& material) const;
	MATERIAL_DESC ReadMaterial(const nlohmann::ordered_json& json) const;
	_float3 ReadFloat3(const nlohmann::ordered_json& json, const char* key, const _float3& fallback) const;

private:
	MATERIAL_MAP m_Materials;
	mutable std::shared_mutex m_Mutex;
};

NS_END
