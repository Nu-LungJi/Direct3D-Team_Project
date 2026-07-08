#pragma once
#include "Engine_Defines.h"
#include "SerializerInterface.h"
#include <nlohmann/json.hpp>
#include <vector>

NS_BEGIN(Engine)

class ENGINE_DLL CJsonDeSerializer : public CEngineBase, public IDeserializer
{
public:
	DECLARE_DERIVED_TYPE(CJsonDeSerializer, CEngineBase)

	CJsonDeSerializer();
	~CJsonDeSerializer() override;

public:
	HRESULT LoadFromFile(const std::string& path);

	// IDeserializer 구현
	void Read(const std::string& key, int& outValue) override;
	void Read(const std::string& key, float& outValue) override;
	void Read(const std::string& key, std::string& outValue) override;
	void Read(const std::string& key, _float2& outValue)  override;
	void Read(const std::string& key, _float3& outValue) override;
	void Read(const std::string& key, _float4& outValue)  override;
	void Read(const std::string& key, _float4x4& outValue) override;
	void Read(const std::string& key, ISerializable& outValue) override;

	// 배열을 순회하려면 크기를 알아야 하므로 size_t를 반환하도록 합니다.
	size_t StartArray(const std::string& key) override;
	void EndArray() override;

	size_t StartMap(const std::string& key) override;
	void EndMap() override;
	std::string ReadMapKey() override;

private:
	HRESULT Initialize();

private:
	nlohmann::json m_json{};
	std::vector<nlohmann::json*> m_nodeStack;
	std::vector<size_t> m_arrayIndexStack; // [핵심] 배열 순회 시 인덱스 추적기

	std::vector<std::vector<std::string>> m_mapKeysStack;
	std::vector<size_t> m_mapKeyIndexStack;

public:
	static UPtr<CJsonDeSerializer> Create(const std::string& path);
};

NS_END
