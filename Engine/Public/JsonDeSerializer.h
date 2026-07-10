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

public:
	void Read(const std::string& key, bool& outValue) override;
	void Read(const std::string& key, uint32_t& outValue) override;
	void Read(const std::string& key, uint64_t& outValue) override;
	void Read(const std::string& key, int& outValue) override;
	void Read(const std::string& key, float& outValue) override;
	void Read(const std::string& key, std::string& outValue) override;
	void Read(const std::string& key, _float2& outValue)  override;
	void Read(const std::string& key, _float3& outValue) override;
	void Read(const std::string& key, _float4& outValue)  override;
	void Read(const std::string& key, _float4x4& outValue) override;
	void Read(const std::string& key, ISerializable& outValue) override;

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
	std::vector<size_t> m_arrayIndexStack;

	std::vector<std::vector<std::string>> m_mapKeysStack;
	std::vector<size_t> m_mapKeyIndexStack;

public:
	static UPtr<CJsonDeSerializer> Create(const std::string& path);
};

NS_END
